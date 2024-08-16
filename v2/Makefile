all: simple-server-cpp simple-client-cpp

simple-server-cpp: simple-server-cpp.o
	g++ -o simple-server-cpp simple-server-cpp.o

simple-client-cpp: simple-client-cpp.o
	g++ -o simple-client-cpp simple-client-cpp.o

simple-server-cpp.o: simple-server-cpp.cpp
	g++ -c simple-server-cpp.cpp

simple-client-cpp.o: simple-client-cpp.cpp
	g++ -c simple-client-cpp.cpp

clean:
	-rm -f *.o simple-server-cpp simple-client-cpp
