PACKAGE=prometheus
VERSION=0.7.7
CFLAGS=-Wall
prefix=/usr
mandir=$(prefix)/share/man
bindir=$(prefix)/sbin
crondir=/etc/cron.d

main: prometheus
	$(CC) -o prometheus prometheus.c

install: main
	install -d $(bindir)
	install -m 755 prometheus $(bindir)
	install -m 755 sample_config/prometheus.cron $(crondir)/prometheus

clean:
	rm -f prometheus
