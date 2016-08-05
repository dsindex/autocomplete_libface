INCDIR=     ../include
TESTDIR=    ../tests
COPT=       -DRMQ=BenderRMQ -DNMAX=100
#COPT=       -DRMQ=SparseTable -DNMAX=40
CXXFLAGS=   -Wall $(COPT) -D_FILE_OFFSET_BITS=64 -fPIC
LINKFLAGS=	-Wl,-rpath -Wl,. -lm -lrt -pthread
INCDEPS=    $(INCDIR)/segtree.hpp $(INCDIR)/sparsetable.hpp $(INCDIR)/benderrmq.hpp \
            $(INCDIR)/phrase_map.hpp $(INCDIR)/suggest.hpp $(INCDIR)/types.hpp \
            $(INCDIR)/utils.hpp $(INCDIR)/libface.hpp
INC=        -I$(INCDIR) -I.
LDIR=       -L.

LIBNAME=    liblibface.a
LIBNAME_SO= liblibface.so
SRCS=       libface.cpp
OBJS=       $(SRCS:.cpp=.o)

all: CXXFLAGS += -O2
all: targets

debug: CXXFLAGS += -g -DDEBUG
debug: targets

test: CXXFLAGS += -g -DDEBUG
perf: CXXFLAGS += -O2

targets: $(LIBNAME) $(LIBNAME_SO) test_libface

.cpp.o:$(OBJS) 
	$(CXX) $(CXXFLAGS) -c $(INC) $*.cpp

$(LIBNAME) : $(OBJS)
	ar rcs $(LIBNAME) $(OBJS)

$(LIBNAME_SO) : $(OBJS)
	$(CXX) $(CXXFLAGS) -shared -Wl,-soname,$(LIBNAME_SO) -g -o $(LIBNAME_SO) $(OBJS) $(INC)

test_libface: test_libface.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INC) $(LDIR) -llibface $(LINKFLAGS)

test:
	$(CXX) -o $(TESTDIR)/containers $(TESTDIR)/containers.cpp $(INC) $(CXXFLAGS)
	$(TESTDIR)/containers

perf:
	$(CXX) -o $(TESTDIR)/rmq_perf $(TESTDIR)/rmq_perf.cpp $(INC) $(CXXFLAGS)
	$(TESTDIR)/rmq_perf

clean:
	rm -f $(LIBNAME) $(LIBNAME_SO) *.o test_libface $(TESTDIR)/containers $(TESTDIR)/rmq_perf
