%define name dia
%define ver 0.90.RC2

Summary: A gtk+ based diagram creation program.
Name: %name
Version: %ver
Release: 1
Copyright: GPL
Group: Applications/
Source: ftp://ftp.gnome.org/pub/GNOME/stable/sources/dia/%{name}-%{ver}.tar.gz
URL: http://www.lysator.liu.se/~alla/dia/dia.html
BuildRoot: /var/tmp/%{name}-%{ver}-root

Requires: libxml >= 1.8.5

%description
Dia is a GNU program designed to be much like the Windows
program 'Visio'. It can be used to draw different kind of diagrams.

It can be used to draw a variety of diagram types, including UML, Network,
flowchart and others.  The native file format for Dia is XML (optionally
gzip compressed).  It has print support, and can export to a number of formats such as EPS, SVG, CGM and PNG.

%changelog
* Fri May 11 2001  James Henstridge  <james@daa.com.au>
- update version number.
- Use make install rather than install-strip as it is causing some problems.
* Sun Aug 6 2000  James Henstridge <james@daa.com.au>
- update description as it was out of date, and increment version number.
* Sun Sep 5 1999  James Henstridge <james@daa.com.au>
- added $(prefix)/share/dia to files list.
* Thu Apr 29 1999  Enrico Scholz <enrico.scholz@wirtschaft.tu-chemnitz.de>
- Made %setup quiet
- Enabled build from cvs
- Removed superfluous mkdirs
- using DESTDIR and install-strip
* Fri Aug 28 1998  Francis J. Lacoste <francis@Contre.COM> 
- First RPM release.

%prep
%setup -q

%build

# this is to make sure all translations are included in the build
unset LINGUAS || :

if [ -x ./configure ]; then
  CFLAGS=$RPM_OPT_FLAGS ./configure --prefix=%{_prefix} --enable-gnome
else 
  CFLAGS=$RPM_OPT_FLAGS ./autogen.sh --prefix=%{_prefix} --enable-gnome
fi  
make

%install
rm -fr $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT%{_prefix} install

gzip --best $RPM_BUILD_ROOT%{_prefix}/man/man1/dia.1

%clean
rm -fr $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README TODO NEWS INSTALL COPYING ChangeLog AUTHORS doc
%{_prefix}/bin/dia
%{_prefix}/lib/dia
%{_prefix}/man/man1/dia.1.gz
%{_prefix}/share/dia
%{_prefix}/share/gnome/apps/Applications/dia.desktop
%{_prefix}/share/locale/*/*/*
%{_prefix}/share/mime-info/*
%{_prefix}/share/pixmaps/*
%doc %{_prefix}/share/gnome/help/dia
# if you are building without gnome support, use /usr/share/dia/help instead
