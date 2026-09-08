BUILD_DIR=./build/linux
TARGET=$(BUILD_DIR)/sp400
PYTHON?=python3
GENERATED_DIR=$(BUILD_DIR)/generated
CURSOR_ASSET_HEADER=$(GENERATED_DIR)/paper_cursor_assets.h
M68_OPTABLE_HEADER=$(GENERATED_DIR)/m68_optab_hc05.h
CPPFLAGS+=-I. -I$(GENERATED_DIR)
CXXFLAGS+=-Wall -pthread -std=c++17 -g
CFLAGS+=-Wall -pthread -std=c11 -g
CPP_SOURCES= serial.cpp file_input.cpp tcp_input.cpp m68sys.cpp plotter.cpp paper_cursor.cpp main.cpp board.cpp
C_SOURCES= stepper.c m68emu/m68emu.c m68emu/m68_ops.c m68emu/m68tmr.c




OBJ = $(CPP_SOURCES:%.cpp=$(BUILD_DIR)/%.o) $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)
DEP = $(OBJ:%.o=%.d)


.PHONY: all sp400 clean

all: $(TARGET)

sp400: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p $(@D)
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
	g++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@ -MMD -MP
$(BUILD_DIR)/%.o : %.c
	mkdir -p $(@D)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@ -MMD -MP

$(BUILD_DIR)/paper_cursor.o: $(CURSOR_ASSET_HEADER)

$(BUILD_DIR)/m68emu/m68_ops.o: $(M68_OPTABLE_HEADER)

$(CURSOR_ASSET_HEADER): scripts/embed_cursor_assets.py assets/cursors/open_hand.bmp assets/cursors/closed_hand.bmp
	mkdir -p $(@D)
	$(PYTHON) scripts/embed_cursor_assets.py --open assets/cursors/open_hand.bmp --closed assets/cursors/closed_hand.bmp --output $@

$(M68_OPTABLE_HEADER): m68emu/optable/makeoptab.py m68emu/optable/opcodes_m68hc05.csv m68emu/m68emu.h
	mkdir -p $(@D)
	$(PYTHON) m68emu/optable/makeoptab.py m68emu/optable/opcodes_m68hc05.csv m68hc05 optable > $@


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
clean:
	rm -rf $(BUILD_DIR)
	rm -f sp400
