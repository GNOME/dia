%define name dia
# This is the full Dia version
%define ver 0.96

Summary: A gtk+ based diagram creation program.
Name: %name
Version: %ver
# This indicates changes to the spec file after last time %ver has changed.
Release: 1
Copyright: GPL
Group: Applications/
Source: ftp://ftp.gnome.org/pub/GNOME/stable/sources/dia/%{name}-%{ver}.tar.gz
URL: http://www.gnome.org/projects/dia/
BuildRoot: /var/tmp/%{name}-%{ver}-root

Requires: libxml2 >= 2.3.9 gtk2 >= 2.6.0 pango >= 1.1.5 freetype2 >= 2.0.9
Requires: libgnome libgnomeui popt
Requires: python python-gtk
BuildRequires: libxml2-devel >= 2.3.9 gtk2-devel >= 2.6.0 pango-devel >= 1.1.5 
BuildRequires: freetype2-devel >= 2.0.9 intltool > 0.21
BuildRequires: libgnome-devel libgnomeui-devel popt-devel
BuildRequires: python python-gtk

%description
Dia is a GNU program designed to be much like the Windows
program 'Visio'. It can be used to draw different kind of diagrams.

It can be used to draw a variety of diagram types, including UML, Network,
flowchart and others.  The native file format for Dia is XML (optionally
gzip compressed).  It has print support, and can export to a number of formats such as EPS, SVG, CGM and PNG.

%changelog
* Mon Mar 29 2004 Lars Clausen <lars@raeder.dk>
- change url to www.gnome.org, fix freetype2 stuff, add python and gnome
- dependencies
* Thu Feb 4 2003 Lars Clausen <lrclause@cs.uiuc.edu>
- update requirements.
- update prerelease number, move to release field.
* Thu Jan 30 2003 Lars Clausen <lrclause@cs.uiuc.edu>
- update version number.
* Sat Jun 1 2002  Cyrille Chepelov <cyrille@chepelov.org>
- update version number.
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
%setup -q -n %name-%ver-%release

%build

# this is to make sure all translations are included in the build
unset LINGUAS || :

if [ -x ./configure ]; then
  CFLAGS=$RPM_OPT_FLAGS ./configure --prefix=%{_prefix} --enable-gnome --with-python
else 
  CFLAGS=$RPM_OPT_FLAGS ./autogen.sh --prefix=%{_prefix} --enable-gnome --with-python
fi  
make

%install
rm -fr $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT%{_prefix} install

rm -rf $RPM_BUILD_ROOT/%{_prefix}/var

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
%{_prefix}/share/applications/dia.desktop
%{_prefix}/share/locale/*/*/*
%{_prefix}/share/mime-info/*
%{_prefix}/share/pixmaps/*
%doc %{_prefix}/share/gnome/help/dia
# if you are building without gnome support, use /usr/share/dia/help instead
