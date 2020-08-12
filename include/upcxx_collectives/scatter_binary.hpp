//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_SCATTER_BINARY_HPP__
#define __UPCXX_SCATTER_BINARY_HPP__

#include <string>
#include <sstream>
#include <iterator>
#include <numeric>
#include <algorithm>
#include <vector>
#include <unistd.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <upcxx/upcxx.hpp>

#include "collective_traits.hpp" 
#include "scatter.hpp"

namespace upcxx { namespace utils { namespace collectives {

template< typename BlockingPolicy, typename Serializer >
class scatter<tree_binary, BlockingPolicy, Serializer> {

    using value_type_t = typename Serializer::value_type;
    using serializer_t = typename Serializer::serializer;
    using deserializer_t = typename Serializer::deserializer;

private:
    std::int64_t root, cas_count, rel_rank, left, right;
    upcxx::dist_object< std::tuple<std::int32_t, std::vector<std::string> > > args;

public:
    using communication_pattern = tree_binary;
    using blocking_policy = BlockingPolicy;                                                                                                                                                                         
    scatter(const std::int64_t root_=0) :
        root(root_),
        cas_count(0),
        rel_rank(0),
        left(0),
        right(0),
        args{std::make_tuple(0, std::vector<std::string>{})} {

        rel_rank = (upcxx::rank_me()-root_) % upcxx::rank_n();
        left = (2*rel_rank) + 1;
        right = (2*rel_rank) + 2;
        cas_count = ( left < upcxx::rank_n() ) + ( right < upcxx::rank_n() );
    }

    template<typename InputIterator, typename OutputIterator>
    void operator()(InputIterator input_beg, InputIterator input_end, OutputIterator out_beg) {
        // https://en.cppreference.com/w/cpp/header/iterator
        //
        using itr_value_type_t = typename std::iterator_traits<InputIterator>::value_type;

        const auto data_n = static_cast<std::int64_t>(input_end - input_beg); 
        const auto block_size = data_n /
            static_cast<std::int64_t>(upcxx::rank_n());

        std::int64_t rank_n = upcxx::rank_n();
        const std::int64_t P = static_cast<std::int64_t>(std::log2(rank_n));
        std::int64_t k = rank_n / 2;
        const std::int64_t rank_me = rel_rank;

        if(rank_me == 0) {
            InputIterator litr{};

            {
                const auto input_block_end = input_beg+block_size;
                for(litr = input_beg; litr != input_block_end; ++litr) {
                    (*out_beg++) = (*litr);
                }
            }

            std::vector<std::string> lvalue_buffers{}, rvalue_buffers{};
            std::int64_t pos = block_size;
            for(auto i = 0; i < P+1; i <<= 1) {

                if( pos >= data_n-1) { break; }

                value_type_t lval_buffer{};
                serializer_t lvalue_oa(lval_buffer);
                const auto lpos_end = pos + block_size;
                lvalue_oa << block_size;
                for(; pos < lpos_end; ++pos) {
                    lvalue_oa << *(input_beg+pos);
                }
                lvalue_buffers.push_back(Serializer::get_buffer(lval_buffer));

                pos += block_size;

                if( pos >= data_n-1) { break; }

                value_type_t rval_buffer{};
                serializer_t rvalue_oa(rval_buffer);
                rvalue_oa << block_size;
                const auto rpos_end = pos + block_size;
                for(; pos < rpos_end; ++pos) {
                    rvalue_oa << *(input_beg+pos);
                }
                rvalue_buffers.push_back(Serializer::get_buffer(rval_buffer));

                pos += block_size;
            }
    
            for(std::int64_t i = 0; i < cas_count; ++i) {
 
                const std::int64_t lr_rank = (i == 0) ? left : right;
                auto & value_buffers = (i == 0) ? lvalue_buffers : rvalue_buffers;
                upcxx::rpc_ff(
                    lr_rank,
                    [](upcxx::dist_object< std::tuple<std::int32_t, std::vector<std::string> > > & args_, std::vector<std::string> data_) {
                        std::get<1>(*args_).resize(data_.size());
                        std::copy(data_.begin(), data_.end(), std::get<1>(*args_).begin());

                        atomic_xchange( &std::get<0>(*args_), 0, 1 );
                    }, args, value_buffers
                );

            }
        }
        else {
            upcxx::progress();
            while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                upcxx::progress();
            }

            std::int64_t k = 0, count = 0;
            auto value_buffer_itr = std::get<1>(*args).begin();
            value_type_t recv_buffer{(*value_buffer_itr)};
            deserializer_t value_ia{recv_buffer};
            value_ia >> count;
            std::get<1>(*args).erase(value_buffer_itr);

            for(std::int64_t i = 0; i < count; ++i) {
                value_ia >> (*out_beg++);
            }

            for(std::int64_t i = 0; i < cas_count; ++i) {
 
                const auto parent = ( i == 0 ) ? left : right;
                std::vector<std::string> send_buffer{Serializer::get_buffer(recv_buffer)};
                upcxx::rpc_ff(
                    parent,
                    [](upcxx::dist_object< std::tuple<std::int32_t, std::vector<std::string> > > & args_, std::vector<std::string> data_) {
                        std::get<1>(*args_).resize(data_.size());
                        std::copy(data_.begin(), data_.end(), std::get<1>(*args_).begin());

                        atomic_xchange( &std::get<0>(*args_), 0, 1 );
                    }, args, send_buffer 
                );
            }
        }

        if constexpr(is_blocking<BlockingPolicy>()) {
            upcxx::barrier(); // make sure communications terminate properly
        }
    }

};

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
