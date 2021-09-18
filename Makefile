HDRS = \
    $(wildcard std/sys/*.h) \
    $(wildcard std/ios/*.h) \
    $(wildcard std/lib/*.h) \
    $(wildcard std/str/*.h) \
    $(wildcard std/alg/*.h) \
    $(wildcard std/mem/*.h)

LIBS = \
    $(wildcard std/sys/*.cpp) \
    $(wildcard std/ios/*.cpp) \
    $(wildcard std/lib/*.cpp) \
    $(wildcard std/str/*.cpp) \
    $(wildcard std/alg/*.cpp) \
    $(wildcard std/mem/*.cpp)

LIBO = $(LIBS:%=%.o)

TSTS = $(wildcard tst/*.cpp)
TSTO = $(TSTS:%=%.o)

CXXF = -I. -W -Wall -O2 -std=c++2a $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS)

all: libstd.a test

libstd.a: $(LIBO) Makefile
	-rm libstd.a
	ar q libstd.a $(LIBO)
	ranlib libstd.a

std/alg/%.cpp.o: std/alg/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

std/lib/%.cpp.o: std/lib/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

std/str/%.cpp.o: std/str/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

std/ios/%.cpp.o: std/ios/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

std/sys/%.cpp.o: std/sys/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

std/mem/%.cpp.o: std/mem/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<

test: $(TSTO) libstd.a Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(LDFLAGS) -o $@ $(TSTO) libstd.a

tst/%.cpp.o: tst/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<
