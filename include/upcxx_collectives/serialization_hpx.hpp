#pragma once
#ifndef __UPCXX_COLLECTIVES_SERIALIZER_HPX__
#define __UPCXX_COLLECTIVES_SERIALIZER_HPX__

#include <type_traits>
#include <string>

#ifdef HPX
    #include <hpx/config/defines.hpp>
    #include <hpx/serialization/serialize.hpp>
#endif

namespace upcxx { namespace utils { namespace collectives { namespace serialization {

#ifdef HPX

struct hpx {
    using value_type = std::string;
    using serializer = ::hpx::serialization::output_archive;
    using deserializer = ::hpx::serialization::input_archive;

    static std::string get_buffer(const value_type & vt) {
        return vt;
    }
};

#else

struct hpx{};

#endif

template<typename Tag>
struct is_hpx : public std::false_type {
};

template<>
struct is_hpx<upcxx::utils::collectives::serialization::hpx> : public std::true_type {
};

} } } } // end namespaces

#endif
