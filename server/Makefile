CC = gcc
CFLAGS = -Wno-implicit-function-declaration -g
INC = -Iinclude
MYSQL_FLAGS := $(shell mysql_config --cflags --libs)

SRC_DIR = src
BIN_DIR = bin

# 所有需要 mysql_config 的程式名稱
MYSQL_PROGRAMS = login listMail mailto delMail gyell createGroup delGroup addTo leaveGroup remove listGroup
ALL_PROGRAMS = $(basename $(notdir $(wildcard $(SRC_DIR)/*.c)))
EXECUTABLES = $(addprefix $(BIN_DIR)/, $(ALL_PROGRAMS))

.PHONY: all clean

all: $(EXECUTABLES)

# 動態生成執行檔規則
$(BIN_DIR)/%: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INC) $< -o $@ $(MYSQL_FLAGS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# 清理目標
clean:
	rm -f $(EXECUTABLES)
