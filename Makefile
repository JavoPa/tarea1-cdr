# Makefile para compilar el cliente y el servidor

CC = g++
CFLAGS = -std=c++11 -Wall

# Directorios de salida
BUILD_DIR = build
BIN_DIR = bin

# Archivos fuente del cliente y el servidor
CLIENT_SRC = cliente.cpp
SERVER_SRC = servidor.cpp

# Nombres de los ejecutables
CLIENT_EXE = cliente
SERVER_EXE = servidor

# Objetivos de construcción
all: $(CLIENT_EXE) $(SERVER_EXE)

# Cliente
$(CLIENT_EXE): $(BUILD_DIR)/$(CLIENT_SRC:.cpp=.o) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

# Servidor
$(SERVER_EXE): $(BUILD_DIR)/$(SERVER_SRC:.cpp=.o) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

# Compilación de archivos fuente
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Crear directorios de salida si no existen
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Limpiar archivos compilados y ejecutables
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
