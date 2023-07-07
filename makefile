a.out : lern1.cpp
	g++ -std=c++17 -O2 -Wall -pedantic -pthread lern1.cpp && ./a.out

clean :
	rm *.out $(objects)
