CXX = g++

CXXFLAGS = -Wall -std=c++11 -lmingw32 -lSDL3

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
	$(CXX) -L$(LIBS) -I$(INCLUDES) $^ -o $@   $(CXXFLAGS)