CXX := g++

all: stack1 stack2 stack3

stack1: stack1.cpp stack1.hpp
	$(CXX) -std=c++0x stack1.cpp -o $@

stack2: stack2.cpp stack2.hpp
	$(CXX) -std=c++0x stack2.cpp -o $@

stack3: stack3.cpp stack3.hpp haz_ptr.hpp haz_ptr.cpp
	$(CXX) -std=c++0x stack3.cpp haz_ptr.cpp -o $@

clean:
	rm -f stack1 stack2 stack3
