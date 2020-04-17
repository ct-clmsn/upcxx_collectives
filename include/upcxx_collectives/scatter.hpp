//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_SCATTER_HPP__
#define __UPCXX_SCATTER_HPP__

#include "collective_traits.hpp" 

namespace upcxx { namespace utils { namespace collectives {

template< typename CommunicationPattern, typename BlockingPolicy >
class scatter {

public:
    using communication_pattern = CommunicationPattern;
    using blocking_policy = BlockingPolicy;

    scatter(const std::int64_t root_=0);

    template<typename InputIterator, typename OutputIterator>
    void operator()(InputIterator input_beg, InputIterator input_end, OutputIterator out_beg);

};

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
