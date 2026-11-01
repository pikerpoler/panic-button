CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I../src -I.

OBJS = ../src/panic_engine.cpp ../src/uuid_cache.cpp

test_panic: test_panic.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) test_panic.cpp $(OBJS) -o test_panic

test_button: test_button.cpp ../src/button_input.cpp
	$(CXX) $(CXXFLAGS) test_button.cpp ../src/button_input.cpp -o test_button

run: test_panic test_button
	./test_panic
	./test_button

clean:
	rm -f test_panic test_button
