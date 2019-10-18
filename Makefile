CC = gcc
CXX = g++
FLAGS = -Wall -O3 -flto
CFLAGS = -std=gnu11
CXXFLAGS = -std=gnu++11
BIN = ./bin
OBJ = ./obj

ifeq (1, $(DEBUG))
FLAGS := $(FLAGS) -g
endif

.PHONY:all clean main bounds

all: main bounds

main:
	@$(MAKE) --no-print-directory -f make_main CC='$(CC)' CXX='$(CXX)' FLAGS='$(FLAGS)' CFLAGS='$(CFLAGS)' CXXFLAGS='$(CXXFLAGS)' BIN='$(BIN)' OBJ='$(OBJ)'

bounds:
	@$(MAKE) --no-print-directory -f make_bounds CC='$(CC)' CXX='$(CXX)' FLAGS='$(FLAGS)' CFLAGS='$(CFLAGS)' CXXFLAGS='$(CXXFLAGS)' BIN='$(BIN)' OBJ='$(OBJ)'

clean:
	@rm -f *.png
	@rm -f -r $(OBJ)
	@rm -f -r $(BIN)
