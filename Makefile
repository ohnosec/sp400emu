BUILD_DIR=./build
PYTHON?=python3
GENERATED_DIR=$(BUILD_DIR)/generated
CURSOR_ASSET_HEADER=$(GENERATED_DIR)/paper_cursor_assets.h
CXXFLAGS=-Wall -pthread -I. -I$(GENERATED_DIR)
CPP_SOURCES= serial.cpp file_input.cpp tcp_input.cpp m68sys.cpp plotter.cpp paper_cursor.cpp main.cpp board.cpp
C_SOURCES= stepper.c m68emu/m68emu.c m68emu/m68_ops.c m68emu/m68tmr.c




OBJ = $(CPP_SOURCES:%.cpp=$(BUILD_DIR)/%.o) $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)
DEP = $(OBJ:%.o=%.d)


sp400: $(OBJ) 
	g++ $(CXXFLAGS) -o $@ $^ -lSDL2



# Include all .d files
-include $(DEP)
# Build target for every single object file.
# The potential dependency on header files is covered
# by calling `-include $(DEP)`.
$(BUILD_DIR)/%.o : %.cpp
	mkdir -p $(@D)
# The -MMD flags additionaly creates a .d file with
 # the same name as the .o file.
	g++ $(CXXFLAGS) -c $< -o $@ -MMD -MP
$(BUILD_DIR)/%.o : %.c
	mkdir -p $(@D)
	gcc $(CXXFLAGS) -c $< -o $@ -MMD -MP

$(BUILD_DIR)/paper_cursor.o: $(CURSOR_ASSET_HEADER)

$(CURSOR_ASSET_HEADER): scripts/embed_cursor_assets.py assets/cursors/open_hand.bmp assets/cursors/closed_hand.bmp
	mkdir -p $(@D)
	$(PYTHON) scripts/embed_cursor_assets.py --open assets/cursors/open_hand.bmp --closed assets/cursors/closed_hand.bmp --output $@


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
clean:
	rm -rf build sp400
