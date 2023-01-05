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

exe: trace
	./trace

trace: obj/main.o obj/context.o obj/device.o obj/swapchain.o obj/alloc.o
	$(LD) $(LDFLAGS) $^ -o trace $(LDLIBS)

obj/main.o: src/main.cc src/context.h src/util.h
	$(CXX) $(CPPFLAGS) $(WFLAGS) $< -o $@

obj/context.o: src/context.cc src/context.h src/util.h
	$(CXX) $(CPPFLAGS) $(WFLAGS) $< -o $@

obj/device.o: src/device.cc src/context.h src/util.h
	$(CXX) $(CPPFLAGS) $(WFLAGS) $< -o $@

obj/swapchain.o: src/swapchain.cc src/context.h src/util.h
	$(CXX) $(CPPFLAGS) $(WFLAGS) $< -o $@

obj/alloc.o: src/alloc.cc src/context.h src/util.h
	$(CXX) $(CPPFLAGS) $(WFLAGS) $< -o $@

clean:
	$(RM) obj/* trace

.PHONY: exe