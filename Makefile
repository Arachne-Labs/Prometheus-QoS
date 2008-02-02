PACKAGE=prometheus
VERSION=0.7.8
CFLAGS=-std=c99 -Wall
prefix=/usr
mandir=$(prefix)/share/man
sbindir=$(prefix)/sbin
sysconfdir=/etc

main: prometheus
	$(CC) -o prometheus prometheus.c

deb: main
	debian/prometheus.debian
	dpkg-buildpackage

tgz: clean
	cp -r . ../$(PACKAGE)-$(VERSION)
	rm -rf ../$(PACKAGE)-$(VERSION)/.svn/
	rm -rf ../$(PACKAGE)-$(VERSION)/*/.svn/
	rm -rf ../$(PACKAGE)-$(VERSION)/*~ $(PACKAGE)-$(VERSION)/*/*~
	tar -czf ../$(PACKAGE)-$(VERSION).tar.gz ../$(PACKAGE)-$(VERSION)
	rm -rf ../$(PACKAGE)-$(VERSION)

install: main
	install -d $(sbindir)
	install -d $(mandir)/man1
	install -d $(mandir)/man5
	install -d $(sysconfdir)/cron.d
	install -d $(sysconfdir)/prometheus
	install -m 755 prometheus $(sbindir)
	install -m 644 prometheus.1 $(mandir)/man1
	install -m 644 prometheus.conf.5 $(mandir)/man5
	install -m 755 conf/prometheus.cron $(sysconfdir)/cron.d/prometheus
	install -m 755 conf/prometheus.init $(sysconfdir)/init.d/prometheus
	install -m 600 conf/prometheus.conf $(sysconfdir)/prometheus
	install -m 600 conf/prometheus.hosts $(sysconfdir)/prometheus/hosts
	install -m 644 conf/prometheus.default $(sysconfdir)/default/prometheus

clean:
	rm -f prometheus
	rm -f optinal-tools/prometheus-stats
