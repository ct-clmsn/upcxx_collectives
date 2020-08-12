#  Copyright (c) 2020 Christopher Tayloa
#
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
INSTALL_ROOT=$(HOME)/git/install

BOOST_INSTALL_ROOT=$(INSTALL_ROOT)
BOOST_INC_DIR=$(BOOST_INSTALL_ROOT)/include
BOOST_LIB_DIR=$(BOOST_INSTALL_ROOT)/lib
BOOST_LIB=-lboost_serialization

HPX_INSTALL_ROOT=$(INSTALL_ROOT)
HPX_INC_DIR=$(HPX_INSTALL_ROOT)/include
HPX_LIB_DIR=$(HPX_INSTALL_ROOT)/lib
HPX_LIB=-latomic -lhpx_logging -lhpx_errors -lhpx_io_service -lhpx_serialization -lhpx

CC=upcxx

SERIALIZER=-DBOOST
LIB_DIR=$(BOOST_LIB_DIR)
INC_DIR=$(BOOST_INC_DIR)
LIBS=$(BOOST_LIB)

#SERIALIZER=-DHPX
#LIB_DIR=$(HPX_LIB_DIR)
#INC_DIR=$(HPX_INC_DIR)
#LIBS=$(HPX_LIB)

CFLAGS=-codemode=O3 -network=smp -threadmode=par -std=c++17 -DNDEBUG $(SERIALIZER) -mavx
#-static

all:
	$(CC) test.cpp $(CFLAGS) -o test -I$(INC_DIR) -L$(LIB_DIR) $(LIBS)

clean:
	rm test
