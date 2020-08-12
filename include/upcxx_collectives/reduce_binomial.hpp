//  Copyright (c) 2020 Christopher Tayloa
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_REDUCE_BINOMIAL_HPP__
#define __UPCXX_REDUCE_BINOMIAL_HPP__

#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <unistd.h>

//#include <boost/archive/binary_oarchive.hpp>
//#include <boost/archive/binary_iarchive.hpp>
#include <upcxx/upcxx.hpp>

#include "collective_traits.hpp" 
#include "reduce.hpp"
#include "serialization.hpp"

namespace upcxx { namespace utils { namespace collectives {

template< typename BlockingPolicy, typename Serialization >
class reduce<tree_binomial, BlockingPolicy, Serialization> {

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

    reduce(const std::int64_t root_=upcxx::rank_me()) :
        root(root_),
        mask(0x1),
        args{std::make_tuple(0, std::vector<std::string>{})} {
    }

    template<typename InputIterator, typename BinaryOp>
    void operator()(InputIterator input_beg, InputIterator input_end, typename std::iterator_traits<InputIterator>::value_type init, BinaryOp op, typename std::iterator_traits<InputIterator>::value_type & output) {
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
        const std::int64_t rank_me = (upcxx::rank_me() - root) % upcxx::rank_n();

        value_type local_result{std::reduce(input_beg, input_end, init, op)};

        for(std::int64_t i = 0; i < logp; ++i) {
            if((mask & rank_me) == 0) {
                if((rank_me | mask) < rank_n) {
                    // recv this atomic flips back and forth
                    //
                    upcxx::progress();

                    while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
                        upcxx::progress();
                    }

                    local_result = std::transform_reduce(
                        std::get<1>(*args).begin(), std::get<1>(*args).end(),
                        local_result, op,
                        [](auto & element) {
                            value_type val{};
                            //std::stringstream value_buffer{element };
                            //boost::archive::binary_iarchive iarch{value_buffer};
                            value_type_t value_buffer{element };
                            deserializer_t iarch{value_buffer};
                            iarch >> val;
                            return std::move(val);
                    });

                    std::get<1>(*args).erase(
                        std::get<1>(*args).begin(), std::get<1>(*args).end()
                    );
                }
            }
            else {
                // leaf-parent exchange send
                //
                const std::int64_t parent = (rank_me & (~mask)); // % rank_n;

                //std::stringstream value_buffer{};
                //boost::archive::binary_oarchive value_oa{value_buffer};
                value_type_t value_buffer{};
                serializer_t value_oa{value_buffer};
                value_oa << local_result;

                upcxx::rpc_ff(
                    parent,
                    [](upcxx::dist_object< std::tuple< std::int32_t, std::vector<std::string> > > & args_, std::string data_) {
                        std::get<1>(*args_).reserve(std::get<1>(*args_).size() + 1);
                        std::get<1>(*args_).push_back(data_);
                        atomic_xchange( &std::get<0>(*args_), 0, 1 );
                    }, args, value_buffer.str()
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
            output = local_result;
        }

        if constexpr(is_blocking<BlockingPolicy>()) {
            upcxx::barrier(); // make sure communications terminate properly
        }

    } // end operator()

};

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
