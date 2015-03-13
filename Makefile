
ARCH=$(shell uname -m | grep arm && echo -march=armv5)
OPT_MODE=$(shell if [[ "$(MODE)" = "Release" ]]; then echo "-O2 -DNDEBUG"; else echo "-g"; fi)

CXXFLAGS=$(ARCH) -std=c++11 -D_GLIBCXX_USE_NANOSLEEP $(OPT_MODE) -lpthreads
WFLAGS=-Wall -Wextra -Wold-style-cast
DEPS=ev3dev.h
OBJ=ev3dev.o

all:  $(OBJ)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(WFLAGS)

ev3dev.o : ev3dev.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

drive-test: $(OBJ) drive-test.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

button-test: $(OBJ) button-test.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

.PHONY: all clean

clean:
	rm -f *.o *~ ev3dev-lang-test ev3dev-lang-demo remote_control-test drive-test button-test
