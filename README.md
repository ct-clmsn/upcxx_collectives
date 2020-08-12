<!-- Copyright (c) 2020 Christopher Taylor                                          -->
<!--                                                                                -->
<!--   Distributed under the Boost Software License, Version 1.0. (See accompanying -->
<!--   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)        -->

# Collective Communication Primitives for UPC++

Project details [here](http://www.github.com/ct-clmsn/upcxx_collectives/).

This project implements collective operations for UPC++ using single-sided
communications. The project is 'header-only'. Serialization routines are
third-party provided and will require a user application to link to a
library and a compile-time-flag to tell the header files which serializer
to utilize.

### License

Boost Software License

### Collectives Implemented

* Broadcast
* Scatter
* Gather
* Reduce

### Communication Patterns

* binomial tree
* binary tree

### Dependencies

* UPC++
* Boost Serialization or STE||AR HPX Serialization

#### Notes

The implementations specifically targets log2 nodes and data set sizes.
Any C++ container type providing iterator support can be used with this
library.

The implementation depends on using the following datatype to control
communications:

~~~
upcxx::dist_object< std::tuple< std::int32_t , std::string > > args;
~~~

args is exposed to the global address space. std::get<0>(args) returns
a std::int32_t that is used as a mutex for a spinlock. The spinlock
code looks like this:

~~~
upcxx::progress();
while(!atomic_xchange( &std::get<0>(*args), 1, 0 )) {
    upcxx::progress();
}
~~~

the calls to upcxx::progress() clear the active message queues and allow
remotely invoked asynchronous functions to execute. std::get<1>(args)
returns the communication data buffer associated with the mutex.

The communication data buffer is a std::string which is stored in the
globally addressable tuple. The data buffer contains serialized data
structures that are 'shipped' across the network with a remotely invoked
active message that fills the buffer and flips the mutex.

Data types should be compatible with Boost or STE||AR HPX Serialization
requirements.

Users can select which PE is the 'root' process for communication ('root'
process for the tree communication does not have to be `rank 0`).

To use the library, create a 'communicator pattern' on each PE, set the
root level rank for communication when you create the 'communicator pattern',
and you should be all set.

To install, recursively copy the `./include/upcxx_collectives` into your
project or your system installation path, usually this is some place like
`../include/`.

To compile, read the examples directory for guidance. Switching between
serializers is as simple as uncommenting the HPX block and commenting the
BOOST block in the makefile.

### TODO
* All2All
* hypercube

### Author
Christopher Taylor

### Dependencies

* [UPC++](https://bitbucket.org/berkeleylab/upcxx/wiki/Home)
* [Boost](https://github.com/boostorg/boost)
* [STE||AR HPX](https://github.com/STEllAR-GROUP/hpx)
