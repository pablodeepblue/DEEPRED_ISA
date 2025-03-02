CXX = g++

CXXFLAGS = -Wall -Wextra -Wshadow -std=c++17

EXECUTABLE = main

SRC := ./src
BUILD := ./build
LIBS = ./lib
INCLUDES = ./include
OBJ := obj

all: $(BUILD)/$(EXECUTABLE)

$(BUILD)/$(EXECUTABLE): $(SRC)/$(EXECUTABLE).cpp
	$(CXX) -L$(LIBS) -I$(INCLUDES) $^ -o $@ $(CXXFLAGS)

run: $(BUILD)/$(EXECUTABLE)
	$(BUILD)/$(EXECUTABLE)


memory_manager: memdrive
memdrive : $(SRC)/memdrive.cpp
	$(CXX) -L$(LIBS) -I$(INCLUDES) $^ -o $@  $(CXXFLAGS)