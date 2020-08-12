//  Copyright (c) 2020 Christopher Tayloa
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_REDUCE_HPP__
#define __UPCXX_REDUCE_HPP__

#include "collective_traits.hpp" 

namespace upcxx { namespace utils { namespace collectives {

template< typename CommunicationPattern, typename BlockingPolicy, typename Serialization >
class reduce {

public:
    using communication_pattern = CommunicationPattern;
    using blocking_policy = BlockingPolicy;

    reduce(const std::int64_t root_=0);

    template<typename InputIterator, typename BinaryOp>
    void operator()(InputIterator input_beg, InputIterator input_end, typename std::iterator_traits<InputIterator>::value_type init, BinaryOp op, typename std::iterator_traits<InputIterator>::value_type & output);

};

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
