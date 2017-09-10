CC = gcc
CFLAGS = -Wall -O2
TARGET=tinyserver
SRC=functions.c tiny.c
OBJS=$(SRC:.c=.o)
CONFIG_DIR=~/.config/$(TARGET)
BINFILE=/usr/local/bin/$(TARGET)
LOGNAME=$(shell echo $(HOME) | cut -d / -f3)

.PHONY: all
all: install

.PHONY: $(TARGET)
$(TARGET): $(OBJS) 
	$(CC) $(CFLAGS) -o $@.o $^

.PHONY: config
config:
	install -g $(LOGNAME) -o $(LOGNAME) -d $(CONFIG_DIR)
	install -g $(LOGNAME) -o $(LOGNAME) -d $(CONFIG_DIR)/cache
	install -g $(LOGNAME) -o $(LOGNAME) -m 664 dir.template.html $(CONFIG_DIR)/dir.template.html

.PHONY: check
check: clean $(TARGET)

.PHONY: test
test: check

.PHONY: clean
clean:
	rm -f *.o *~ $(TARGET) $(OBJS)

.PHONY: install
install: clean config $(TARGET)
	install $(TARGET).o $(BINFILE)

.PHONY: uninstall
uninstall: clean
	rm -rf $(CONFIG_DIR)
	rm -rf $(BINFILE)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
