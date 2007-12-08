PACKAGE=prometheus
VERSION=0.7.1
CFLAGS=-Wall
prefix=/usr
mandir=$(prefix)/share/man
bindir=$(prefix)/sbin

main: prometheus
	$(CC) -o prometheus prometheus.c

install: main
	install -d $(bindir)
	install -m 755 prometheus $(bindir)

clean:
	rm -f prometheus
