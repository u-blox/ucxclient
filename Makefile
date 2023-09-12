TARGET_EXEC := at_client

BUILD_DIR := ./build

SRCS := \
	at_client.c \
	at_util.c \
	main.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CFLAGS := $(INC_FLAGS) -g -MMD -MP

all: $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

-include $(DEPS)