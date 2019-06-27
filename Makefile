CXXFLAGS += -DFLIR

.PHONY: all clean

all: demo ng

demo: FLIRCamera.o FLIRCameraDemo.o
	$(CXX) $(CXXFLAGS) FLIRCamera.o FLIRCameraDemo.o `pkg-config --libs opencv4` -lSpinnaker -o demo

ng: FLIRCamera.o ng.o
	$(CXX) $(CXXFLAGS) FLIRCamera.o ng.o `pkg-config --libs opencv4` -lSpinnaker -o ng

ng.o: ng.cpp
	$(CXX) $(CXXFLAGS) -std=c++11 -c ng.cpp `pkg-config --cflags opencv4` -I/usr/include/spinnaker

FLIRCamera.o: FLIRCamera.cpp
	$(CXX) $(CXXFLAGS) -std=c++11 -c FLIRCamera.cpp `pkg-config --cflags opencv4` -I/usr/include/spinnaker

FLIRCameraDemo.o: FLIRCameraDemo.cpp
	$(CXX) $(CXXFLAGS) -std=c++11 -c FLIRCameraDemo.cpp `pkg-config --cflags opencv4` -I/usr/include/spinnaker

clean:
	rm -rf demo ng *.o
