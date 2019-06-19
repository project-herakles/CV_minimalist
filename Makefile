.PHONY: all clean

all: demo

demo: FLIRCamera.o FLIRCameraDemo.o
	$(CXX) FLIRCamera.o FLIRCameraDemo.o `pkg-config --libs opencv4` -lSpinnaker -o demo

FLIRCamera.o: FLIRCamera.cpp
	$(CXX) -std=c++11 -c FLIRCamera.cpp `pkg-config --cflags opencv4` -I/usr/include/spinnaker

FLIRCameraDemo.o: FLIRCameraDemo.cpp
	$(CXX) -std=c++11 -c FLIRCameraDemo.cpp `pkg-config --cflags opencv4` -I/usr/include/spinnaker

clean:
	rm -rf demo *.o
