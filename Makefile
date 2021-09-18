HDRS = $(wildcard std/*/*.h)
SRCS = $(wildcard std/*/*.cpp)
TMPS = $(subst _ut.cpp,_ut.u,$(SRCS))

LIBS = $(filter %.cpp,$(TMPS))
LIBO = $(LIBS:%=%.o)

TSTS = $(wildcard tst/*.cpp) $(subst _ut.u,_ut.cpp,$(filter %.u,$(TMPS)))
TSTO = $(TSTS:%=%.o)

CXXF = -isystem . -W -Wall -O2 -std=c++2a $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS)

all: libstd.a test

libstd.a: $(LIBO) Makefile
	@-rm libstd.a 2>/dev/null
	ar q libstd.a $(LIBO)
	ranlib libstd.a

test: $(TSTO) libstd.a Makefile
	@-mkdir -p `dirname $@`
	$(CXX) $(LDFLAGS) -o $@ $(TSTO) libstd.a

%.cpp.o: %.cpp $(HDRS) Makefile
	@-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

clear:
	(echo test; echo libstd.a; (find . | grep '\.o')) | xargs rm
