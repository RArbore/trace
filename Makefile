RELEASE ?= 0
ifeq ($(RELEASE), 1)
	CPPFLAGS := -c -Ofast -flto -DRELEASE
	LDFLAGS := -fuse-ld=mold -flto
else
	CPPFLAGS := -c -g
	LDFLAGS := -fuse-ld=mold
endif

CXX := g++
LD := g++
RM := rm -f
WFLAGS := -Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror
LDLIBS := -lvulkan -lglfw

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
