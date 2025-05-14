BIN_DIR = bin
OBJ_DIR = obj

all: $(BIN_DIR)/client $(BIN_DIR)/server

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $(BIN_DIR) $(OBJ_DIR)

$(OBJ_DIR)/common.o: common.C | $(OBJ_DIR)
	gcc -Wall -c common.C -o $(OBJ_DIR)/common.o

$(BIN_DIR)/client: client.C $(OBJ_DIR)/common.o | $(BIN_DIR)
	gcc -Wall client.C $(OBJ_DIR)/common.o -o $(BIN_DIR)/client

$(BIN_DIR)/server: server.C $(OBJ_DIR)/common.o | $(BIN_DIR)
	gcc -Wall server.C $(OBJ_DIR)/common.o -o $(BIN_DIR)/server

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)