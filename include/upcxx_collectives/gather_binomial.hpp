//  Copyright (c) 2020 Christopher Tayloa
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_GATHER_BINOMIAL_HPP__
#define __UPCXX_GATHER_BINOMIAL_HPP__

#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <unistd.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <upcxx/upcxx.hpp>

#include "collective_traits.hpp" 
#include "gather.hpp"

namespace upcxx { namespace utils { namespace collectives {

template< typename BlockingPolicy, typename Serialization >
class gather< tree_binomial, BlockingPolicy, Serialization > {

    using value_type_t = typename Serialization::value_type;
    using serializer_t = typename Serialization::serializer;
    using deserializer_t = typename Serialization::deserializer;

private:
    std::int64_t root;
    std::int64_t mask;
    upcxx::dist_object< std::tuple< std::int32_t, std::vector<std::string> > > args;

public:
    using communication_pattern = upcxx::utils::collectives::tree_binomial;
    using blocking_policy = BlockingPolicy;

    gather(const std::int64_t root_=0) :
        root(root_),
        mask(0x1),
        args{std::make_tuple(0, std::vector<std::string>{})} {
    }

    template<typename InputIterator, typename OutputIterator>
    void operator()(InputIterator input_beg, InputIterator input_end, OutputIterator out_beg) {
        // https://en.cppreference.com/w/cpp/header/iterator
        //
        using value_type = typename std::iterator_traits<InputIterator>::value_type;

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

        const std::int64_t rank_n = upcxx::rank_n();
        const std::int64_t rank_me = (upcxx::rank_me() + root) % rank_n;

        // cache local data set into transmission buffer
        if(rank_me != root) {
            //std::stringstream value_buffer{};
            //boost::archive::binary_oarchive value_oa{value_buffer};
            value_type_t value_buffer{};
            serializer_t value_oa{value_buffer};

            const std::int64_t iter_diff = (input_end-input_beg);

            value_oa << rank_me << iter_diff;
            for(auto seg_itr = input_beg; seg_itr != input_end; ++seg_itr) {
                value_oa << (*seg_itr);
            }

            std::get<1>(*args).push_back(Serialization::get_buffer(value_buffer));
        }

        for(std::int64_t i = 0; i < logp; ++i) {
            if((mask & rank_me) == 0) {
                if((rank_me | mask) < rank_n) {
                    // recv this atomic flips back and forth
                    //
                    upcxx::progress();
                    while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                        upcxx::progress();
                    }
                }
            }
            else {
                // leaf-parent exchange send
                //
                const std::int64_t parent = ((rank_me & (~mask)) + root) % rank_n;

                upcxx::rpc_ff(
                    parent,
                    [](upcxx::dist_object< std::tuple< std::int32_t, std::vector<std::string> > > & args_, std::vector<std::string> data_) {
                        std::get<1>(*args_).reserve(std::get<1>(*args_).size() + data_.size());
                        std::get<1>(*args_).insert(std::get<1>(*args_).end(), data_.begin(), data_.end());
                        atomic_xchange( &std::get<0>(*args_), 0, 1 );
                    }, args, std::get<1>(*args)
                );
            }

            mask <<= 1;

            // make sure all outstanding communications terminate properly
            // if there is not barrier, then the loop will continue and
            // potentially deadlock on a PE
            //
            upcxx::barrier();
        }

        if(rank_me < 1) {
            for(auto & recv_str : std::get<1>(*args)) {
                std::int64_t in_rank = 0, in_count = 0;
                //std::stringstream value_buffer{recv_str};
                //boost::archive::binary_iarchive iarch{value_buffer};
                value_type_t value_buffer{recv_str};
                deserializer_t iarch{value_buffer};

                iarch >> in_rank >> in_count;
                for(auto count = 0; count < in_count; ++count) {
                    const std::int64_t out_pos = (in_rank*in_count) + count;
                    iarch >> (*(out_beg + out_pos));
                }
            }
        }

        if constexpr(is_blocking<BlockingPolicy>()) {
            upcxx::barrier(); // make sure communications terminate properly
        }

    } // end operator()

};

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
