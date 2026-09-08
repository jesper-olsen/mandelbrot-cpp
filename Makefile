# -pthread is a no-op flag on Apple clang/macOS (pthread is always linked)
# but is required for std::thread/std::jthread to link on Linux/libstdc++.
CXX = clang++
CXXFLAGS = -std=c++23 -O3 -Wall -Wextra
THREAD_FLAGS = -pthread

TARGET = mandelbrot
THREAD_TARGET = mandelbrot_thread
SRC = mandelbrot.cpp
THREAD_SRC = mandelbrot_thread.cpp

all: $(TARGET) $(THREAD_TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

$(THREAD_TARGET): $(THREAD_SRC)
	$(CXX) $(CXXFLAGS) $(THREAD_FLAGS) -o $(THREAD_TARGET) $(THREAD_SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(THREAD_TARGET)

.PHONY: all run clean
