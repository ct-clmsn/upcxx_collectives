#pragma once
#ifndef __UPCXX_COLLECTIVES_SERIALIZER_BOOST__
#define __UPCXX_COLLECTIVES_SERIALIZER_BOOST__

#include <type_traits>
#include <sstream>

#ifdef BOOST
    #include <boost/archive/binary_oarchive.hpp>
    #include <boost/archive/binary_iarchive.hpp>
#endif

namespace upcxx { namespace utils { namespace collectives { namespace serialization {

#ifdef BOOST

struct boost {
    using value_type = std::stringstream;
    using serializer = ::boost::archive::binary_oarchive;
    using deserializer = ::boost::archive::binary_iarchive;

    static std::string get_buffer(const value_type & vt) {
        return vt.str();
    }
};

#else

struct boost {};

#endif

template<typename Tag>
struct is_boost : public std::false_type {
};

template<>
struct is_boost<upcxx::utils::collectives::serialization::boost> : public std::true_type {
};

} } } } // end namespaces

#endif
