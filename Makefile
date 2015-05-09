TARGET=pipeline

$(TARGET): main.o error.o load.o output.o
	g++ -o $(TARGET) main.o error.o load.o output.o

main.o: main.cpp parameter.h error.h load.h output.h
	g++ -c main.cpp

error.o: error.h error.cpp parameter.h
	g++ -c error.cpp
	
load.o: load.h load.cpp parameter.h
	g++ -c load.cpp
	
output.o: output.h output.cpp parameter.h
	g++ -c output.cpp

clean:
	rm -f $(TARGET) *.o *.bin *.rpt *.exe *.out
