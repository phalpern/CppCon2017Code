#            Copyright 2009 Pablo Halpern.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

TESTARGS +=

CXX=g++ -std=c++11
CXXFLAGS=-I. -Wall
WD := $(shell basename $(PWD))

all : polymorphic_allocator.test test_resource.test

.SECONDARY :

.FORCE :

.PHONY : .FORCE clean

.PRECIOUS : .t

%.test : %.t
	./$< $(TESTARGS)

%.t : %.t.o %.o
	$(CXX) $(CXXFLAGS) -o $@ -g $^

%.o : %.cpp %.h polymorphic_allocator.h
	$(CXX) $(CXXFLAGS) -c -g $<

%.t.o : %.t.cpp %.h polymorphic_allocator.h
	$(CXX) $(CXXFLAGS) -c -g $<

test_resource.o :: pmr_vector.h

test_resource.t :: polymorphic_allocator.o

slist.t :: polymorphic_allocator.o test_resource.o

slist.t.o :: test_resource.h pmr_string.h

clean :
	rm -f *.t *.o
