PACKAGE=prometheus
VERSION=0.7.7
CFLAGS=-Wall
prefix=/usr
mandir=$(prefix)/share/man
sbindir=$(prefix)/sbin
sysconfdir=/etc/

main: prometheus
	$(CC) -o prometheus prometheus.c

deb: main
	./prometheus.debian
	
install: main
	install -d $(sbindir)
	install -d $(mandir)/man1
	install -d $(mandir)/man5
	install -d $(sysconfdir)/cron.d
	install -d $(sysconfdir)/prometheus
	install -m 755 prometheus $(sbindir)
	install -m 644 prometheus.1 $(mandir)/man1
	install -m 644 prometheus.conf.5 $(mandir)/man5
	install -m 755 etc/cron.d/prometheus $(sysconfdir)/cron.d
	install -m 755 etc/init.d/prometheus $(sysconfdir)/init.d
	install -m 600 etc/prometheus/prometheus.conf $(sysconfdir)/prometheus
	install -m 600 etc/prometheus/hosts $(sysconfdir)/prometheus

clean:
	rm -f prometheus
