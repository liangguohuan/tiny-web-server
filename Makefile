CC = gcc
CFLAGS = -Wall -O2
CONFIG_DIR=~/.config/tinyserver
BINFILE=/usr/local/bin/tinyserver
TARGET=tinyserver

.PHONY: all
all: install

.PHONY: $(TARGET)
$(TARGET): 
	$(CC) $(CFLAGS) -o $@.o *.c

.PHONY: config
config:
	install -d $(CONFIG_DIR)
	install -d $(CONFIG_DIR)/cache
	install -m 664 dir.template.html $(CONFIG_DIR)/dir.template.html

.PHONY: check
check:
	rm -f *.o *~
	$(CC) $(CFLAGS) -o $(TARGET).o *.c

.PHONY: clean
clean:
	rm -f *.o *~

.PHONY: install
install: clean config all
	sudo cp $(TARGET).o $(BINFILE)

.PHONY: uninstall
uninstall: clean
	rm -rf $(CONFIG_DIR)
	sudo rm -rf $(BINFILE)

*.o: *.c
	$(CC) $(CFLAGS) -o $@ -c $<
