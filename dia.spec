%define prefix /usr

Summary: A gtk+ based diagram creation program.
Name: dia
Version: 0.41
Release: 1
Copyright: GPL
Group: Applications/
Source: http://www.lysator.liu.se/~alla/dia/%{name}-%{version}.tar.gz
URL: http://www.lysator.liu.se/~alla/dia/dia.html
Packager: Francis J. Lacoste <francis@Contre.COM>
BuildRoot: /var/tmp/%{name}-%{version}-root

%description
Dia is a program designed to be much like the Windows
program 'Visio'. It can be used to draw different kind of diagrams. In
this first version there is support for UML static structure diagrams
(class diagrams) and Network diagrams. It can currently load and save
diagrams to a custom fileformat and export to postscript.

%changelog
* Sun Sep 5 1999  James Henstridge <james@daa.com.au>
- added $(prefix)/share/dia to files list.
* Thu Apr 29 1999  Enrico Scholz <enrico.scholz@wirtschaft.tu-chemnitz.de>
- Made %setup quiet
- Enabled build from cvs
- Removed superfluous mkdir's
- using DESTDIR and install-strip
* Fri Aug 28 1998  Francis J. Lacoste <francis@Contre.COM> 
- First RPM release.

%prep
%setup -q -n %{name}

%build
if [ -x ./configure ]; then
  CFLAGS=$RPM_OPT_FLAGS ./configure --prefix=%{prefix} --enable-gnome
else 
  CFLAGS=$RPM_OPT_FLAGS ./autogen.sh --prefix=%{prefix} --enable-gnome
fi  
make

%install
rm -fr $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install-strip 

%clean
rm -fr $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README TODO NEWS INSTALL COPYING ChangeLog AUTHORS
%{prefix}/bin/dia
%{prefix}/lib/dia
%{prefix}/share/dia

