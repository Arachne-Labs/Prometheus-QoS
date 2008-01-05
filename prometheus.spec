Summary: Traffic shaper replacement for Internet Service Providers (ISP).
Name: prometheus
Version: 0.7.7
Release: 2
License: GPL
Vendor: Arachne Labs http://www.arachne.cz
Packager: Tomas Lastovicka <aquarius@lamer.cz>
Group: Applications/System
Source0: http://gpl.arachne.cz/download/%name-%version.tar.gz
URL: http://gpl.arachne.cz
Requires: iptables, iproute, screen
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
QoS (or Quality-of-service) is traffic shaper replacement for Internet Service
Providers (ISP). Dump your vintage hard-wired routers/shapers (C|sco, etc.) 
in favour of powerful open source and free solution.

Prometheus QoS generates multiple nested HTB tc classes with various rate and
ceil values, and implements optional daily traffic quotas and data transfer
statistics (as HTML). It is compatible with NAT, both asymetrical and
symetrical, yet still provides good two-way shaping and prioritizing, both
upload and download. 

Prometheus QoS was written in C<<1 and depends on HTB algorithm hardcoded
in Linux kernel. It is currently being tested in real-world enviroment to
provide QoS services on internet gateway and proxy being used by 1500+ members
of CZFree.Net broadband community network. 

%prep
%setup -q

%build
make %{_smp_mflags}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_sbindir}
mkdir -p %{buildroot}%{_mandir}/man1
mkdir -p %{buildroot}%{_mandir}/man5
mkdir -p %{buildroot}%{_sysconfdir}/cron.d/

%makeinstall

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_sbindir}/prometheus
%{_mandir}/man1/prometheus.1*
%{_mandir}/man5/prometheus.conf.5*
%{_sysconfdir}/cron.d/prometheus

%changelog
* Sat Jan 5 2008 Tomas Lastovicka <aquarius@lamer.cz> 0.7.7-2
- added manual pages
- added crontab file

* Fri Dec 28 2007 Tomas Lastovicka <aquarius@lamer.cz> 0.7.7-1
- update to latest upstream upstream 0.7.7

* Mon Dec 03 2007 Tomas Lastovicka <aquarius@lamer.cz> 0.7.1-1
- Upgraded to newest version, first *real* build :).

* Sun Dec 02 2007 Tomas Lastovicka <aquarius@lamer.cz> 0.7-1
- Initial RPM release.
