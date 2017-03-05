# Software modules to be built
MODULES := \
	PES-write \
	arib-write \
	buffer \
	data-group \
	timer

# Comment/uncoment for debug/release build
#CFLAGS := -std=c11 -Ofast -flto -DNDEBUG
CFLAGS := -std=c11 -Wall -Wextra -g

LIBS := -lm

CC = gcc

SRC := $(addsuffix .c, $(addprefix src/,$(MODULES)))
OBJS := $(addsuffix .o, $(addprefix build/,$(MODULES)))
DEPS := $(addsuffix .d, $(addprefix deps/,$(MODULES)))

.PHONY : clean flags

arib-write: $(OBJS) | build
	$(CC) -o arib-write $(CFLAGS) $(OBJS) $(LIBS)

-include $(DEPS)

build/%.o: src/%.c | build deps
	$(CC) -c $(CFLAGS) src/$*.c -o build/$*.o
	$(CC) -MM -MT build/$*.o $(CFLAGS) src/$*.c > deps/$*.d

build:
	mkdir build

deps:
	mkdir deps

flags:
	@echo $(CFLAGS)

clean:
	-rm -rf build deps arib-write
