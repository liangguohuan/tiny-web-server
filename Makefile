CC = gcc
CFLAGS = -Wall -O2
CONFIG_DIR=~/.config/tinyserver
BINFILE=/usr/local/bin/tinyserver

all: tiny

tiny: tiny.c
	install -d $(CONFIG_DIR)
	install -d $(CONFIG_DIR)/cache
	install -m 664 dir.template.html $(CONFIG_DIR)/dir.template.html
	sudo $(CC) $(CFLAGS) -o $(BINFILE) *.c

check:
	rm -f *.o tiny *~
	$(CC) $(CFLAGS) -o tiny *.c

clean:
	rm -f *.o tiny *~
	rm -rf $(CONFIG_DIR)

install:
	make clean
	make all

uninstall:
	make clean
	sudo rm -rf $(BINFILE)

