//  Copyright (c) 2020 Christopher Taylor
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//   
#pragma once
#ifndef __UPCXX_COLLECTIVES_HPP__
#define __UPCXX_COLLECTIVES_HPP__

#include "collective_traits.hpp"
#include "broadcast.hpp"
#include "broadcast_binomial.hpp"
#include "broadcast_binary.hpp"
#include "scatter.hpp"
#include "scatter_binomial.hpp"
#include "gather.hpp"
#include "gather_binary.hpp"
#include "gather_binomial.hpp"
#include "reduce.hpp"
#include "reduce_binary.hpp"
#include "reduce_binomial.hpp"

namespace upcxx { namespace utils { namespace collectives {

template<typename Operation>
class scalar_collective {

private:
    const std::int64_t root;

public:
    scalar_collective(const std::int64_t root_=0) :
        root(root_) {
    }

    template<typename DataType>
    void operator()(DataType & data) {
        Operation op{root};
        op(data);
    }
};

template<typename Operation>
class iterable_collective {

private:
    const std::int64_t root;

public:
    iterable_collective(const std::int64_t root_=0) :
        root(root_) {
    }

    template<typename InputIter, typename OutputIter>
    void operator()(InputIter input_beg, InputIter input_end, OutputIter output_beg) {
        Operation op{root};
        op(input_beg, input_end, output_beg);
    }

    template<typename InputIterator, typename BinaryOp>
    void operator()(InputIterator input_beg, InputIterator input_end, typename std::iterator_traits<InputIterator>::value_type init, BinaryOp op, typename std::iterator_traits<InputIterator>::value_type & output) {
        Operation opr{};
        opr(input_beg, input_end, init, op, output);
    }

/*
    template<class ExecutionPolicy, typename InputIterator, typename BinaryOp>
    void operator()(ExecutionPolicy && policy, InputIterator input_beg, InputIterator input_end, typename std::iterator_traits<InputIterator>::value_type init, BinaryOp op, typename std::iterator_traits<InputIterator>::value_type & output) {
        Operation opr{};
        opr(input_beg, input_end, init, op, output);
    }
*/

};

// broadcast
//
using nonblocking_binomial_broadcast = upcxx::utils::collectives::scalar_collective<upcxx::utils::collectives::broadcast<upcxx::utils::collectives::tree_binomial, upcxx::utils::collectives::nonblocking>>;
using blocking_binomial_broadcast = upcxx::utils::collectives::scalar_collective<upcxx::utils::collectives::broadcast<upcxx::utils::collectives::tree_binomial, upcxx::utils::collectives::blocking>>;

using nonblocking_binary_broadcast = upcxx::utils::collectives::scalar_collective<upcxx::utils::collectives::broadcast<upcxx::utils::collectives::tree_binary, upcxx::utils::collectives::nonblocking>>;
using blocking_binary_broadcast = upcxx::utils::collectives::scalar_collective<upcxx::utils::collectives::broadcast<upcxx::utils::collectives::tree_binary, upcxx::utils::collectives::blocking>>;

// scatter
//
using nonblocking_binomial_scatter = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::scatter<upcxx::utils::collectives::tree_binomial, upcxx::utils::collectives::nonblocking>>;
using blocking_binomial_scatter = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::scatter<upcxx::utils::collectives::tree_binomial, upcxx::utils::collectives::blocking>>;

// gather
//
using nonblocking_binary_gather = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::gather<upcxx::utils::collectives::tree_binary, upcxx::utils::collectives::nonblocking>>;
using blocking_binary_gather = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::gather<upcxx::utils::collectives::tree_binary, upcxx::utils::collectives::blocking>>;

using nonblocking_binomial_gather = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::gather<upcxx::utils::collectives::tree_binomial, upcxx::utils::collectives::nonblocking>>;
using blocking_binomial_gather = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::gather<upcxx::utils::collectives::tree_binomial, upcxx::utils::collectives::blocking>>;

// reduce
//
using nonblocking_binary_reduce = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::reduce<upcxx::utils::collectives::tree_binary, upcxx::utils::collectives::nonblocking>>;
using blocking_binary_reduce = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::reduce<upcxx::utils::collectives::tree_binary, upcxx::utils::collectives::blocking>>;

using nonblocking_binomial_reduce = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::reduce<upcxx::utils::collectives::tree_binomial, upcxx::utils::collectives::nonblocking>>;
using blocking_binomial_reduce = upcxx::utils::collectives::iterable_collective<upcxx::utils::collectives::reduce<upcxx::utils::collectives::tree_binomial, upcxx::utils::collectives::blocking>>;

} /* end namespace collectives */ } /* end namespace utils */ } /* end namespace upcxx */

#endif
