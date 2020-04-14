BOOST_INSTALL_ROOT=$(HOME)/git/install
BOOST_INC=$(BOOST_INSTALL_ROOT)/include
BOOST_LIB=$(BOOST_INSTALL_ROOT)/lib
CC=upcxx
CFLAGS=-codemode=O3 -network=smp -threadmode=par -std=c++17 -stdlib=libc++ -DNDEBUG -mavx
INCLUDES=-I$(BOOST_INC) -I$(PWD)/../include/upcxx_collectives
LIBS=-L$(BOOST_LIB) -lboost_serialization -ltbb

all:
	$(CC) test.cpp $(CFLAGS) -o test $(INCLUDES) $(LIBS)

clean:
	rm test
