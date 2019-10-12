CC = gcc
CXX = g++
FLAGS = -Wall -O3 -flto
CFLAGS = -std=gnu11
CXXFLAGS = -std=gnu++11
LIBS = -lm -lpng -lpthread -lOpenCL
BIN = ./bin
OBJ = ./obj
objects = main.o Renderer.o Scene.o lodepng.o error.o float3.o BVH.o tinyply.o
OBJS = $(objects:%.o=$(OBJ)/%.o)
binaries = main
BINS = $(binaries:%=$(BIN)/%)

ifeq (1, $(DEBUG))
FLAGS := $(FLAGS) -g
endif

.PHONY: all
all: $(BINS)

$(BIN)/%: $(OBJ)/%.o $(OBJS)
	@echo Linking $@
	@mkdir -p $(BIN)
	@$(CXX) -o $@ $(OBJS) $(FLAGS) $(CXXFLAGS) $(LIBS)

.PRECIOUS: $(OBJ)/%.o
$(OBJ)/%.o: ./src/%.c
	@echo Compiling $<
	@mkdir -p $(OBJ)
	@$(CC) -MMD -c -o $@ $< $(FLAGS) $(CFLAGS)

$(OBJ)/%.o: ./src/%.cpp
	@echo Compiling $<
	@mkdir -p $(OBJ)
	@$(CXX) -MMD -c -o $@ $< $(FLAGS) $(CXXFLAGS)

.PHONY:clean
clean:
	@rm -f *.png
	@rm -f -r $(OBJ)
	@rm -f -r $(BIN)

-include $(OBJ)/*.d
