CC=gcc

BROKER_DIR = broker/src
PUBLISHER_DIR = publisher/src
SUBSCRIBER_DIR = subscriber/src
RUDP_DIR = rudp

CFLAGS=-Wall -I. -I$(BROKER_DIR) -I$(PUBLISHER_DIR) -I$(SUBSCRIBER_DIR) -I$(RUDP_DIR)

BROKER_C = $(BROKER_DIR)/broker_rudp.c $(BROKER_DIR)/broker_common.c
PUBLISHER_C = $(PUBLISHER_DIR)/publisher_rudp.c $(PUBLISHER_DIR)/publisher_common.c
SUBSCRIBER_C = $(SUBSCRIBER_DIR)/subscriber_rudp.c $(SUBSCRIBER_DIR)/subscriber_common.c

TARGET_BROKER = broker_rudp
TARGET_PUB = publisher_rudp
TARGET_SUB = subscriber_rudp

all: $(TARGET_BROKER) $(TARGET_PUB) $(TARGET_SUB)

$(TARGET_BROKER): $(BROKER_C)
    $(CC) $(CFLAGS) $^ -o $@

$(TARGET_PUB): $(PUBLISHER_C)
    $(CC) $(CFLAGS) $^ -o $@

$(TARGET_SUB): $(SUBSCRIBER_C)
    $(CC) $(CFLAGS) $^ -o $@

clean:
    rm -f $(TARGET_BROKER) $(TARGET_PUB) $(TARGET_SUB)