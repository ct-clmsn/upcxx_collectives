//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_COLLECTIVES_UTILS_H__
#define __UPCXX_COLLECTIVES_UTILS_H__

namespace upcxx { namespace utils { namespace collectives { namespace utils {

#define STRONG 0
#define WEAK 1

template<typename T>
static inline bool atomic_cmp_ex(T * ptr, T expected, T desired) {
    T ex = expected;
    T de = desired;
    return __atomic_compare_exchange(ptr, &expected, &desired, STRONG, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
}

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
    else                                                                                                                                                                                                               {                                                                                                                                                                                                                      Type1 z = a;
        Type1 y = 1;
        while ( 1 )                                                                                                                                                                                                        {                                                                                                                                                                                                                      if ( ex & 1 )  y *= z;
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

} } } } // end namespace

#if defined(__CLANG_ATOMICS)
#define atomic_xchange(object, object_value, value) \
    __sync_swap(object, value)
#elif defined(__GNUC_ATOMICS)
#define atomic_xchange(object, object_value, value) \
    __sync_bool_compare_and_swap( object, object_value, value)
#else
//#elif __has_builtin(__sync_swap)
/* Clang provides a full-barrier atomic exchange - use it if available. */

#define atomic_xchange(object, object_value, value) \
    upcxx::utils::collectives::utils::atomic_cmp_ex(object, object_value, value)
//    __sync_swap(object, value)
#endif

#endif
