//  Copyright (c) 2020 Christopher Tayloa
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_REDUCE_BINARY_HPP__
#define __UPCXX_REDUCE_BINARY_HPP__

#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <numeric>
#include <unistd.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <upcxx/upcxx.hpp>

#include "collective_traits.hpp" 
#include "reduce.hpp"

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

template< typename BlockingPolicy >
class reduce<tree_binary, BlockingPolicy> {

private:
    std::int64_t root;
    std::int64_t cas_count;
    upcxx::dist_object< std::tuple<std::int32_t, std::int32_t, std::string, std::string> > args;

public:
    using communication_pattern = upcxx::utils::collectives::tree_binary;
    using blocking_policy = BlockingPolicy;

    reduce(const std::int64_t root_=upcxx::rank_me()) :
        root(root_),
        cas_count(0),
        args{std::make_tuple(0, 0, std::string{}, std::string{})} {

        const std::int64_t left = (2*upcxx::rank_me()) + 1;
        const std::int64_t right = (2*upcxx::rank_me()) + 2;
        cas_count = ( left < upcxx::rank_n() ) + ( right < upcxx::rank_n() );
    }

    template<typename InputIterator, typename BinaryOp>
    void operator()(InputIterator input_beg, InputIterator input_end, typename std::iterator_traits<InputIterator>::value_type init, BinaryOp op, typename std::iterator_traits<InputIterator>::value_type & output) {
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
        const std::int64_t rank_me = ((upcxx::rank_me() - root) + rank_n) % rank_n;

        // i.am.root.
        if(rank_me == 0) {

            if(cas_count > 1) {
                upcxx::progress();
                while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                upcxx::progress();
                while(!atomic_xchange( &std::get<1>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                std::vector<value_type_t> recv_vec{};
                {
                    value_type_t odd{};
                    std::stringstream value_buffer_odd{ std::get<2>(*args) };
                    boost::archive::binary_iarchive iarch_odd{value_buffer_odd};
                    iarch_odd >> odd;
                    recv_vec.push_back(std::move(odd));
                }

                {
                    value_type_t even{};
                    std::stringstream value_buffer_even{ std::get<3>(*args) };
                    boost::archive::binary_iarchive iarch_even{value_buffer_even};
                    iarch_even >> even;
                    recv_vec.push_back(std::move(even));
                }

                recv_vec.push_back(std::reduce(input_beg, input_end, init, op));
                output = std::reduce(recv_vec.begin(), recv_vec.end(), init, op); 
            }
            else {
                upcxx::progress();
                while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                value_type_t val{};
                std::stringstream value_buffer_odd{ std::get<2>(*args) };
                boost::archive::binary_iarchive iarch_odd{value_buffer_odd};

                iarch_odd >> val;
                output = val;
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

            const value_type_t result_local = std::reduce(input_beg, input_end, init, op);

            if(is_leaf) {
                if(is_even) {
                    value_oa << result_local;
                    upcxx::rpc_ff(
                        parent,
                        [](upcxx::dist_object< std::tuple<std::int32_t, std::int32_t, std::string, std::string> > & args_, std::string data_) {
                            std::get<3>(*args_).append(data_);
                            atomic_xchange( &std::get<1>(*args_), 0, 1 );
                        }, args, value_buffer.str()
                    );
                }
                else {
                    value_oa << result_local;
                    upcxx::rpc_ff(
                        parent,
                        [](upcxx::dist_object< std::tuple<std::int32_t, std::int32_t, std::string, std::string> > & args_, std::string data_) {
                            std::get<2>(*args_).append(data_);
                            atomic_xchange( &std::get<0>(*args_), 0, 1 );
                        }, args, value_buffer.str()
                    );
                }
            }
            else {

                if(cas_count > 1) {
                    upcxx::progress();
                    while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                        upcxx::progress();
                    }

                    upcxx::progress();
                    while(!atomic_xchange( &std::get<1>(*args), 1, 0 )) {
                        upcxx::progress();
                    }

                    std::vector<value_type_t> recv_vec{};
                    {
                        value_type_t odd{};
                        std::stringstream value_buffer_odd{ std::get<2>(*args) };
                        boost::archive::binary_iarchive iarch_odd{value_buffer_odd};
                        iarch_odd >> odd;
                        recv_vec.push_back(std::move(odd));
                    }

                    {
                        value_type_t even{};
                        std::stringstream value_buffer_even{ std::get<3>(*args) };
                        boost::archive::binary_iarchive iarch_even{value_buffer_even};
                        iarch_even >> even;
                        recv_vec.push_back(std::move(even));
                    }

                    value_type_t result_join = std::reduce(recv_vec.begin(), recv_vec.end(), result_local, op); 
                    value_oa << result_join;
                }
                else {
                    upcxx::progress();
                    while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                        upcxx::progress();
                    }

                    value_type_t odd{};

                    {
                        std::stringstream value_buffer_odd{ std::get<2>(*args) };
                        boost::archive::binary_iarchive iarch_odd{value_buffer_odd};
                        iarch_odd >> odd;
                    }

                    value_type_t result_join = op(result_local, odd);
                    value_oa << result_join;
                }

                if(is_even) {
                    upcxx::rpc_ff(
                        parent,
                        [](upcxx::dist_object< std::tuple<std::int32_t, std::int32_t, std::string, std::string> > & args_, std::string data_) {
                            std::get<3>(*args_).append(data_);
                            atomic_xchange( &std::get<1>(*args_), 0, 1 );
                        }, args, value_buffer.str()
                    );
                }
                else {
                    upcxx::rpc_ff(
                        parent,
                        [](upcxx::dist_object< std::tuple<std::int32_t, std::int32_t, std::string, std::string> > & args_, std::string data_) {
                            std::get<2>(*args_).append(data_);
                            atomic_xchange( &std::get<0>(*args_), 0, 1 );
                        }, args, value_buffer.str()
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
