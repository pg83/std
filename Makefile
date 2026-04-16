HDRS = $(wildcard std/*/*.h) Makefile dev/build.sh dev/run.sh
SRCS = $(wildcard std/*/*.cpp)
TMPS = $(subst _ut.cpp,_ut.u,$(SRCS))

LIBS = $(filter %.cpp,$(TMPS))
LIBO = $(LIBS:%=%.o)
LIBA = std/libstd.a

UTSS = $(subst _ut.u,_ut.cpp,$(filter %.u,$(TMPS)))
UTSO = $(UTSS:%=%.o)

TSTS = $(wildcard tst/*.cpp)
TSTO = $(TSTS:%=%.o)
TSTA = tst/test

BINS = $(filter-out tst/test,$(TSTS:%.cpp=%))

OPTF = -O2 -g -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer
ifneq ($(filter x86_64%,$(shell $(CXX) -dumpmachine)),)
OPTF += -mcx16 -Werror
endif
CXXF = -I. -W -Wall -std=c++26 $(OPTF) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $(EXTRA)

# -Dnoexcept=

all: $(LIBA) $(TSTA) $(BINS)

$(LIBA): $(LIBO)
	rm -rf $(LIBA)
	llvm-ar q $(LIBA) $(LIBO)

$(TSTA): tst/test.cpp.o $(UTSO) $(LIBA)
	$(CXX) $(OPTF) $(LDFLAGS) -o $@ tst/test.cpp.o $(UTSO) $(LIBA)

$(BINS): %: %.cpp.o $(LIBA)
	$(CXX) $(OPTF) $(LDFLAGS) -o $@ $< $(LIBA)

%.cpp.o: %.cpp $(HDRS)
	$(CXX) $(CXXF) -o $@ -c $<

install: $(LIBA)
	find std -name '*.h' -exec install -Dm644 {} $(DESTDIR)/include/{} \;
	install -Dm644 $(LIBA) $(DESTDIR)/lib/libstd.a

clear:
	rm -rf $(LIBA) $(TSTA) $(BINS) $(TSTO) $(UTSO) $(LIBO)
