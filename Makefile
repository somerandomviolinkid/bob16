SRC_DIR:=src
BUILD_DIR:=build

all: $(BUILD_DIR)/main.exe

$(BUILD_DIR)/main.exe: $(SRC_DIR)/main.c
	mkdir $(BUILD_DIR)
	gcc $(SRC_DIR)/main.c -o $(BUILD_DIR)/main.exe

run: all
	$(BUILD_DIR)/main.exe

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
