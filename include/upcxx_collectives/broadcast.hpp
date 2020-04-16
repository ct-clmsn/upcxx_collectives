//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_BROADCAST_HPP__
#define __UPCXX_BROADCAST_HPP__

#include "collective_traits.hpp" 

namespace upcxx { namespace utils { namespace collectives {

template< typename CommunicationPattern, typename BlockingPolicy >
class broadcast {

public:
    using communication_pattern = CommunicationPattern;
    using blocking_policy = BlockingPolicy;

    broadcast(const std::int64_t root=0);

    template<typename DataType>
    void operator()(DataType & data);

};

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
