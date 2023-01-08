CXX := g++
LD := g++
RM := rm -f
GLSL := glslc

RELEASE ?= 0
ifeq ($(RELEASE), 1)
	CPPFLAGS := -Ofast -flto -DRELEASE
	LDFLAGS := -flto
else
	CPPFLAGS := -g
endif

CPPFLAGS := $(CPPFLAGS) -c -fno-rtti
LDFLAGS := $(LDFLAGS) -fuse-ld=mold
WFLAGS := -Wall -Wextra -Wshadow -Wconversion -Wpedantic
LDLIBS := -lvulkan -lglfw

SRCS := $(shell find src -name "*.cc")
HEADERS := $(shell find src -name "*.h")
SHADERS := $(shell find shaders -name "*.glsl")
OBJS := $(subst src/, build/, $(patsubst %.cc, %.o, $(SRCS)))
SPIRVS := $(subst shaders/, build/, $(patsubst %.glsl, %.spv, $(SHADERS)))

trace: $(OBJS) $(SPIRVS)
	$(LD) $(LDFLAGS) $(OBJS) -o trace $(LDLIBS)

$(OBJS): build/%.o: src/%.cc $(HEADERS)
	$(CXX) $(CPPFLAGS) $(WFLAGS) $< -o $@

$(SPIRVS): build/%.spv: shaders/%.glsl
	$(GLSL) $< -o $@

exe: trace
	./trace

clean:
	$(RM) build/* trace

.PHONY: clean exe
