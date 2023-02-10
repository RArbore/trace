CXX := g++
LD := g++
RM := rm -f
GLSL := glslc

RELEASE ?= 0
ifeq ($(RELEASE), 1)
	CPPFLAGS := -Ofast -march=native -flto -DRELEASE
	GLSLFLAGS := -O
	LDFLAGS := -flto
else
	CPPFLAGS := -g
	GLSLFLAGS := -g
endif

CPPFLAGS := $(CPPFLAGS) -c -fno-rtti -pipe -Iimgui -Iimgui/backends -std=c++20
GLSLFLAGS := $(GLSLFLAGS) --target-spv=spv1.5 --target-env=vulkan1.2
LDFLAGS := $(LDFLAGS) -fuse-ld=mold
WFLAGS := -Wall -Wextra -Wshadow -Wconversion -Wpedantic
LDLIBS := -lvulkan -lglfw
IMGUI_FLAGS := -c -Iimgui -Iimgui/backends

SRCS := $(shell find src -name "*.cc")
HEADERS := $(shell find src -name "*.h")
SHADERS := $(shell find shaders -name "*.glsl" | grep -v "common")
SHADER_HEADERS := $(shell find shaders -name "*.glsl" | grep -e "common")
OBJS := $(subst src/, build/, $(patsubst %.cc, %.o, $(SRCS)))
SPIRVS := $(subst shaders/, build/, $(patsubst %.glsl, %.spv, $(SHADERS)))
IMGUI_SRCS := imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_vulkan.cpp
IMGUI_OBJS := build/imgui/imgui.o build/imgui/imgui_demo.o build/imgui/imgui_draw.o build/imgui/imgui_tables.o build/imgui/imgui_widgets.o build/imgui/imgui_impl_glfw.o build/imgui/imgui_impl_vulkan.o

trace: $(OBJS) $(SPIRVS) $(IMGUI_OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(IMGUI_OBJS) -o trace $(LDLIBS)

$(OBJS): build/%.o: src/%.cc $(HEADERS)
	$(CXX) $(CPPFLAGS) $(WFLAGS) $< -o $@

$(SPIRVS): build/%.spv: shaders/%.glsl $(SHADER_HEADERS)
	$(GLSL) $(GLSLFLAGS) $< -o $@

build/imgui/imgui.o: imgui/imgui.cpp
	$(CXX) $(IMGUI_FLAGS) $< -o $@

build/imgui/imgui_demo.o: imgui/imgui_demo.cpp
	$(CXX) $(IMGUI_FLAGS) $< -o $@

build/imgui/imgui_draw.o: imgui/imgui_draw.cpp
	$(CXX) $(IMGUI_FLAGS) $< -o $@

build/imgui/imgui_tables.o: imgui/imgui_tables.cpp
	$(CXX) $(IMGUI_FLAGS) $< -o $@

build/imgui/imgui_widgets.o: imgui/imgui_widgets.cpp
	$(CXX) $(IMGUI_FLAGS) $< -o $@

build/imgui/imgui_impl_glfw.o: imgui/backends/imgui_impl_glfw.cpp
	$(CXX) $(IMGUI_FLAGS) $< -o $@

build/imgui/imgui_impl_vulkan.o: imgui/backends/imgui_impl_vulkan.cpp
	$(CXX) $(IMGUI_FLAGS) $< -o $@

exe: trace
	./trace

clean:
	$(RM) build/*.o build/*.spv trace

.PHONY: clean exe
