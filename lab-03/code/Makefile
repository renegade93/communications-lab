# Makefile para el proyecto R-UDP con estructura de directorios
CC=gcc

# 1. Definimos las rutas a los directorios de código fuente
BROKER_DIR = broker/src
PUBLISHER_DIR = publisher/src
SUBSCRIBER_DIR = subscriber/src
RUDP_DIR = rudp

# 2. Le decimos al compilador dónde buscar los archivos .h (-I<directorio>)
CFLAGS=-Wall -I. -I$(BROKER_DIR) -I$(PUBLISHER_DIR) -I$(SUBSCRIBER_DIR) -I$(RUDP_DIR)

# 3. Listamos los archivos fuente (.c) usando sus rutas completas
BROKER_C = $(BROKER_DIR)/broker_rudp.c $(BROKER_DIR)/broker_common.c
PUBLISHER_C = $(PUBLISHER_DIR)/publisher_rudp.c $(PUBLISHER_DIR)/publisher_common.c
SUBSCRIBER_C = $(SUBSCRIBER_DIR)/subscriber_rudp.c $(SUBSCRIBER_DIR)/subscriber_common.c

# 4. Definimos los nombres de los ejecutables de salida
TARGET_BROKER = broker_rudp
TARGET_PUB = publisher_rudp
TARGET_SUB = subscriber_rudp

# Objetivo principal
all: $(TARGET_BROKER) $(TARGET_PUB) $(TARGET_SUB)

# 5. Reglas de compilación actualizadas
$(TARGET_BROKER): $(BROKER_C)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_PUB): $(PUBLISHER_C)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_SUB): $(SUBSCRIBER_C)
	$(CC) $(CFLAGS) $^ -o $@

# Limpieza
clean:
	rm -f $(TARGET_BROKER) $(TARGET_PUB) $(TARGET_SUB)