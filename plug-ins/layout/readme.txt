A Dia plug-in offering automatic layout facilities
--------------------------------------------------

This plug-on is based on OGDF (www.ogdf.net) - v. 2012.07 (Sakura).
"OGDF is a self-contained C++ class library for the automatic layout of diagrams."

The plug-in uses a self-devolved wrapper over OGDF (ogdf-simple(h|cpp)), mostly to 
allow a special build setup on win32. But IMO the plug-in also benefits of depending
on a much smaller API. This way it should be easy to plug in other layout libraries
without too much trouble.

Build without OGDF
------------------
Don't define HAVE_OGDF and layout.cpp gets compiled without OGDF dependency.
This does not give any of the automatic layout facility, but should be useful
to keep the plug-in buildable when Dia specifics change.

Build setup on win32
--------------------
Dia's standard compiler on win32 is vc6, which does not support enough C++ to compile OGDF.
There is not only the problem of compile time, but also at runtime there should not be two
C++ runtimes in one process. And of course we should spare the burden of msvcrXX.dll 
distribution.
The header ogdf-simple.h defines a C++ version agnostic interface (no exceptions, no 
allocation details accross DLL boundaries) which makes OGDF build by any VC++ compiler
useable in the context of Dia.
Simply copy ogdf-simple.(h|cpp) and makefile.msc into the OGDF/src directory and call
	nmake -f makefile.msc
After building OGDF.dll configure your build setup in $(TOP)\glib\build\win32\make.msc to 
define include path and library linkage, mine has:

OGDF_CFLAGS = -I $(TOP)\..\other\ogdf\src -DHAVE_OGDF
OGDF_LIBS = $(TOP)\..\other\ogdf\src\ogdf.lib
	

Build on *NIX
-------------
 * ogdf_prefix is hard-coded to ~/devel/OGDF
 * HAVE_OGDF is defined 
 * OGDF_CFLAGS defined by configure
 * OGDF_LIBS defined by configure

/~/devel/OGDF/_release/libOGDF.a(CombinatorialEmbedding.o): relocation R_X86_64_32 against `.bss' 
	can not be used when making a shared object; recompile with -fPIC
add
	compilerParams = -fPIC -I.
to makeMakefile.config, run
	./makeMakefile.sh

