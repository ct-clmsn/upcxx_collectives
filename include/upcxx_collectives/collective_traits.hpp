//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef __UPCXX_COLLECTIVE_TRAITS_HPP__
#define __UPCXX_COLLECTIVE_TRATIS_HPP__

#include <type_traits>

namespace upcxx { namespace utils { namespace collectives {

struct blocking {};
struct nonblocking {};

template<typename Blocking>
struct is_blocking : public std::false_type {
};

template<>
struct is_blocking<blocking> : public std::true_type {
};

struct tree_binomial {};
struct tree_binary {};

template<typename Blocking>
struct is_tree_binomial : public std::false_type {
};

template<>
struct is_tree_binomial<tree_binomial> : public std::true_type {
};

template<typename CommunicationPattern>
struct is_tree_binary : public std::false_type {
};

template<>
struct is_tree_binary<tree_binomial> : public std::true_type {
};

struct topology_ring {};
struct topology_mesh {};
struct topology_hypercube {};

template<typename CommunicationPattern>
struct is_topology_ring : public std::false_type {
};

template<>
struct is_topology_ring<topology_ring> : public std::true_type {
};

template<typename CommunicationPattern>
struct is_topology_mesh : public std::false_type {
};

template<>
struct is_topology_mesh<topology_mesh> : public std::true_type {
};

template<typename CommunicationPattern>
struct is_topology_hypercube : public std::false_type {
};

template<>
struct is_topology_hypercube<topology_hypercube> : public std::true_type {
};



} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
