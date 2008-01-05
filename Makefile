PACKAGE=prometheus
VERSION=0.7.7
CFLAGS=-Wall
prefix=/usr
mandir=$(prefix)/share/man
sbindir=$(prefix)/sbin
sysconfdir=/etc/

main: prometheus
	$(CC) -o prometheus prometheus.c

install: main
	install -d $(sbindir)
	install -d $(mandir)/man1
	install -d $(mandir)/man5
	install -d $(sysconfdir)/cron.d
	install -m 755 prometheus $(sbindir)
	install -m 644 prometheus.1 $(mandir)/man1
	install -m 644 prometheus.conf.5 $(mandir)/man5
	install -m 755 sample-configuration/prometheus.cron $(sysconfdir)/cron.d/prometheus

clean:
	rm -f prometheus
