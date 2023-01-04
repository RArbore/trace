CXX := g++
LD := g++
RM := rm -f
CPPFLAGS := -c -g
LDFLAGS := -fuse-ld=mold
LDLIBS :=

all: trace

trace: obj/main.o
	$(LD) $(LDFLAGS) $^ -o trace $(LDLIBS)

obj/main.o: src/main.cc
	$(CXX) $(CPPFLAGS) $< -o $@

clean:
	$(RM) obj/*
