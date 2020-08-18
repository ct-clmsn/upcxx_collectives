//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_COLLECTIVES_SERIALIZER__
#define __UPCXX_COLLECTIVES_SERIALIZER__

#include <type_traits>
#include <string>

#include "serialization_hpx.hpp"
#include "serialization_boost.hpp"

namespace upcxx { namespace utils { namespace collectives { namespace serialization {

#ifdef HPX
    using backend = hpx;
#else
    using backend = boost;
#endif

} } } } // end namespaces

#endif
