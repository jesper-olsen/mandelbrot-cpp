CXX = clang++
CXXFLAGS = -std=c++23 -O3 -Wall -Wextra

TARGET = mandelbrot
SRC = mandelbrot.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
