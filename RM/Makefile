PRG_NAME := RM

ROOT_DIR := $(abspath .)
SRC_DIR := ./src
OBJ_DIR := obj

C_SRCS := $(shell find $(SRC_DIR) -name '*.c')
CPP_SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(C_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) $(CPP_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CC := wcc
CXX := wpp
CFLAGS := -ms -s -zls -zc -ox -oe=2 -ot -oh -oi -ol+ -i=$(WATCOM)/h
CXXFLAGS := -ms -s -zls -zc -ox -oe=2 -ot -oh -oi -ol+ -xs -i=$(WATCOM)/h
LDFLAGS := system dos option quiet, map, eliminate, stack=1k

all: $(PRG_NAME).exe run

$(PRG_NAME).exe: $(OBJS)
	@wlink name $(PRG_NAME).exe file { $^ } $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@echo "compiling $<"
	@$(CC) $(CFLAGS) -fo=$@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo "compiling $<"
	@$(CXX) $(CXXFLAGS) -fo=$@ $< 

clean:
	@clear
	@rm -f $(PRG_NAME).exe
	@rm -rf $(OBJ_DIR)

reset: clean all

run: $(PRG_NAME).exe
	@dosbox -c "mount c $(ROOT_DIR)" -c "c:" -c "cd $(PRG_NAME)" -c "cls"

.PHONY: clean reset all run
