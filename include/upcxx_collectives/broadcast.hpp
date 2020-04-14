//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_BROADCAST_HPP__
#define __UPCXX_BROADCAST_HPP__

#include <unistd.h>
#include <string>
#include <sstream>

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
class broadcast {

private:
    upcxx::dist_object< std::tuple<std::int32_t, std::string> > args;

    static inline bool compare_and_swap(volatile int * cond, int old_, int new_) {
        if( (*cond) != old_ ) { return false; }
        (*cond) = new_;
        return true;
    }

    template <typename Type1, typename Type2>
    static inline Type1 ipow(Type1 a, Type2 ex)
    // Return a**ex
    {
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
    broadcast() :
        args{std::make_tuple(0, std::string{})} {
    }

    template<typename DataType>
    void operator()(DataType & data) {
        // https://legacy.cs.indiana.edu/classes/b673-bram/Notes/mpi3.html
        //
        const std::int64_t logp =
            static_cast<std::int64_t>(
                std::log2(
                    static_cast<double>(
                        upcxx::rank_n()
                    )
                )
            );

        std::int64_t k = upcxx::rank_n() / 2;
        bool not_recieved = true;
        const std::int64_t rank_me = upcxx::rank_me();

        for(std::int64_t i = 0; i < logp; ++i) {

            const auto twok = 2 * k;
            if( (rank_me % twok) == 0 ) {

                std::stringstream send_buffer{std::get<1>(*args)};
                boost::archive::binary_oarchive send_ia{send_buffer};
                send_ia << data;

                upcxx::rpc_ff(
                    (rank_me + k),
                    [](upcxx::dist_object< std::tuple<std::int32_t, std::string> > & args_, std::string serialized_value) {
                        std::get<1>(*args_).resize(serialized_value.size());
                        std::get<1>(*args_).insert(0, serialized_value);
                        atomic_xchange( &std::get<0>(*args_), 0, 1 );
                    }, args, send_buffer.str()
                );
            }
            else if( not_recieved && ((rank_me % twok) == k) ) {
                upcxx::progress();
                while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                std::stringstream recv_buffer{std::get<1>(*args)};
                boost::archive::binary_iarchive recv_ia{recv_buffer};

                recv_ia >> data;

                not_recieved = false;
            }

            k /= 2;

        } // end for loop

        if constexpr(is_blocking<BlockingPolicy>()) {
            upcxx::barrier(); // make sure communications terminate properly
        }

    } // end operator()

};

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
