SRCD = 
BLDD = bld/

HDRS = $(wildcard $(SRCD)std/io/*.h)

LIBS = $(wildcard $(SRCD)std/tl/*.cpp)
LIBO = $(LIBS:%=$(BLDD)%.o)
LIBA = $(BLDD)libstd.a

TSTS = $(wildcard $(SRCD)tst/*.cpp)
TSTO = $(TSTS:%=$(BLDD)%.o)

CXXF = -I$(SRCD)./ $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS)

all: $(LIBA) $(BLDD)test

$(LIBA): $(LIBO) Makefile
	-rm $(LIBA)
	ar q $(LIBA) $(LIBO)
	ranlib $(LIBA)

$(BLDD)std/tl/%.cpp.o: $(SRCD)std/tl/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CC) $(CXXF) -o $@ -c $<

$(BLDD)test: $(TSTO) $(LIBA) Makefile
	-mkdir -p `dirname $@`
	$(CC) $(LDFLAGS) -o $@ $(TSTO) $(LIBA)

$(BLDD)tst/%.cpp.o: $(SRCD)tst/%.cpp $(HDRS) Makefile
	-mkdir -p `dirname $@`
	$(CC) $(CXXF) -o $@ -c $<
 