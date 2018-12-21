all:
	g++  -Wall -g -o test test.cpp   -Wl,--rpath=. `pkg-config --cflags --libs opencv` -L ./ -limageproc
run:   
	./test
clean:
	rm *.o *.so

