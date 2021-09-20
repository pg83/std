HDRS = $(wildcard std/*/*.h)
SRCS = $(wildcard std/*/*.cpp)
TMPS = $(subst _ut.cpp,_ut.u,$(SRCS))

LIBS = $(filter %.cpp,$(TMPS))
LIBO = $(LIBS:%=%.o)
LIBA = std/libstd.a

TSTS = $(wildcard tst/*.cpp) $(subst _ut.u,_ut.cpp,$(filter %.u,$(TMPS)))
TSTO = $(TSTS:%=%.o)
TSTA = tst/test

OPTF = -O3 -flto -fdata-sections -ffunction-sections -fcommon
CXXF = -I. -W -Wall -std=c++2a $(OPTF) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS)

all: $(LIBA) $(TSTA)

$(LIBA): $(LIBO) Makefile
	@-rm $(LIBA) 2>/dev/null
	ar q $(LIBA) $(LIBO)
	ranlib $(LIBA)

$(TSTA): $(TSTO) $(LIBA) Makefile
	$(CXX) $(OPTF) $(LDFLAGS) -o $@ $(TSTO) $(LIBA)

%.cpp.o: %.cpp $(HDRS) Makefile
	$(CXX) $(CXXF) -o $@ -c $<

clear:
	rm -rf $(LIBA) $(TSTA) $(TSTO) $(LIBO)
