HDRS = \
    $(wildcard std/tl/*.h) \
    $(wildcard std/os/*.h) \
    $(wildcard std/io/*.h)

LIBS = \
    $(wildcard std/tl/*.cpp) \
    $(wildcard std/os/*.cpp) \
    $(wildcard std/io/*.cpp)

LIBO = $(LIBS:%=%.o)

TSTS = $(wildcard tst/*.cpp)
TSTO = $(TSTS:%=%.o)

CXXF = -I. -std=c++2a $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS)

all: libstd.a test

libstd.a: $(LIBO) Makefile
	-rm libstd.a
	ar q libstd.a $(LIBO)
	ranlib libstd.a

std/tl/%.cpp.o: std/tl/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

std/io/%.cpp.o: std/io/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

std/os/%.cpp.o: std/os/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

test: $(TSTO) libstd.a Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(LDFLAGS) -o $@ $(TSTO) libstd.a

tst/%.cpp.o: tst/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<
 