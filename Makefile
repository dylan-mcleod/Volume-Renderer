CXX = g++ -mavx -std=c++14 -O3 -g -Wall 

main.exe: main.o Makefile
	$(CXX) -o main.exe main.o libDGtal.so -l mingw32 -l SDL2main -l SDL2

main.o: main.cpp volume.cpp Makefile
	$(CXX) -c -o main.o main.cpp

clean: 
	rm -f main.o main.exe