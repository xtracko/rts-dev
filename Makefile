
ARCH=$(shell uname -m | grep -q arm && echo -march=armv5)
OPT_MODE=$(shell if [ "$(MODE)" = "Release" ]; then echo "-O2 -DNDEBUG"; else echo "-g"; fi)

CXXFLAGS=$(ARCH) -std=c++1y -D_GLIBCXX_USE_NANOSLEEP $(OPT_MODE) -pthread
WFLAGS=-Wall -Wextra -Wold-style-cast
DEPS=ev3dev.h
OBJ=ev3dev.o

all:  $(OBJ) bot2

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(WFLAGS)

ev3dev.o : ev3dev.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

line: ${OBJ} line.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

line2: ${OBJ} line2.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

bot: ${OBJ} bot.o
	$(CXX) -o $@ $^ $(CXXFLAGS)
	
bot2: ${OBJ} bot2.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

test : buffer-test
	./buffer-test

buffer-test : buffer-test.cpp buffer.h
	$(CXX) -o $@ $< $(CXXFLAGS)

job-test : job-test.cpp job.h
	$(CXX) -o $@ $< $(CXXFLAGS)

.PHONY: all clean

clean:
	rm -f bot2.o bot2
