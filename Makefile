

CXX = g++ -mavx -std=c++14 -O3 -g -Wall 
H_FILES = voxel.h volume.h main.h svo.h PLYfileReader.h
O_FILES = volume.o main.o svo.o PLYfileReader.o

main.exe: $(O_FILES) Makefile
	$(CXX) -o main.exe $(O_FILES) libDGtal.so -l mingw32 -l SDL2main -l SDL2

volume.o: volume.cpp volume.h voxel.h Makefile
	$(CXX) -c -o volume.o volume.cpp

svo.o: svo.cpp $(H_FILES) Makefile
	$(CXX) -c -o svo.o svo.cpp

PLYfileReader.o: main.cpp $(H_FILES) Makefile
	$(CXX) -c -o PLYfileReader.o PLYfileReader.cpp

main.o: main.cpp $(H_FILES) Makefile
	$(CXX) -c -o main.o main.cpp

clean: 
	rm -f $(O_FILES) main.exe