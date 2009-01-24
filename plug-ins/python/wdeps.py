# Generates a weighted dependencies graph from DLLs
# Copyright (c) 2001, 2005 Hans Breuer <hans@breuer.org>

#    This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

import sys, os, re, string, math, time

g_maxWeight = 1
# a very limited demangling (leaves junk for some forms)
#rDemangle = re.compile("\?(?:\?0)*([^@]+)(?:@@[0ABGHIKPQUVXYZ?1]+|@Z|@)*([^@]*)")
# somewhat better:
# 1) every mangled name starts with '?'
# 2) ctor has '?1' following, dtor has '?0'  and the class@namespaces@@parameters
# 3) other members have '' following function@class_and_namespaces@@parameters

# this is overly simplified: somehow the key appears to have more than one meaning
Operators = { "2" : " new", "3" : " delete", 
			  "4" : "=", "5" : ">>", "6" : "<<", "7" : "!", "8" : "==", "9" : "!=", 
			  "C" : "->", "Z" : "-", "F" : "--", "G" : "-", "H" : "+", "D" : "*", "E" : "++", 
			  "M" : "<", "Y" : "+=", "A" : "[]", "B" : " char const *", "K" : "/" }
rDemangle = re.compile ("\?(?P<tor>\?[01" + string.join(Operators.keys(), "") + "])*(?P<sym>[\w@_]+?(?=@))@@")
# still NOT handling
'''
d:\Projects>undname ?ReleaseAccess@?$CWriteAccess@VCDocEx@app@@@utl@@UAEXXZ
Microsoft (R) C++ Name Undecorator
Copyright (C) Microsoft Corporation 1981-2001. All rights reserved.

Undecoration of :- "?ReleaseAccess@?$CWriteAccess@VCDocEx@app@@@utl@@UAEXXZ"
is :- "public: virtual void __thiscall utl::CWriteAccess<class app::CDocEx>::ReleaseAccess(void)"

Undecoration of :- "??_7CDocMgrView@app@@6B@"
is :- "const app::CDocMgrView::`vftable'"

Undecoration of :- "??_0CUnit@utl@@QAEAAV01@ABV01@@Z"
is :- "public: class utl::CUnit & __thiscall utl::CUnit::operator/=(class utl::CUnit const &)"
'''
# a list of dlls to ignore dependencies of
g_DontFollow = []

class Node :
	def __init__ (self, name) :
		self.name = name
		self.deps = {}
	def AddEdge (self, name, symbols) :
		self.deps[name] = Edge(name, symbols)

class Edge :
	def __init__ (self, name, symbols) :
		global g_maxWeight
		self.name = name # the target
		self.weight = len(symbols)
		if self.weight > g_maxWeight :
			g_maxWeight = self.weight
		demangled = []
		for s in symbols :
			m = rDemangle.match (s)
			if m :
				#print m.group("tor"), "::", m.group("sym")
				dm = ""
				names = string.split (m.group("sym"), "@")
				# we must not append the .reverse() in the line above otherwise names would become None. WTF? Inplace operation?
				names.reverse()
				if m.group("tor") == "?0" : # constructor
					dm =  string.join(names, "::") + "::" + names[-1]
				elif  m.group("tor") == "?1" : # constructor
					dm =  string.join(names, "::") + "::~" + names[-1]
				elif  m.group("tor") != None and m.group("tor")[0] == "?" and m.group("tor")[1] in Operators.keys() : # constructor
					dm =  string.join(names, "::") + "::operator" + Operators[m.group("tor")[1]]
				else :
					dm =  string.join(names, "::")
				#print " => ", dm
				demangled.append (dm)
			else :
				#print " <unmatched>: ", s
				demangled.append (s)
		self.symbols = demangled
		
def GetDeps (sFrom, dAll, nMaxDepth, nDepth=0) :
	"calculates the dependents of the passed in dll"
	if nMaxDepth <= nDepth :
		return
	sFrom = string.lower(sFrom)
	global g_DontFollow
	if sFrom in g_DontFollow :
		return
	if not dAll.has_key (sFrom) :
		node = Node (sFrom)
		f = os.popen ("dumpbin /imports " + sFrom)
		name = None
		arr = []
		sDump = f.readlines ()
		# avoids multiple instances of dumpbin running simultaneously
		f = None
		# avoids infinite recursion on circular dependencies
		dAll[sFrom] = None
		for s in sDump :
			r = re.match ("^    (.*\.dll)", s, re.IGNORECASE)
			if r :
				name = string.lower(r.group(1))
				# print name
			else :
				# import by name
				# The system dlls seem to follow a diferent pattern (or is this for know-dlls?)  thus (?:[0123456789ABCDEF]{8}[ ]+)?
				r2 = re.match ("^[ ]+(?:[0123456789ABCDEF]{8}[ ]+)?[0123456789ABCDEF]{1,5}[ ]+([\w@?$]+)$", s)
				if not r2 :
					r2 = re.match ("^[ ]+(?:[0123456789ABCDEF]{8}[ ]+)?Ordinal[ ]+([1234567890]+)$", s)
				if r2 :
					arr.append (r2.group(1))
				elif s[:-1] == "" and name != None and len(arr) > 0 :
					#print name, len(arr)
					node.AddEdge (name, arr)
					arr = []
					nDepth = nDepth + 1
					GetDeps (name, dAll, nMaxDepth-nDepth+1, nDepth)
					nDepth = nDepth - 1
		# add to all nodes
		dAll[sFrom] = node
	
def Remove (deps, list) :
	"From the deps tree remove the nodes matching list"
	for s in list :
		if deps.has_key (s) :
			del deps[s]
		for sn in deps.keys() :
			node = deps[sn]
			if node.deps.has_key (s) :
				del node.deps[s]
def Sorted (dict) :
	"given a dictionary with name to number, sort by number"
	ret = []
	keys = dict.keys()
	keys.sort ( lambda a, b : cmp(dict[a], dict[b]) )
	for k in keys :
		ret.append ((k, dict[k]))
	return ret
# some predefined sets of DLLs, either for hiding from the dependencies or maybe to tin them
dllsSysWin32 = [
	"version.dll", "winmm.dll", 
	"kernel32.dll", "user32.dll", "gdi32.dll", "comdlg32.dll", "advapi32.dll", "shell32.dll",
	"comctl32.dll", "ole32.dll", "oleaut32.dll", "winspool.drv", "imm32.dll",
	"wsock32.dll", "mpr.dll", 
	"rpcrt4.dll", "shlwapi.dll", "netapi32.dll", "msimg32.dll", "oledlg.dll",
	"uxtheme.dll"]
dllsCrts = [
	"msvcrt.dll", "msvcrtd.dll", "msvcp60.dll",
	"msvcr71.dll", "msvcr71d.dll", "msvcp71.dll"
	]
dllsMfc = [
	"mfc71u.dll", "mfc71.dll"
	]
dllsGtk = [
	"libglib-2.0-0.dll", "libgmodule-2.0-0.dll", "libgobject-2.0-0.dll", "libgthread-2.0-0.dll",
	"libpango-1.0-0.dll", "libpangowin32-1.0-0.dll", "libpangoft2-1.0-0.dll", "libpangocairo-1.0-0.dll",
	"libgdk_pixbuf-2.0-0.dll", "libgdk-win32-2.0-0.dll", "libgtk-win32-2.0-0.dll", "libatk-1.0-0.dll", 
	"libcairo.dll", "libintl-1.dll", "iconv.dll"
	]
def main () :
	deps = {}
	dllsToRemove = []
	nMaxDepth = 10000 # almost unlimited
	bHaveComponents = 0
	bDump = 0
	bByUse = 0
	sOutFilename = None

	nSymbols = 0

	for arg in sys.argv[1:] :
		if string.find (arg, "--remove") == 0 :
			if arg == "--remove-sys" : dllsToRemove.extend (dllsSysWin32)
			elif arg == "--remove-crt" : dllsToRemove.extend (dllsCrts)
			elif arg == "--remove-mfc" : dllsToRemove.extend (dllsMfc)
			elif arg == "--remove-gtk" : dllsToRemove.extend (dllsGtk)
			else :
				noDeps = string.split(arg[len("--remove="):], ",")
				for s in noDeps :
					dllsToRemove.append(s)
		elif string.find (arg, "--dont-follow") == 0 :
			global g_DontFollow
			noFollow = string.split(arg[len("--dont-follow="):], ",")
			g_DontFollow.extend (noFollow)
		elif string.find (arg, "--depth=") == 0 :
			nMaxDepth = int(arg[len("--depth="):])
			if nMaxDepth < 1 : print  "Wrong depth"; sys.exit(1)
		elif string.find (arg, "--symbols=") == 0 :
			nSymbols = int(arg[len("--symbols="):])
			if nSymbols < 0 : nSymbols = 0
		elif arg == "--dump" :
			bDump = 1
		elif arg == "--by-use" :
			bByUse = 1
		elif string.find (arg, "--") == 0 :
			print "Unknown option or missing parameter:", arg
		else :
			if not bHaveComponents :
				components = string.split(arg, ",")
				for s in components:
					GetDeps (s, deps, nMaxDepth)
				bHaveComponents = 1
			else :
				sOutFilename = arg
		print "arg is:", arg 
	if len(sys.argv) < 2 :
		print sys.argv[0], "[parameters] <component,s> [dot-output]"
		print """
Given one or mores starting components like

	a.dll,another.dll

this tool anayzes the dependencies of the DLLs and generates a depency graph
in the dot format (see: www.graphviz.org). This tool requires DUMPBIN from
the MSVC toolchain. With the version included in VC7.1 it is capable to also 
'see' 'delay load imports'. The tool follows the depencies to DLLs in the 
current directory only.

If a second parameter is given the depency graph is written to that file. 
Otherwise output is written to stdout.

There is also a number of extra parameters to fine-tune the graph.
First you can use --remove{-crt,-sys,-mfc} to remove the respective
set of dependencies. There is also a special form of remove 
--remove=another.dll removes excatly that dll from the graph.

Another way to get more manageable graphs is to limit the recursion 
depth while searching, like --depth=2 which cuts everything below 
the second dependency level.

For more information read the source.
"""
		sys.exit(0)

	Remove (deps, dllsToRemove)
	# output ...
	if not sOutFilename :
		f = sys.stdout
	else :
		# ... dot
		f = open(sOutFilename, "w")
	
	if bDump :
		Symbols = {}
		Modules = {}
		Imports = {}
		for sn in deps.keys() :
			node = deps[sn]
			if len(node.deps.keys()) == 0 :
				continue
			print sn
			Imports[sn] = 0
			for se in node.deps.keys() :
				edge = node.deps[se]
				print "\t", node.name, "->", edge.name
				syms = edge.symbols
				syms.sort()
				for sy in syms :
					print "\t\t", sy
					if Symbols.has_key(sy) : 
						Symbols[sy] += 1 
					else : 
						Symbols[sy] = 1
				Imports[sn] += len(syms)
				if Modules.has_key (edge.name) :
					Modules[edge.name].append (node.name)
				else :
					Modules[edge.name] = [node.name]
		# symbols used by many modules are good candidates for refactoring
		print "***** Modules with users (symbols) *****"
		for s, i in Sorted (Imports) :
			print s, "(" + str(i) + ") :" , string.join (Modules[s], ",")
		# no diagram at all
		sys.exit(0)
	f.write ('digraph "' + components[0] + '" {\n')
	f.write ('graph [fontsize=24.0 label="wdeps.py ' + string.join (sys.argv[1:], " ") 
			+ '\\n' + time.ctime() + '"]\n') 
	f.write ('ratio=0.7\nnode [fontsize=32.0 ]\n')
	if bByUse :
		# kind of inverted diagram not showing edependencies but 'users'
		users = {}
		for sn in deps.keys() :
			users[sn] = []
		for sn in deps.keys() :
			node = deps[sn]
			for se in node.deps.keys() :
				edge = node.deps[se]
				users[edge.name].append(node.name)
		for sn in users.keys() :
			for s in users[sn] :
				f.write ('"%s" -> "%s"\n' % (sn, s))
	else :
		# first pass mark don't follows
		dontFollowsDone = {}
		for sn in deps.keys() :
			node = deps[sn]
			for se in node.deps.keys() :
				if se in g_DontFollow :
					if not dontFollowsDone.has_key(se) :
						# mark as such
						f.write ('"%s" [style=filled,color=lightgray]\n' % (se,))
						dontFollowsDone[se] = 1
					
		for sn in deps.keys() :
			# write weighted edges, could also classify the nodes ...
			node = deps[sn]
			for se in node.deps.keys() :
				edge = node.deps[se]
				if edge.weight <= nSymbols :
					#f.write ('"%s" -> "%s" [weight=%f,label=%s]\n' % (node.name, edge.name, math.log(1)-0.5, edge.symbols[0]))
					f.write ('"%s" -> "%s" [fontsize=8,label="%s",weight=%f]\n' 
							% (node.name, edge.name, string.join(edge.symbols, "\\n"), math.log10(edge.weight)))
				else :
					#f.write ('"%s" -> "%s" [weight=%f]\n' % (node.name, edge.name, math.log(edge.weight)-0.5))
					f.write ('"%s" -> "%s" [label="(%d)",weight=%f]\n' 
							% (node.name, edge.name, edge.weight, math.log10(edge.weight)))
	f.write("}\n")

if __name__ == '__main__': main()
