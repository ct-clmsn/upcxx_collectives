<!-- Copyright (c) 2020 Christopher Taylor                                          -->
<!--                                                                                -->
<!--   Distributed under the Boost Software License, Version 1.0. (See accompanying -->
<!--   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)        -->

# UPC++ Collectives: Collective Communication Primitives for UPC++

Project details [here](http://www.github.com/ct-clmsn/upcxx_collectives/).

This project implements collective operations for UPC++ using single-sided
communications. The project is 'header-only' and requires compilation and
linking of a binary library.

The implementations specifically target log2 nodes and data set sizes. Any C++
container type providing iterator support can be used with this library. Data
types should be compatible with Boost Serialization. Users can select which PE
is the 'root' process for communication.

### Collectives Implemented

* Broadcast
* Scatter
* Gather
* Reduce
* All2All

### Communication Patterns

* binomial tree
* binary tree
* hypercube

### Dependencies

* UPC++
* Boost Serialization
