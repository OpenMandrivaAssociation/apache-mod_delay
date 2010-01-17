#Module-Specific definitions
%define mod_name mod_delay
%define mod_conf A67_%{mod_name}.conf
%define mod_so %{mod_name}.so

Summary:	Delay module for Apache2
Name:		apache-%{mod_name}
Version:	0.9.2
Release:	%mkrel 1
Group:		System/Servers
License:	Apache License
URL:		http://www.heute-morgen.de/modules/mod_delay/
Source0:	http://www.heute-morgen.de/modules/mod_delay/mod_delay.c
Source1:	http://www.heute-morgen.de/modules/mod_delay/mod_delay.h
Source2:	%{mod_conf}
Requires(pre): rpm-helper
Requires(postun): rpm-helper
Requires(pre):	apache-conf >= 2.0.54
Requires(pre):	apache >= 2.0.54
Requires:	apache-conf >= 2.0.54
Requires:	apache >= 2.0.54
BuildRequires:  apache-devel >= 2.0.54
BuildRequires:	file
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
mod_delay is an output filter that does buffering until it is told to stop this
(by a note). It also enables the sending of 304 by delaying the call to
ap_meets_conditions until then. The module exports one optional functions and
has no configuration. It registers as an output filter named delay.

%package	devel
Summary:	Development API for the mod_delay apache module
Group:		Development/C

%description	devel
mod_delay is an output filter that does buffering until it is told to stop this
(by a note). It also enables the sending of 304 by delaying the call to
ap_meets_conditions until then. The module exports one optional functions and
has no configuration. It registers as an output filter named delay.

This package contains the development API for the mod_delay apache module.

%prep

%setup -q -c -T -n %{mod_name}

cp %{SOURCE0} .
cp %{SOURCE1} .
cp %{SOURCE2} .

find . -type d -exec chmod 755 {} \;
find . -type f -exec chmod 644 {} \;

for i in `find . -type d -name CVS` `find . -type d -name .svn` `find . -type f -name .cvs\*` `find . -type f -name .#\*`; do
    if [ -e "$i" ]; then rm -r $i; fi >&/dev/null
done

# strip away annoying ^M
find . -type f|xargs file|grep 'CRLF'|cut -d: -f1|xargs perl -p -i -e 's/\r//'
find . -type f|xargs file|grep 'text'|cut -d: -f1|xargs perl -p -i -e 's/\r//'

%build

%{_sbindir}/apxs -c %{mod_name}.c

%install
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

install -d %{buildroot}%{_sysconfdir}/httpd/modules.d
install -d %{buildroot}%{_libdir}/apache-extramodules
install -d %{buildroot}%{_includedir}

install -m0755 .libs/%{mod_so} %{buildroot}%{_libdir}/apache-extramodules/
install -m0644 mod_delay.h %{buildroot}%{_includedir}/
install -m0644 %{mod_conf} %{buildroot}%{_sysconfdir}/httpd/modules.d/%{mod_conf}

%post
if [ -f /var/lock/subsys/httpd ]; then
    %{_initrddir}/httpd restart 1>&2;
fi

%postun
if [ "$1" = "0" ]; then
    if [ -f /var/lock/subsys/httpd ]; then
	%{_initrddir}/httpd restart 1>&2
    fi
fi

%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/httpd/modules.d/%{mod_conf}
%attr(0755,root,root) %{_libdir}/apache-extramodules/%{mod_so}

%files devel
%defattr(-,root,root)
%attr(0644,root,root) %{_includedir}/mod_delay.h
