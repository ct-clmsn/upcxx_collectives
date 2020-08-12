//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_SCATTER_BINOMIAL_HPP__
#define __UPCXX_SCATTER_BINOMIAL_HPP__

#include <string>
#include <sstream>
#include <iterator>
#include <unistd.h>

//#include <boost/archive/binary_oarchive.hpp>
//#include <boost/archive/binary_iarchive.hpp>
#include <upcxx/upcxx.hpp>

#include "collective_traits.hpp" 
#include "serialization.hpp"
#include "scatter.hpp"
#include "utils.hpp"

namespace upcxx { namespace utils { namespace collectives {

template< typename BlockingPolicy, typename Serialization >
class scatter<tree_binomial, BlockingPolicy, Serialization> {

    using svalue_type_t = typename Serialization::value_type;
    using serializer_t = typename Serialization::serializer;
    using deserializer_t = typename Serialization::deserializer;

private:
    std::int64_t root;
    upcxx::dist_object< std::tuple<std::int32_t, std::string> > args;

public:
    using communication_pattern = tree_binomial;
    using blocking_policy = BlockingPolicy;                                                                                                                                                                         
    scatter(const std::int64_t root_=0) :
        root(root_),
        args{std::make_tuple(0, std::string{})} {
    }

    template<typename InputIterator, typename OutputIterator>
    void operator()(InputIterator input_beg, InputIterator input_end, OutputIterator out_beg) {
        // https://en.cppreference.com/w/cpp/header/iterator
        //
        using value_type_t = typename std::iterator_traits<InputIterator>::value_type;

        const auto block_size = static_cast<std::int64_t>(input_end - input_beg) /
            static_cast<std::int64_t>(upcxx::rank_n());

        const std::int64_t logp =
            static_cast<std::int64_t>(
                std::log2(
                    static_cast<double>(
                        upcxx::rank_n()
                    )
                )
            );

        std::int64_t rank_n = upcxx::rank_n();
        std::int64_t rank_me = upcxx::rank_me();
        std::int64_t k = rank_n / 2;
        bool not_recieved = true;

        for(std::int64_t i = 0; i < logp; ++i) {

            const auto twok = 2 * k;
            if( (rank_me % twok) == 0 ) {

                //std::stringstream value_buffer{};
                svalue_type_t value_buffer{};
                if(std::get<1>(*args).size() < 1) {

                    if(not_recieved) {
                        not_recieved = false;

                        auto cpy_itr = input_beg;
                        for(std::int64_t i = 0; i < block_size; ++i) {
                            (*out_beg++) = (*(input_beg+i));
                            //out_beg++;
                        }
                    }

                    //boost::archive::binary_oarchive value_oa{value_buffer};
                    serializer_t value_oa{value_buffer};

                    const auto seg_beg = ((rank_me + k) % rank_n) * block_size;
                    const auto seg_end = ((rank_n - (rank_me % rank_n)) * block_size) + 1;

                    // fill buffer with segment to send to child
                    //
                    auto sitr_beg = input_beg + seg_beg;
                    auto sitr_end = input_beg + seg_end;

                    for(auto sitr = sitr_beg; sitr != sitr_end; sitr++) {
                        if( ((sitr - sitr_beg) % block_size) == 0 ) {
                            value_oa << block_size; 
                        }
                        value_oa << (*sitr);
                    }

                }
                else {
                    if constexpr( serialization::is_boost<Serialization>::value ) {
                        value_buffer << std::get<1>(*args);
                    }
                    else {
                        value_buffer.append(std::get<1>(*args));
                    }
                }

                upcxx::rpc_ff(
                    (rank_me + k),
                    [](upcxx::dist_object< std::tuple<std::int32_t, std::string> > & args_, std::string data_) {
                        std::get<1>(*args_).resize(data_.size());
                        std::get<1>(*args_).insert(0, data_);

                        atomic_xchange( &std::get<0>(*args_), 0, 1 );
                    }, args, Serialization::get_buffer(value_buffer)
                );
            }
            else if( not_recieved && ((rank_me % twok) == k) ) {

                upcxx::progress();
                while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                //std::stringstream recv_buffer{std::get<1>(*args)};
                //boost::archive::binary_iarchive recv_ia{recv_buffer};
                svalue_type_t recv_buffer{std::get<1>(*args)};
                deserializer_t recv_ia{recv_buffer};

                std::int64_t element_count = 0;
                recv_ia >> element_count;

                // https://en.cppreference.com/w/cpp/algorithm/copy
                //
                for(std::int64_t i = 0; i < element_count; ++i) {
                    recv_ia >> (*out_beg++);
                }

                not_recieved = false;
            }

            k /= 2;
            rank_n /= 2;

        } // end for loop

        if constexpr(is_blocking<BlockingPolicy>()) {
            upcxx::barrier(); // make sure communications terminate properly
        }

    } // end operator()

};

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
