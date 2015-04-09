
ARCH=$(shell uname -m | grep -q arm && echo -march=armv5)
OPT_MODE=$(shell if [ "$(MODE)" = "Release" ]; then echo "-O2 -DNDEBUG"; else echo "-g"; fi)

CXXFLAGS=$(ARCH) -std=c++1y -D_GLIBCXX_USE_NANOSLEEP $(OPT_MODE)
WFLAGS=-Wall -Wextra -Wold-style-cast
DEPS=ev3dev.h
OBJ=ev3dev.o

all:  $(OBJ) bot

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


.PHONY: all clean

clean:
	rm -f bot.o bot
