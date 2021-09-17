SRCD =
BLDD = bld/
SRCS = $(wildcard $(SRCD)std/tl/*.cpp)
HDRS = $(wildcard $(SRCD)std/io/*.h)
OBJS = $(SRCS:%=$(BLDD)%.o)
LIBA = $(BLDD)libstd.a
CXXF = $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS)

all: $(LIBA)

$(LIBA): $(OBJS) Makefile
	-rm $(LIBA)
	ar q $(LIBA) $(OBJS)
	ranlib $(LIBA)

$(BLDD)std/tl/%.cpp.o: $(SRCD)std/tl/%.cpp Makefile
	-mkdir -p `dirname $@`
	$(CC) $(CXXF) -c -o $@ $<
