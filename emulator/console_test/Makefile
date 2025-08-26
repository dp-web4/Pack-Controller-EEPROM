# Makefile for Pack Controller Console Test
# Uses MinGW-w64 on Windows or g++ on Linux/WSL

CXX = g++
CXXFLAGS = -std=c++17 -Wall -I../../Core/Inc -I../include -I. -DCONSOLE_TEST
LDFLAGS = -static-libgcc -static-libstdc++

# Source files
SOURCES = main.cpp \
          test_web4.cpp \
          web4_handler_stub.cpp

OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS := $(OBJECTS:.c=.o)

TARGET = pack_console_test.exe

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean