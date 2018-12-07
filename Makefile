main.exe: main.o
	g++ -std=c++14 -O0 -g -Wall -o main.exe main.o -l mingw32 -l SDL2main -l SDL2

main.o: main.cpp
	g++ -std=c++14 -O0 -g -Wall -c -o main.o main.cpp

clean: 
	rm -f main.o main.exe