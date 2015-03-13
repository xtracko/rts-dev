
CFLAGS=-O2 -march=armv5 -lpthread
CCFLAGS=-std=c++11 -D_GLIBCXX_USE_NANOSLEEP
DEPS=ev3dev.h
OBJ=ev3dev.o
LIBS=-lstdc++

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(CCFLAGS)


drive-test: $(OBJ) drive-test.o
	$(CC) -o $@ $^ $(CFLAGS) $(CCFLAGS) $(LIBS) -lpthread
	
button-test: $(OBJ) button-test.o
	$(CC) -o $@ $^ $(CFLAGS) $(CCFLAGS) $(LIBS)

.PHONY: all clean

clean:
	rm -f *.o *~ ev3dev-lang-test ev3dev-lang-demo remote_control-test drive-test button-test

all:  ev3dev-lang-test ev3dev-lang-demo remote_control-test drive-test button-t
