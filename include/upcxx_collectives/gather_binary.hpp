//  Copyright (c) 2020 Christopher Tayloa
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_GATHER_BINARY_HPP__
#define __UPCXX_GATHER_BINARY_HPP__

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

#if defined(__CLANG_ATOMICS)
#define atomic_xchange(object, object_value, value) \
    __sync_swap(object, value)
#elif defined(__GNUC_ATOMICS)
#define atomic_xchange(object, object_value, value) \
    __sync_bool_compare_and_swap( object, object_value, value)
#elif __has_builtin(__sync_swap)
/* Clang provides a full-barrier atomic exchange - use it if available. */
#define atomic_xchange(object, object_value, value) \
    __sync_swap(object, value)
#endif

template<typename BlockingPolicy >
class gather<tree_binary, BlockingPolicy> {

private:
    std::int64_t root;
    std::int64_t cas_count;
    upcxx::dist_object< std::tuple< std::int32_t, std::int32_t, std::vector<std::string>, std::vector<std::string> > > args;

public:
    using communication_pattern = upcxx::utils::collectives::tree_binary;
    using blocking_policy = BlockingPolicy;

    gather(const std::int64_t root_=upcxx::rank_me()) :
        root(root_),
        cas_count(0),
        args{std::make_tuple(0, 0, std::vector<std::string>{}, std::vector<std::string>{})} {

        const std::int64_t left = (2*upcxx::rank_me()) + 1;
        const std::int64_t right = (2*upcxx::rank_me()) + 2;
        cas_count = ( left < upcxx::rank_n() ) + ( right < upcxx::rank_n() );
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
        const std::int64_t rank_me = upcxx::rank_me();

        // i.am.root.
        if(rank_me == 0) {

            std::vector<std::string> recv_str_vec{};

            if(cas_count > 1) {
                upcxx::progress();
                while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                upcxx::progress();
                while(!atomic_xchange( &std::get<1>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                recv_str_vec.reserve(std::get<2>(*args).size() + std::get<3>(*args).size());
                recv_str_vec.insert(recv_str_vec.end(), std::get<2>(*args).begin(), std::get<2>(*args).end());
                recv_str_vec.insert(recv_str_vec.end(), std::get<3>(*args).begin(), std::get<3>(*args).end());
            }
            else {
                upcxx::progress();
                while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                recv_str_vec.reserve(std::get<2>(*args).size());
                recv_str_vec.insert(recv_str_vec.end(), std::get<2>(*args).begin(), std::get<2>(*args).end());
            }

            for(auto & recv_str : recv_str_vec) {
                std::int64_t in_rank = 0, in_count = 0;
                std::stringstream value_buffer{recv_str};
                boost::archive::binary_iarchive iarch{value_buffer};

                iarch >> in_rank >> in_count;
                for(auto count = 0; count < in_count; ++count) {
                    const std::int64_t out_pos = (in_rank*in_count) + count;
                    iarch >> (*(out_beg + out_pos));
                }
            }

        }
        else {

            const std::int64_t parent = (rank_me - 1) / 2;
            const std::int64_t left = (2*rank_me) + 1;
            const std::int64_t right = (2*rank_me) + 2;
            const bool is_leaf = ( left >= rank_n ) && ( right >= rank_n );
            const bool is_even = (rank_me % 2) == 0;

            std::stringstream value_buffer{};
            boost::archive::binary_oarchive value_oa{value_buffer};
            const std::int64_t iter_diff = (input_end-input_beg);

            value_oa << rank_me << iter_diff;
            for(auto seg_itr = input_beg; seg_itr != input_end; ++seg_itr) {
                value_oa << (*seg_itr);
            }

            if(is_leaf) {
                if(is_even) {
                    upcxx::rpc_ff(
                        parent,
                        [](upcxx::dist_object< std::tuple<std::int32_t, std::int32_t, std::vector<std::string>, std::vector<std::string>> > & args_, std::string data_) {
                            std::get<3>(*args_).push_back(data_);
                            atomic_xchange( &std::get<1>(*args_), 0, 1 );
                        }, args, value_buffer.str()
                    );
                }
                else {
                    upcxx::rpc_ff(
                        parent,
                        [](upcxx::dist_object< std::tuple<std::int32_t, std::int32_t, std::vector<std::string>, std::vector<std::string>> > & args_, std::string data_) {
                            std::get<2>(*args_).push_back(data_);
                            atomic_xchange( &std::get<0>(*args_), 0, 1 );
                        }, args, value_buffer.str()
                    );
                }
            }
            else {

                std::vector<std::string> recv_str_vec{};

                if(cas_count > 1) {
                    upcxx::progress();
                    while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                        upcxx::progress();
                    }

                    upcxx::progress();
                    while(!atomic_xchange( &std::get<1>(*args), 1, 0 )) {
                        upcxx::progress();
                    }

                    recv_str_vec.reserve(std::get<2>(*args).size() + std::get<3>(*args).size());
                    recv_str_vec.insert(recv_str_vec.end(), std::get<2>(*args).begin(), std::get<2>(*args).end());
                    recv_str_vec.insert(recv_str_vec.end(), std::get<3>(*args).begin(), std::get<3>(*args).end());
                }
                else {
                    upcxx::progress();
                    while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                        upcxx::progress();
                    }

                    recv_str_vec.reserve(std::get<2>(*args).size());
                    recv_str_vec.insert(recv_str_vec.end(), std::get<2>(*args).begin(), std::get<2>(*args).end());
                }

                recv_str_vec.push_back(value_buffer.str());

                // forced to deserialize/unmarshal data recieved
                // b/c of how boost serialization works; this code
                // repackages the data for transmission
                //
                if(is_even) {
                    upcxx::rpc_ff(
                        parent,
                        [](upcxx::dist_object< std::tuple< std::int32_t, std::int32_t, std::vector<std::string>, std::vector<std::string> > > & args_, std::vector<std::string> data_) {
                            std::get<3>(*args_).reserve(std::get<3>(*args_).size() + data_.size());
                            std::get<3>(*args_).insert(std::get<3>(*args_).end(), data_.begin(), data_.end());
                            atomic_xchange( &std::get<1>(*args_), 0, 1 );
                        }, args, recv_str_vec
                    );
                }
                else {
                    upcxx::rpc_ff(
                        parent,
                        [](upcxx::dist_object< std::tuple< std::int32_t, std::int32_t, std::vector<std::string>, std::vector<std::string> > > & args_, std::vector<std::string> data_) {
                            std::get<2>(*args_).reserve(std::get<2>(*args_).size() + data_.size());
                            std::get<2>(*args_).insert(std::get<2>(*args_).end(), data_.begin(), data_.end());
                            atomic_xchange( &std::get<0>(*args_), 0, 1 );
                        }, args, recv_str_vec
                    );
                }
            } // leaf/non-leaf end
        } // end non-root else

        if constexpr(is_blocking<BlockingPolicy>()) {
            upcxx::barrier(); // make sure communications terminate properly
        }

    } // end operator()

};

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
