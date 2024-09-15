COMPILER := clang++
COMPILER_FLAGS := -std=c++23 -g
LD_FLAGS := -lGL -lGLEW -lglfw -lm -lassimp
OBJ_DIR := objs
BIN_DIR := bin

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(wildcard *.cpp))

all: directories mandelbrot_gl

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

mandelbrot_gl: $(OBJS)
	$(COMPILER) $(COMPILER_FLAGS) -o $(BIN_DIR)/$@ $^ $(LD_FLAGS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: all mandelbrot_gl
	./$(BIN_DIR)/mandelbrot_gl
	


