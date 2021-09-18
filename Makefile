HDRS = $(wildcard std/*/*.h)

LIBS = $(wildcard std/*/*.cpp)
LIBO = $(LIBS:%=%.o)

TSTS = $(wildcard tst/*.cpp)
TSTO = $(TSTS:%=%.o)

CXXF = -I. -W -Wall -O2 -std=c++2a $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS)

all: libstd.a test

libstd.a: $(LIBO) Makefile
	-rm libstd.a
	ar q libstd.a $(LIBO)
	ranlib libstd.a

test: $(TSTO) libstd.a Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(LDFLAGS) -o $@ $(TSTO) libstd.a

%.cpp.o: %.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CXX) $(CXXF) -o $@ -c $<
