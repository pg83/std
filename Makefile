HDRS = $(wildcard std/*/*.h)
SRCS = $(wildcard std/*/*.cpp)
TMPS = $(subst _ut.cpp,_ut.u,$(SRCS))

LIBS = $(filter %.cpp,$(TMPS))
LIBO = $(LIBS:%=%.o)
LIBA = std/libstd.a

TSTS = $(wildcard tst/*.cpp) $(subst _ut.u,_ut.cpp,$(filter %.u,$(TMPS)))
TSTO = $(TSTS:%=%.o)
TSTA = tst/test

OPTF = -O2 -g
CXXF = -I. -W -Wall -std=c++2a $(OPTF) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $(EXTRA)

all: $(LIBA) $(TSTA)

$(LIBA): $(LIBO) Makefile
	rm -rf $(LIBA)
	llvm-ar q $(LIBA) $(LIBO)

$(TSTA): $(TSTO) $(LIBA) Makefile
	$(CXX) -fuse-ld=lld $(OPTF) $(LDFLAGS) -o $@ $(TSTO) $(LIBA)

%.cpp.o: %.cpp $(HDRS) Makefile
	$(CXX) $(CXXF) -o $@ -c $<

clear:
	rm -rf $(LIBA) $(TSTA) $(TSTO) $(LIBO)
