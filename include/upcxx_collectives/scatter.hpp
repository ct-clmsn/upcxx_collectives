//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_SCATTER_HPP__
#define __UPCXX_SCATTER_HPP__

#include <string>
#include <sstream>
#include <iterator>
#include <unistd.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <upcxx/upcxx.hpp>

#include "collective_traits.hpp" 

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
class scatter {

private:
    std::int64_t root;
    upcxx::dist_object< std::tuple<std::int32_t, std::string> > args;

    static inline bool compare_and_swap(volatile int * cond, int old_, int new_) {
        if( (*cond) != old_ ) { return false; }
        (*cond) = new_;
        return true;
    }

    template <typename Type1, typename Type2>
    static inline Type1 ipow(Type1 a, Type2 ex)
    {
        // Return a**ex
        //
        if ( 0==ex )  return 1;
        else
        {
            Type1 z = a;
            Type1 y = 1;
            while ( 1 )
            {
                if ( ex & 1 )  y *= z;
                ex /= 2;
                if ( 0==ex )  break;
                z *= z;
            }
            return y;
        }
    }

    static inline int backoff(const int attempt, const int base, const int cap) {
        return std::min(cap, ipow(base * 2, 2));
    }

public:
    scatter(const std::int64_t root_=upcxx::rank_me()) :
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
                std::stringstream value_buffer{};
                if(std::get<1>(*args).size() < 1) {

                    boost::archive::binary_oarchive value_oa{value_buffer};

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
                    value_buffer << std::get<1>(*args);
                    //std::get<1>(*args).clear();
                }

                upcxx::rpc_ff(
                    (rank_me + k),
                    [](upcxx::dist_object< std::tuple<std::int32_t, std::string> > & args_, std::string data_) {
                        std::get<1>(*args_).resize(data_.size());
                        std::get<1>(*args_).insert(0, data_);

                        atomic_xchange( &std::get<0>(*args_), 0, 1 );
                    }, args, value_buffer.str()
                );
            }
            else if( not_recieved && ((rank_me % twok) == k) ) {

                upcxx::progress();
                while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                std::stringstream recv_buffer{std::get<1>(*args)};
                boost::archive::binary_iarchive recv_ia{recv_buffer};

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
