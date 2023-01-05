CXX := g++
LD := g++
RM := rm -f
WFLAGS := -Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror
LDLIBS := -lvulkan -lglfw

RELEASE ?= 0
ifeq ($(RELEASE), 1)
	CPPFLAGS := -Ofast -flto -DRELEASE
	LDFLAGS := -flto
else
	CPPFLAGS := -g
endif

CPPFLAGS := $(CPPFLAGS) -c -fno-rtti
LDFLAGS := $(LDFLAGS) -fuse-ld=mold

SRCS := $(shell find src -name "*.cc")
HEADERS := $(shell find src -name "*.h")
OBJS := $(subst src/, obj/, $(patsubst %.cc, %.o, $(SRCS)))

exe: trace
	./trace

trace: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o trace $(LDLIBS)

$(OBJS): obj/%.o: src/%.cc $(HEADERS)
	$(CXX) $(CPPFLAGS) $(WFLAGS) $< -o $@

clean:
	$(RM) obj/* trace

.PHONY: exe
