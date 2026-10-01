CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I../src

OBJS = ../src/panic_engine.cpp ../src/uuid_cache.cpp

test_panic: test_panic.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) test_panic.cpp $(OBJS) -o test_panic

run: test_panic
	./test_panic

clean:
	rm -f test_panic
