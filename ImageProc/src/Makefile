

all:
	g++ image.cpp ImageProc.cpp  `pkg-config --cflags --libs opencv`  -shared -fPIC  -o libimageproc.so 
clean:
	rm *.o *.so

