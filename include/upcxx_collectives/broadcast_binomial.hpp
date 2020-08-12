//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_BROADCAST_BINOMIAL_HPP__
#define __UPCXX_BROADCAST_BINOMIAL_HPP__

#include <unistd.h>
#include <string>
#include <sstream>

//#include <boost/archive/binary_oarchive.hpp>
//#include <boost/archive/binary_iarchive.hpp>
#include <upcxx/upcxx.hpp>

#include "collective_traits.hpp" 
#include "serialization.hpp"
#include "broadcast.hpp"
#include "utils.hpp"

namespace upcxx { namespace utils { namespace collectives {

template< typename BlockingPolicy, typename Serialization >
class broadcast< tree_binomial, BlockingPolicy, Serialization > {

    using value_type_t = typename Serialization::value_type;
    using serializer_t = typename Serialization::serializer;
    using deserializer_t = typename Serialization::deserializer;

private:
    const std::int64_t root;
    upcxx::dist_object< std::tuple<std::int32_t, std::string> > args;

public:
    using communication_pattern = upcxx::utils::collectives::tree_binomial;
    using blocking_policy = BlockingPolicy;

    broadcast(const std::int64_t root_=0) :
        root(root_),
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
        const std::int64_t rank_n = upcxx::rank_n();
        const std::int64_t rank_me = (upcxx::rank_me() - root) % rank_n;

        for(std::int64_t i = 0; i < logp; ++i) {

            const auto twok = 2 * k;
            if( (rank_me % twok) == 0 ) {

                //std::stringstream send_buffer{std::get<1>(*args)};
                //boost::archive::binary_oarchive send_ia{send_buffer};
                value_type_t send_buffer{std::get<1>(*args)};
                serializer_t send_ia{send_buffer};
                send_ia << data;

                upcxx::rpc_ff(
                    (rank_me + k),
                    [](upcxx::dist_object< std::tuple<std::int32_t, std::string> > & args_, std::string serialized_value) {
                        std::get<1>(*args_).resize(serialized_value.size());
                        std::get<1>(*args_).insert(0, serialized_value);
                        atomic_xchange( &std::get<0>(*args_), 0, 1 );
                    }, args, Serialization::get_buffer(send_buffer)
                );
            }
            else if( not_recieved && ((rank_me % twok) == k) ) {
                upcxx::progress();
                while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                    upcxx::progress();
                }

                //std::stringstream recv_buffer{std::get<1>(*args)};
                //boost::archive::binary_iarchive recv_ia{recv_buffer};
                value_type_t recv_buffer{std::get<1>(*args)};
                deserializer_t recv_ia{recv_buffer};

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
