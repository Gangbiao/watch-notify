CXXFLAGS=-std=c++0x -I../../src/include -I../../src
#CXXFLAGS=-std=c++0x -I../../src/include
#CFLAGS=-I../../src/include -I../../src
CFLAGS=-I../../src
all: watcher_cpp notifier_cpp

watcher_cpp: watcher.cc
	g++ -g -c watcher.cc -o watcher_cpp.o $(CXXFLAGS)
	g++ -g watcher_cpp.o -lrados -o watcher_cpp $(LDFLAGS)

notifier_cpp: notifier.cc
	g++ -g -c notifier.cc -o notifier_cpp.o $(CXXFLAGS)
	g++ -g notifier_cpp.o -lrados -o notifier_cpp $(LDFLAGS)

clean:
	rm -rf *.o notifier_cpp watcher_cpp
