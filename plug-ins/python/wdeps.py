# Generates a weighted dependencies graph from DLLs
# Copyright (c) 2001, 2005, 2007 Hans Breuer <hans@breuer.org>

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

import sys, os, re, math, time

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
rDemangle = re.compile (r"\?(?P<tor>\?[01" + "".join(list(Operators.keys())) + r"])*(?P<sym>[\w@_]+?(?=@))@@")
# still NOT handling
r'''
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

def Demangle (symbols) :
	demangled = []
	for s in symbols :
		m = rDemangle.match (s)
		if m :
			#print m.group("tor"), "::", m.group("sym")
			dm = ""
			names = m.group("sym").split ("@")
			# we must not append the .reverse() in the line above otherwise names would become None. WTF? Inplace operation?
			names.reverse()
			if m.group("tor") == "?0" : # constructor
				dm = "::".join(names) + "::" + names[-1]
			elif  m.group("tor") == "?1" : # constructor
				dm = "::".join(names) + "::~" + names[-1]
			elif  m.group("tor") != None and m.group("tor")[0] == "?" and m.group("tor")[1] in list(Operators.keys()) : # constructor
				dm = "::".join(names) + "::operator" + Operators[m.group("tor")[1]]
			else :
				dm = "::".join(names)
			#print " => ", dm
			demangled.append (dm)
		else :
			#print " <unmatched>: ", s
			demangled.append (s)
	return demangled

class Node :
	def __init__ (self, name, depth) :
		self.name = name
		self.deps = {}
		self.depth = depth # where we found it
		# symbol -> use-count
		self.exports = {}
	def AddEdge (self, name, symbols, delayLoad) :
		self.deps[name] = Edge(name, symbols, delayLoad)
	def AddExports (self, symbols, used=0) :
		''' remember the symbols this node exports (and their use)'''
		for s in symbols :
			if s in self.exports :
				self.exports[s] += used
			else :
				self.exports[s] = used

class Edge :
	def __init__ (self, name, symbols, delayLoad) :
		global g_maxWeight
		self.name = name # the target
		self.reduce = 0 # != 0 if this should better vanish
		self.delayLoad = delayLoad
		if len(symbols) > g_maxWeight :
			g_maxWeight = len(symbols)
		self.symbols = Demangle(symbols)
	def Weight (self) :
		"As method to have let always be the number of symbols"
		return len(self.symbols)
g_path = None # empty, default behaviour
def FindInPath (sName) :
	"returns the complete path of the given name, looking at cwd first"
	sDir = os.getcwd()
	p = sDir + os.sep + sName
	if os.path.exists (p) :
		return p
	myPath = os.environ['PATH']
	if g_path :
		myPath = g_path
	for sDir in myPath.split (os.pathsep) :
		p = sDir + os.sep + sName
		if os.path.exists (p) :
			return p
	# safety
	return sName

def GetDepsDotnet (sFrom, dAll, node, nMaxDepth, nDepth=0) :
	""" passing in a node allows to check C++ and .Net dependecies """
	try :
		import clr
		clr.AddReference("IronPython")
	except ImportError :
		print("No Iron!")
		return
	from System.Reflection import Assembly

	if sFrom not in dAll or node :
		if not node :
			node = Node (sFrom, nDepth)
			dAll[sFrom] = node
		try :
			# assembly = Assembly.LoadFrom(sFrom)
			# the former does not show C++ -> .Net connections
			assembly = Assembly.ReflectionOnlyLoadFrom(sFrom)
		except IOError :
			return
		except SystemError :
			return
		symbols = []

		try :
			types = assembly.GetExportedTypes()
		except IOError :
			# some failed dependencies are showing up here?
			return
		for t in types :
			#if not t.IsPublic :
			#	continue
			if t.IsClass or t.IsInterface :
				members = t.GetMembers()
				for  m in members :
					if m.Module.Assembly == assembly :
						#print "Exp:", sym
						pass
					elif t.IsImport :
						#print "Imp:", t.Name + "." + m.Name + "()"
						sym = t.Name + "." + m.Name + "()"
						symbols.append (sym)
		# we may want to look at all refernced assemblies ...
		refs = assembly.GetReferencedAssemblies()
		for r in refs :
			n2 = r.Name + ".dll";
			print("\t" * nDepth + n2, r.Version)
			symbols = [ "[" + str(r.Version) + "]" ]
			node.AddEdge (n2, symbols, 1) # delayLoad: kind of
			GetDepsDotnet (n2, dAll, None, nMaxDepth, nDepth+1)

def GetDepsWin32 (sFrom, dAll, nMaxDepth, nDepth=0) :
	"calculates the dependents of the passed in dll"
	if nMaxDepth <= nDepth :
		return
	sFrom = sFrom.lower()
	global g_DontFollow
	if sFrom in g_DontFollow :
		dAll[sFrom] = Node (sFrom, nDepth) # needed here so other algoritm can remove it ;)
		return
	if sFrom not in dAll :
		node = Node (sFrom, nDepth)
		sPath = sFrom
		if g_path :
			sPath = FindInPath (sFrom)

		# first remember all exports
		arr = []
		f = os.popen ('dumpbin /exports "' + sPath + '"')
		sDump = f.readlines()
		for s in sDump :
			# similiar regex as above, but we have three numbers
			r2 = re.match (r"^[ ]+(?:[0123456789ABCDEF]{1,5}[ ]+)(?:[0123456789ABCDEF]{1,5}[ ]+)?[0123456789ABCDEF]{8}[ ]+([\w@?$]+).*", s)
			if r2 :
				arr.append (r2.group(1))
		node.AddExports (arr, 0)

		f = os.popen ('dumpbin /imports "' + sPath + '"')
		name = None
		arr = []
		sDump = f.readlines ()
		# avoids multiple instances of dumpbin running simultaneously
		f = None
		# avoids infinite recursion on circular dependencies
		dAll[sFrom] = None
		delayLoad = 0
		directDeps = []
		for s in sDump :
			r = re.match (r"^    (.*\.dll)", s, re.IGNORECASE)
			# the delay load switch is flipped once
			if s.find ("delay load imports") > 0 :
				delayLoad = 1
			if r :
				name = r.group(1).lower()
				# print name
			else :
				# import by name
				# The system dlls seem to follow a diferent pattern (or is this for know-dlls? Delay load?)  thus (?:[0123456789ABCDEF]{8}[ ]+)?
				r2 = re.match (r"^[ ]+(?:[\dA-F]{8}[ ]+)?[\dA-F]{1,5}[ ]+([\w@?$]+)$", s)
				if not r2 :
					r2 = re.match (r"^[ ]+(?:[\dA-F]{8}[ ]+)?Ordinal[ ]+([\d]+)$", s)
				if r2 :
					arr.append (r2.group(1))
				elif s[:-1] == "" and name != None and len(arr) > 0 :
					#print name, len(arr)
					node.AddEdge (name, arr, delayLoad)
					arr = []
					directDeps.append(name)
					GetDepsWin32 (name, dAll, nMaxDepth-nDepth+2, nDepth+1)
		# add to all nodes
		dAll[sFrom] = node

		# check for dotnet dependencies
		if "mscoree.dll" in directDeps :
			print("Dotnet check ...", sFrom)
			GetDepsDotnet (sFrom, dAll, node, nMaxDepth-nDepth+2, nDepth+1)

		# restore original depth (independent of how the recursion works)
		for sd in directDeps :
			if sd in list(dAll.keys()) :
				try :
					dAll[sd].depth = nDepth + 1
				except AttributeError as msg :
					print("FIXME:", sd, msg)

def GetDepsPosix (sFrom, dAll, nMaxDepth, nDepth=0) :
	"calculates the dependents of the passed in so"
	# sFrom must be with absolute on Posix, still we prefer the shorter name
	sFromName = os.path.split(sFrom)[-1]
	if nMaxDepth <= nDepth :
		return
	global g_DontFollow
	if sFromName in g_DontFollow :
		dAll[sFromName] = Node (sFromName, nDepth) # needed here so other algoritm can remove it ;)
		return
	if not sFromName in list(dAll.keys()) :
		print("Creating", sFromName, nDepth)
		node = Node (sFromName, nDepth)
		sPath = sFrom
		#TODO: work with relative pathes? Current dir?
		if os.sys.platform in ['darwin'] :
			f = os.popen ('dyldinfo -dylibs "' + sPath + '"')
		else :
			f = os.popen ('ldd "' + sPath + '"')
		sModules = f.readlines ()
		# avoids multiple instances of dumpbin running simultaneously
		f = None
		# avoids infinite recursion on circular dependencies
		dAll[sFromName] = None
		# sFrom imports
		# --extern-only : -g
		# --undefined-only : -u
		# ? --defined-only : -U
		if os.sys.platform in ['darwin'] :
			fImports = os.popen ('nm -g -u ' + sPath)
		else :
			fImports = os.popen ('nm --extern-only --dynamic --undefined-only ' + sPath)
		sImports = fImports.readlines()
		dImports = {}
		for si in sImports :
			if os.sys.platform in ['darwin'] :
				if len(si) > 1 :
					dImports[si[:-1]] = 1
			else :
				mi = re.match(r"\s+U (\S+)", si)
				if mi :
					print(mi.group(1))
					dImports[mi.group(1)] = 1
		toRecurse = []
		for s in sModules :
			if os.sys.platform in ['darwin'] :
				r = re.match (r"\s+(?P<path>\S+).*", s)
			else :
				r = re.match (r"\s+(?P<name>\S+) => (?P<path>\S+).*", s)
			if r :
				# print r.group("name"), r.group("path")
				# now find symbols between sFrom and any of the SOs
				usedSymbols = []
				unusedSymbols = []
				if os.sys.platform in ['darwin'] :
					fExports = os.popen ('nm -g -U ' + r.group("path"))
				else :
					fExports = os.popen ('nm --extern-only --dynamic --defined-only ' + r.group("path"))
				sExports = fExports.readlines()
				for se in sExports :
					me = re.match (r"\w+ T (\S+)", se)
					if me :
						symbol = me.group(1)
						# print symbol
						# print me.group(1), r.group('name')
						if symbol in list(dImports.keys()) :
							# direct connection
							usedSymbols.append (symbol)
						else :
							unusedSymbols.append(symbol)
				# now we have complete symbols for node r.group('name'),
				# should remember the node although it may not be connected yet
				# OTOH the recursion would become more difficult to understand, favor KISS
				if len(usedSymbols) > 0 :
					try :
						node.AddEdge(r.group("name"), usedSymbols, 0)
					except IndexError : # darwin
						sToPath = r.group("path")
						node.AddEdge(sToPath[sToPath.rfind('/')+1:], usedSymbols, 0)
					toRecurse.append(r.group("path"))
					print(sFromName, "(", nDepth, "):", r.group("path"))
		# add to all nodes
		dAll[sFromName] = node
		for sd in toRecurse :
			print(sFromName, "->", sd, "level:", nDepth+1)
			sdn = os.path.split(sd)[-1]
			GetDepsPosix (sd, dAll, nMaxDepth-nDepth+2, nDepth+1)
			if sdn in list(dAll.keys()) :
				#print "Adjusting depth", sd, nDepth+1
				dAll[sdn].depth = nDepth+1 # adjust to original depth (not how our recursion found it)

def IsWin32 () :
	return os.name in ["nt"]

def Remove (deps, list) :
	"From the deps tree remove the nodes matching list"
	for s in list :
		if s in deps :
			del deps[s]
		for sn in list(deps.keys()) :
			node = deps[sn]
			if s in node.deps :
				del node.deps[s]
def RemoveRegEx (deps, reg_ex) :
	"If a module matches reg_ex remove it"
	import re
	rr = re.compile (reg_ex)
	for k1 in list(deps.keys()) :
		if rr.match (k1) :
			del deps[k1]
		else :
			node = deps[k1]
			for k2 in list(node.deps.keys ()) :
				if rr.match (k2) :
					del node.deps[k2]
def RemoveNonLocal (deps) :
	"Remove everything not available in the current directory"
	for k in list(deps.keys()) :
		node = deps[k]
		for c in list(node.deps.keys()) :
			if not os.path.exists (c) :
				del node.deps[c]
	# also remove from the root
	root_keys = list(deps.keys())
	for k in root_keys :
		if not os.path.exists (k) :
			del deps[k]

def RemoveBySymbols (deps, list) :
	"If a connection is conly caused by some symbol in 'list' it is removed"
	for k in list(deps.keys()) :
		node = deps[k]
		for c in list(node.deps.keys ()) :
			edge = node.deps[c]
			kills = []
			for s2 in edge.symbols :
				for s1 in list :
					# robustness, dont compare the empty string
					if s1 == "" : continue
					# only comparing the start of the symbol
					if s2.find (s1) == 0 :
						#print "removing", s2, "from", c, "->", k
						kills.append (s2)
						#NOT modifying while iterating: edge.symbols.remove (s2)
						break
			if len(edge.symbols) == len(kills) :
				# remove complete edge (maybe we should only mark it removed?)
				#print "removing", c, "from", k
				del node.deps[c]
			else :
				for s in kills :
					edge.symbols.remove (s)
		# remove the symbols also from the dependency edge is pointing to
		for s in list :
			if s in list(node.exports.keys()) :
				del node.exports[s]

def Reduce (deps, f, bHintOnly = 1) :
	"Automatically remove connections until there is only something reasonable left"
	# first iteration: two components are connected in both directions
	# if one connection is much weaker than the other one, the weaker one is considered bad
	# if both have similar weight they are both treated as bad and removed
	keys = list(deps.keys())
	keys.sort()
	nReduced = 0
	for k in keys :
		node = deps[k]
		for c in list(node.deps.keys ()) :
			edge = node.deps[c]
			if c in deps :
				node2 = deps[c]
				if k in node2.deps :
					edge2 = node2.deps[k]
					# if one already is reduced dont do it again
					if edge.reduce or edge2.reduce :
						continue
					n = len(edge.symbols)
					n2 = len(edge2.symbols)
					if  abs(n - n2) < 3 :
						f.write ("Remove (" + str(n) + "," + str(n2) +  ") " + edge.name + " <-> " + edge2.name +
							"\n\t" + ", ".join (edge.symbols) +
							"\n\t" + ", ".join (edge2.symbols) + "\n")
						# if we now remove *both* connections some components could become completely disconnected.
						# Instead may try some more guessing. Maybe the component with less deps should drop one more ?
						if bHintOnly :
							edge.reduce = 1
							edge2.reduce = 1
						else :
							if c in node.deps : del node.deps[c]
							if k in node2.deps : del node2.deps[k]
						nReduced += (n + n2)
					elif n > n2 :
						f.write ("Remove (" + str(n2) + ") " + edge.name + " -> " + edge2.name +
							"\n\t" + ", ".join (edge2.symbols) + "\n")
						if bHintOnly :
							edge2.reduce = 1
						else :
							if k in node2.deps : del node2.deps[k]
						nReduced += n2
					else :
						f.write("Remove (" + str(n) + ") " + edge2.name + " -> " + edge.name +
							"\n\t" + ", ".join (edge.symbols) + "\n")
						if bHintOnly :
							edge.reduce = 1
						else :
							if c in node.deps : del node.deps[c]
						nReduced += n
	f.write("Remove total: %d\n" % (nReduced))

def CutLeafs (deps, nCuts) :
	"Remove components without connections. After some iterations only circulars remain (or nothing)"
	leafs = {}
	cut = [] # what we have cut in the last pass
	while nCuts > 0 :
		cut = list(leafs.keys())
		leafs = {}
		# first pass: find leafs
		for k in list(deps.keys()) :
			node = deps[k]
			if len(list(node.deps.keys ())) == 0 :
				leafs[k] = 0
		if len(list(leafs.keys())) == 0 :
			break # nothing remaining
		nCuts -= 1
		# second pass : remove references to leafs
		for k in list(deps.keys()) :
			node = deps[k]
			for c in list(leafs.keys()) :
				if c in node.deps :
					leafs[c] += 1
					del node.deps[c]
		# third pass: remove the leafs
		for k in list(leafs.keys()) :
			# we either need to reset leafs in every round or
			if k in deps :
				del deps[k]
	if len(list(leafs.keys())) > 0 :
		return list(leafs.keys())
	return cut
def TransitiveReduction (deps, tops, treds=[]) :
	"""If a component A has B and C as dependency and B also depends on C reduce A->C.
	   This algorithm is available with dot, too. But here we can adjust the respective
	   'inherited' symbols and 'depth' as well.
	"""
	for a in tops :
		bs = list(deps[a].deps.keys())
		cs = {}
		for b in bs :
			for c in list(deps[b].deps.keys()) :
				if not c in list(cs.keys()) :
					cs[c] = 1
		# bs and cs are now complete, remove all a->b, where b in cs
		extra_syms = {}
		for c in list(cs.keys()) :
			if c in bs :
				print("Cut", a, "->", c)
				# we need to know which edge a->b is taking the extra symbols
				extra_syms[c] = deps[a].deps[c].symbols
				del deps[a].deps[c]
		# fill remaining bs having a c dependency with the extra symbols
		e_keys = list(extra_syms.keys())
		for e in e_keys :
			bs = list(deps[a].deps.keys()) # they are reduced above
			for b in bs :
				if e in list(deps[b].deps.keys ()) :
					print("Adjust", a, "->", b, "+", len(extra_syms[e]), "from", e)
					#NOT: deps[a].deps[b].symbols.extend(extra_syms[e]) # producing duplicates
					for s in extra_syms[e] :
						if not s in deps[a].deps[b].symbols :
							deps[a].deps[b].symbols.append(s)
					del extra_syms[e]
					break
		#FIXME: something we have cut before may not be visible anymore (would need a deeper search or an ordered reduction)
		if len(list(extra_syms.keys())) :
			print("Not adjusted:", a, "->", ", ".join(list(extra_syms.keys())))
	# recurse into remaining bs
	for a in tops :
		bs = list(deps[a].deps.keys())
		treds.append(a) # rember already done to avoid endless recursion
		TransitiveReduction (deps, bs, treds)

def TopMost (deps) :
	"everything not used by something else"
	keys = list(deps.keys ())
	topmost = []
	used = {}
	for k in keys :
		for d in list(deps[k].deps.keys()) :
			if d not in used :
				used[d] = 1
	for k in keys :
		if k not in used :
			topmost.append (k)
	return topmost

def RecalculateDepth (deps) :
	"From the top-most - i.e. unused - down to the bottom level "
	keys = list(deps.keys ())
	used = {}
	tops = []
	depth = 0
	while 1 :
		top = keys
		for k in keys :
			for d in list(deps[k].deps.keys()) :
				used[d] = 1
		for k in keys :
			if not k in list(used.keys ()) :
				tops.append (k)
		if len(tops) == 0 :
			break
		for k in tops :
			deps[k].depth = depth
		keys = [x for x in keys if not x in tops]
		tops = []
		depth += 1

def CalculateUsed (deps) :
	"Given the complete dependency tree calcualte the use count of every symbol"
	for sn in list(deps.keys()) :
		node = deps[sn]
		edge_keys = list(node.deps.keys())
		for se in edge_keys :
			edge = node.deps[se]
			target = deps[se]
			# every edges symbols
			target.AddExports(edge.symbols, 1)

def UnGlob (comps) :
	import glob
	comp_dict = {}
	for p in comps :
		g = glob.glob (p)
		for d in g :
			if d not in comp_dict :
				comp_dict[d] = 1
	return list(comp_dict.keys())

def Sorted (dict) :
	"given a dictionary with name to number, sort by number"
	ret = []
	keys = list(dict.keys())
	keys.sort ( lambda a, b : cmp(dict[a], dict[b]) )
	for k in keys :
		ret.append ((k, dict[k]))
	return ret
# some predefined sets of DLLs, either for hiding from the dependencies or maybe to tin them
dllsSysWin32 = [
	"version.dll", "winmm.dll",
	"kernel32.dll", "user32.dll", "gdi32.dll", "comdlg32.dll", "advapi32.dll", "shell32.dll",
	"comctl32.dll", "ole32.dll", "oleaut32.dll", "winspool.drv", "imm32.dll",
	"ws2_32.dll", "ws2help.dll", "wsock32.dll", "ntdll.dll", "mpr.dll",
	"rpcrt4.dll", "shlwapi.dll", "netapi32.dll", "msimg32.dll", "oledlg.dll",
	"setupapi.dll", "secur32.dll", "avifil32.dll", "msvfw32.dll",
	"dbghelp.dll", "uxtheme.dll", "dnsapi.dll", "activeds.dll", "crypt32.dll",
	"mscoree.dll", "psapi.dll", "msi.dll", "mscms.dll", "glu32.dll", "hid.dll", "imagehlp.dll",
	"powrprof.dll", "quartz.dll", "wininet.dll", "wintrust.dll", "riched20.dll" , "odbc32.dll",
	"oleacc.dll", "shfolder.dll", "opengl32.dll"
	]
dllsCrts = [
	"msvcrt.dll", "msvcrtd.dll", "msvcp60.dll",
	"msvcrt20.dll",
#	"msvcr70.dll", "msvcp70.dll",
	"msvcr70.dll", "msvcr70d.dll", "msvcp70.dll", "msvcp70d.dll",
	"msvcr71.dll", "msvcr71d.dll", "msvcp71.dll", "msvcp71d.dll",
	"msvcr80.dll", "msvcr80d.dll", "msvcp80.dll", "msvcp80d.dll",
	"msvcr90.dll", "msvcr90d.dll", "msvcp90.dll", "msvcp90d.dll", "vcomp90.dll", "vcomp90d.dll",
	"msvcr100.dll", "msvcr100d.dll", "msvcp100.dll", "msvcp100d.dll", "vcomp100.dll", "vcomp100d.dll",
	# only one on Linux? (or am I missing th C++rt, and what about librt.so.1?)
	"libc.so.5", "libc.so.6"
	]
dllsMfc = [
	"mfc42u.dll", "mfc42.dll", "mfc42ud.dll",
	"mfc70u.dll", "mfc70.dll", "mfc70ud.dll",
	"mfc71u.dll", "mfc71.dll", "mfc71ud.dll",
	"mfc80u.dll", "mfc80.dll", "mfc80ud.dll", "atl80.dll",
	"mfc90u.dll", "mfc90.dll", "mfc90ud.dll",
	"mfc100u.dll", "mfc100.dll", "mfc100ud.dll"
	]
dllsGtk = [
	"libglib-2.0-0.dll", "libgmodule-2.0-0.dll", "libgobject-2.0-0.dll", "libgthread-2.0-0.dll",
	"libpango-1.0-0.dll", "libpangowin32-1.0-0.dll", "libpangoft2-1.0-0.dll", "libpangocairo-1.0-0.dll",
	"libgdk_pixbuf-2.0-0.dll", "libgdk-win32-2.0-0.dll", "libgtk-win32-2.0-0.dll", "libatk-1.0-0.dll",
	"libcairo.dll", "libintl-1.dll", "iconv.dll"
	]

# Tinting themes
colorMatcher = []
tangoColors = [
	"white", # noone is going to ask for it (start of graph)
	"#fce94f", # butter   (light occer)
	"#fcaf3e", # orange
	"#e9b96e", # chocolate (light brown)
	"#8ae234", # chameleon (green)
	"#729fcf", # sky blue
	"#ad7fa8", # plum
	# leaving out red, now getting darker
	"#edd400",
	"#f57900",
	"#c17d11",
	"#73d216",
	"#3465a4",
	"#75507b",
	# even darker
	"#c4a00",
	"#ce5c00",
	"#8f590f",
	"#4ce9a06",
	"#204a87",
	"#5c3566",
]
def ParseRegexTheme (fname) :
	global colorMatcher
	rkv = re.compile(r'^"(?P<regex>[^"]+)"\s*=\s*(?P<color>[\w#]+).*')
	f = open (fname, 'r')
	for s in f.readlines() :
		m = rkv.match(s)
		if m :
			try :
				sr = re.compile(m.group('regex'))
				sc = m.group('color')
				colorMatcher.append ((sr, sc))
			except re.error as msg :
				print("Parser error\n", s)
				sys.exit(2)
	if len(colorMatcher) == 0 :
		print("no colors parsed:", fname)
		sys.exit(3)
	else :
		print(len(colorMatcher), "colors parsed:", fname)

def LookupColor (node) :
	'''Used to tint the nodes by layer '''
	global colorMatcher
	if len(colorMatcher) :
		for rx, color in colorMatcher :
			m = rx.match(node.name)
			if m and m.start() == 0 and m.end() == len(node.name) :
				#print color, node.name
				return color
		return "white"
	# tinting by palette
	try :
		return tangoColors[node.depth]
	except KeyError as msg :
		print(msg)
		return "#ef2929" # scarlet red
	except IndexError as msg :
		print("Dependency", node.name, 'level', node.depth)
		return "#cc0000" # scarlet ret (a bit darker)

def DumpSymbols (deps, f) :
	Symbols = {}
	Modules = {}
	Imports = {}
	node_keys = list(deps.keys())
	node_keys.sort()
	for sn in node_keys :
		node = deps[sn]
		if len(list(node.deps.keys())) == 0 :
			continue
		f.write (sn + "\n")
		Imports[sn] = 0
		edge_keys = list(node.deps.keys())
		edge_keys.sort()
		for se in edge_keys :
			edge = node.deps[se]
			if edge.reduce :
				f.write ("\t!" + node.name + " -> " + edge.name + "\n")
				continue
			if edge.delayLoad :
				f.write ("\t" + node.name + " -> " + edge.name + " (delay load)\n")
			else :
				f.write ("\t" + node.name + " -> " + edge.name + "\n")
			syms = edge.symbols
			syms.sort()
			for sy in syms :
				f.write ("\t\t" + sy + "\n")
				if sy in Symbols :
					Symbols[sy] += 1
				else :
					Symbols[sy] = 1
			Imports[sn] += len(syms)
			if edge.name in Modules :
				Modules[edge.name].append (node.name)
			else :
				Modules[edge.name] = [node.name]
	# symbols used by many modules are good candidates for refactoring
	f.write ( "***** Modules with users (symbols) *****\n" )
	for s, i in Sorted (Imports) :
		if s in Modules :
			f.write (s + " (" + str(i) + ") : " + ",".join (Modules[s]) + "\n")
		else :
			f.write (s + " (0) : <no users>\n")

def DumpUnusedSymbols (deps, f) :
	for sd in list(deps.keys ()) :
		node = deps[sd]
		for se in list(node.deps.keys()) :
			edge = node.deps[se]
			d = deps[se]
			for ss in edge.symbols :
				if ss in list(d.exports.keys()) :
					del d.exports[ss]
	# after we have removed *all* used symbols
	for sd in list(deps.keys ()) :
		node = deps[sd]
		unused = list(node.exports.keys())
		f.write (sd + "\n")
		unused.sort ()
		for ss in unused :
			f.write ("\t" + ss + "\n")

def DumpSymbolsUse (deps, f) :
	f.write ("*** Symbol Use Count ***\n")
	CalculateUsed (deps)
	node_keys = list(deps.keys())
	node_keys.sort()
	for sn in node_keys :
		node = deps[sn]
		used = 0
		for k, v in node.exports.items() :
			if v > 0 :
				used += 1
		f.write (node.name + " (%d:%d)\n" % (used, len(node.exports)))
		sorted_symbols = Sorted(node.exports)
		for sym, cnt in sorted_symbols :
			try :
				f.write ("\t%d\t%s\n" % (cnt, sym))
			except KeyError :
				f.write ("\t?\t" + sym + "\n")

def DumpReverse (deps, f) :
	node_keys = list(deps.keys ())
	edges = {}
	for sn in node_keys :
		node = deps[sn]
		edge_keys = list(node.deps.keys ())
		for se in edge_keys :
			if se in list(edges.keys ()) :
				edges[se].append ((node.deps[se], sn))
			else :
				edges[se] = [(node.deps[se], sn)]
	edge_keys = list(edges.keys ())
	edge_keys.sort ()
	for se in edge_keys :
		f.write (se + "\n")
		for ed in edges[se] :
			f.write ("\t" + se + " <- " + ed[1] + "\n")
			syms = ed[0].symbols
			syms.sort ()
			for sym in syms :
				f.write ("\t" * 2 + sym + "\n")

def RemoveStrays (deps) :
	"Remove every node which has and is no dependency"
	strays = {}
	# does it have no dependencies?
	for k in list(deps.keys()) :
		if len(list(deps[k].deps.keys())) == 0 :
			strays[k] = 1
	# even if it does not have a dependency it still may be a dependency
	stray_keys = list(strays.keys())
	for k in list(deps.keys()) :
		node = deps[k]
		for d in list(node.deps.keys ()) :
			if d in stray_keys :
				if d in list(strays.keys ()) : # delete it only once ;)
					#print "Not stray:", d
					del strays[d]
	stray_keys = list(strays.keys())
	for k in stray_keys :
		print("Stray:", k)
		del deps[k]

def CreateDsm (deps) :
	"Given the complete dependency tree create a dsm sorted by 'leafs'"
	RemoveStrays(deps)
	nTotal = len(list(deps.keys()))
	nDone = 0
	# for DSM row and colum index are the same, this is mapping the module name to it's index (row number)
	table_map = {}
	node_keys = list(deps.keys())
	node_keys.sort()
	level = 0 # the iteration where we cut the leafs
	node_levels = {}
	while nDone < nTotal :
		leafs = []
		strays = []
		for sn in node_keys :
			node = deps[sn]
			nRefs = 0
			for d in list(node.deps.keys()) : # the dependencies
				if d in node_keys : # still referenced
					nRefs = nRefs + 1
			if nRefs == 0 :
				# not removing just now to treat all leafs on this layer the same
				leafs.append(sn)
		if len(leafs) == 0 :
			# no endless loop for circular dependencies
			histo = {} # map number of dependencies to module
			for sn in node_keys :
				node = deps[sn]
				num = len(list(node.deps.keys()))
				if not num in list(histo.keys()) :
					histo[num] = []
				histo[num].append(sn)
			# find modules with lowest number of dependencies
			for num in range(1, len(list(deps.keys()))+1) :
				if num in list(histo.keys()) :
					leafs = histo[num]
					print("Circular cut:", leafs)
					break
		for sn in leafs :
			table_map[sn] = (nTotal - nDone, level)
			nDone = nDone + 1
			node_levels[sn] = level
		level = level + 1
		# everything which has no reference into our tree or is allready processed
		node_keys = [x for x in node_keys if not x in list(table_map.keys())]
	keys = Sorted (table_map)
	# now we know the index of every module in the dsm, fill cells with number of symbols
	dsm = []
	for y, n in keys :
		# a row has everything which depends on o specific module
		target = deps[y]
		row = []
		for x, m in keys :
			# a colum has all the dependencies of a module
			source = deps[x]
			if y in list(source.deps.keys()) :
				edge = source.deps[y]
				# referencing the object, to later have access to all symbols, instead of just it's number
				row.append (edge)
			else :
				# there is no need to specially mark x==y, i.e. the diagonal
				row.append (None)
		dsm.append (row)
	# we no only need the matrix, but also the order of rows and columns
	keys_alone = []
	for k, n in keys :
		keys_alone.append (k)
	return (keys_alone, dsm, node_levels)

def SaveDsm (deps, f) :
	# create and save as csv
	# Yippie ki-yay - Excel csv interpretation is locale dependent - Control Panel/Region/Numbers/List separator
	csv_list_delimiter = ','
	k, m, levels = CreateDsm (deps)
	f.write ("Level;%s%s\n" % (csv_list_delimiter, csv_list_delimiter.join (k)))
	for y in range(0, len(k)) :
		# first column: the level of dependency
		f.write ("%d" % (levels[k[y]]) + csv_list_delimiter)
		f.write (k[y])
		for x in range(0, len(k)) :
			edge = m[y][x]
			if edge :
				f.write ("%s%d" % (csv_list_delimiter, edge.Weight()))
			else :
				f.write (csv_list_delimiter)
		f.write ("\n")

def SaveDB (deps, fname) :
	# create an sqlite database
	import sqlite3

	con = sqlite3.connect (fname)
	cur = con.cursor()

	# create a table containing data for reproduction
	cur.execute ('CREATE TABLE Settings (Key text, Value text)')
	cur.execute ('INSERT INTO Settings VALUES(?, ?)', ("cwd", os.getcwd()))
	cur.execute ('INSERT INTO Settings VALUES(?, ?)', ("args", ' '.join (sys.argv[1:])))
	con.commit()

	# create the dsm to have all data easily available
	k, m, levels = CreateDsm (deps)

	# create and fill the Modules table
	cur.execute ('CREATE TABLE Modules (Name text,' \
		' ImportedModules integer, ExportedSymbols integer, Level integer)')
	for y in range(0, len(k)) :
		# the number of imports - module count
		num_imp = len(list(deps[k[y]].deps.keys()))
		num_exp = len(list(deps[k[y]].exports.keys()))
		cur.execute ('INSERT INTO Modules VALUES(?, ?, ?, ?)', (k[y], num_imp, num_exp, levels[k[y]]))
	con.commit()
	# create an index of ModuleNames
	cur.execute ('CREATE UNIQUE INDEX ModuleNames ON Modules (Name)')
	con.commit()

	# create and fill the Dependencies table
	cur.execute ('CREATE TABLE Dependencies (' \
		'Consumer text, Provider text, Imports integer, Weight real)')
	for y in range(0, len(k)) :
		for x in range(0, len(k)) :
			edge = m[y][x]
			if edge :
				cur.execute ('INSERT INTO Dependencies VALUES(?,?,?,?)',
					(k[x], k[y], len(edge.symbols), edge.Weight()))
	con.commit ()

	# create and fill the table of Symbols
	cur.execute ('CREATE TABLE Symbols (Name text, User text, Provider text)')
	for y in range(0, len(k)) :
		for x in range(0, len(k)) :
			edge = m[y][x]
			if edge :
				for s in edge.symbols :
					cur.execute ('INSERT INTO Symbols VALUES(?,?,?)', (s, k[x], k[y]))
	# add unused symbols, too
	for sm in k :
		node = deps[sm]
		for e, used in node.exports.items() :
			if used == 0 : # the used ones are already added above
				cur.execute ('INSERT INTO Symbols VALUES(?,?,?)', (e, None, sm))
	con.commit ()

	# create a read-only table accumulating Symbols by Users
	# SELECT Provider,max(Users),count(distinct Name) FROM Symbols GROUP BY Provider
	cur.execute ('CREATE VIEW SymbolsByUse AS SELECT ' \
		'Name,Provider,count(distinct User) AS UseCount FROM Symbols GROUP BY Name')
	con.commit ()

	# another view to sort Modules by number of Consumer
	cur.execute ('CREATE VIEW ModulesByUse AS SELECT ' \
		'Provider,count(distinct Consumer),max(Imports),min(Imports) FROM Dependencies GROUP BY Provider')
	con.commit()

	# yet another view to check how much modules symbols are used
	cur.execute ('CREATE VIEW ModuleCoverageTemp AS SELECT ' \
		'Provider,count(distinct Name) AS UsedSymbols FROM Symbols AS S WHERE User>0 GROUP BY Provider')
	# merge it with Modules.ExportedSymbols
	cur.execute ('CREATE View ModuleCoverage AS SELECT ' \
		'Provider,Level,UsedSymbols,ExportedSymbols,100.0*UsedSymbols/ExportedSymbols AS Coverage ' \
		'FROM Modules AS M JOIN ModuleCoverageTemp AS C ON M.Name=C.Provider')
	con.commit()

	# calculate FanIn (UsingModules) and FanOut (ImportedModules)
	# ,1.0-(1.0/(FanIn+FanOut)) AS Complexity -- needs an extra run
	# due to sqlite3.OperationalError: no such column: FanIn
	cur.execute ('CREATE VIEW ModuleCouplingTemp AS SELECT ' \
		'Name,ImportedModules AS FanOut,count(distinct Consumer) AS FanIn ' \
		'FROM Dependencies AS D JOIN Modules AS M ON D.Provider=M.Name ' \
		'GROUP BY Provider')
	cur.execute ('CREATE VIEW ModuleCoupling AS SELECT ' \
		'Name,FanOut,FanIn,1.0-(1.0/(FanIn+FanOut)) AS Coupling ' \
		'FROM ModuleCouplingTemp')
	con.commit()

def CreateList (s) :
	"Return a list of strings either by split or from file"
	lst = []
	if s[0] == "@" :
		f = open (s[1:])
		for line in f.readlines() :
			if not line[0] in ["#", ";"] :
				lst.append (line[:-1].lower())
	else :
		lst = s.split (",")
	return lst

def ImportDump (sfDump, deps) :
	print("Import from:", sfDump)
	global g_DontFollow
	deps = {}
	f = open (sfDump)
	s = f.readline()
	rNode = re.compile (r"^([\w_-]+\.\w+)$")
	rEdge = re.compile (r"^\t([\w_-]+\.\w+) -> ([\w_-]+\.\w+)$")
	rSym = re.compile (r"^\t\t(.*)$")
	curNode = None
	curEdge = None
	symbols = []
	while s :
		s = f.readline()
		m = rNode.match (s)
		if m :
			# store previous results if
			if curNode and curEdge and len(symbols) :
				if not curNode.name in g_DontFollow :
					curNode.AddEdge (curEdge, symbols, 0) # Todo: delay loaded
				curEdge = None
				symbols = []
			k = m.group(0)
			if k in list(deps.keys()) :
				curNode = deps[k]
			else :
				curNode = deps[k] = Node (k, 0) # Todo: depth?
		else :
			m = rEdge.match(s)
			if m :
				if not curNode.name == m.group(1) :
					print("???", s)
					sys.exit(2)
				else :
					if curNode and curEdge and len(symbols) :
						if not curNode.name in g_DontFollow :
							curNode.AddEdge (curEdge, symbols, 0) # Todo: delay loaded
						#print curNode.name, "=>", curEdge
						symbols = []
					curEdge = m.group(2)
					# have to create the node, too
					if not curEdge in list(deps.keys()) :
						deps[curEdge] = Node(curEdge, curNode.depth+1)
			else :
				m = rSym.match (s)
				if m :
					symbols.append (m.group(1))
	# add the lost symbols found
	if curNode and curEdge and len(symbols) :
		curNode.AddEdge (curEdge, symbols, 0) # Todo: delay loaded
		symbols = []

	return deps

def main () :
	deps = {}
	dllsToRemove = []
	regexRemoves = []
	symbolsToRemove = []
	nMaxDepth = 10000 # almost unlimited
	components = []
	bDump = 0
	bDumpUnused = 0
	bDumpSymbolUse = 0
	bByUse = 0
	bReduce = 0
	bTred = 0
	bSaveDt = 0
	bSaveDsm = 0
	bSaveDB = 0
	bSaveXml = 0
	sOutFilename = None
	sPickle = None

	nSymbols = 0
	nCutLeafs = 0
	bRemoveNonLocal = 0
	bDumpReverse = 0

	if IsWin32() :
		# check if we are running from the right environment
		# HB: completely bogus, only works with vc2003?
		#try :
		#	print "FrameworkSDKDir=" + os.environ["FrameworkSDKDir"]
		#except :
		#	print "The script needs to be called from a 'VS command prompt'"
		#	sys.exit (1)
		if FindInPath ("dumpbin.exe") == "dumpbin.exe" :
			print("dumpbin.exe not found")
			sys.exit (1)
	for arg in sys.argv[1:] :
		if arg.find ("--remove") == 0 :
			if arg == "--remove-sys" : dllsToRemove.extend (dllsSysWin32)
			elif arg == "--remove-crt" : dllsToRemove.extend (dllsCrts)
			elif arg == "--remove-mfc" : dllsToRemove.extend (dllsMfc)
			elif arg == "--remove-gtk" : dllsToRemove.extend (dllsGtk)
			elif arg == "--remove-non-local" : bRemoveNonLocal = 1
			elif arg.find ("--remove-regex=") == 0 :
				regexRemoves.append (arg[len("--remove-regex="):])
			elif arg.find ("--remove-symbols=") == 0 :
				noSyms = CreateList(arg[len("--remove-symbols="):])
				symbolsToRemove.extend (noSyms)
			else :
				noDeps = CreateList(arg[len("--remove="):])
				dllsToRemove.extend(noDeps)
		elif arg.find ("--dont-follow") == 0 :
			global g_DontFollow
			noFollow = CreateList(arg[len("--dont-follow="):])
			g_DontFollow.extend (noFollow)
		elif arg.find ("--depth=") == 0 :
			nMaxDepth = int(arg[len("--depth="):])
			if nMaxDepth < 1 : print("Wrong depth"); sys.exit(1)
		elif arg.find ("--symbols=") == 0 :
			nSymbols = int(arg[len("--symbols="):])
			if nSymbols < 0 : nSymbols = 0
		elif arg.find ("--cut-leafs") == 0 :
			if arg.find ("--cut-leafs=") == 0 :
				nCutLeafs = int(arg[len("--cut-leafs="):])
			else :
				nCutLeafs = 10000 # infinite ;)
		elif arg == "--dump" :
			bDump = 1
		elif arg == "--dump-symbol-use" :
			bDumpSymbolUse = 1
			bDump = 1
		elif arg == "--dump-unused" :
			bDumpUnused = 1
			bDump = 1
		elif arg == "--dump-reverse" :
			bDumpReverse = 1
		elif arg == "--dt" :
			bSaveDt = 1
		elif arg == "--dsm" :
			bSaveDsm = 1
		elif arg == '--db' :
			bSaveDB = 1
		elif arg == "--xml" :
			bSaveXml = 1
		elif arg == "--reduce" :
			bReduce = 1
		elif arg == "--tred" :
			bTred = 1
		elif arg == "--by-use" :
			bByUse = 1
		elif arg.find ("--pickle=") == 0 :
			sGraph = arg[len("--pickle="):]
			sPickle = arg[len("--pickle="):] + ".pickle"
		elif arg.find ("--path") == 0 :
			global g_path
			if arg.find ("--path=") == 0 :
				g_path = arg[len("--path="):]
			else :
				g_path = os.environ['PATH']
		elif arg.find ("--theme=") == 0 :
			theme = arg[len("--theme="):]
			if theme == 'regex' :
				ParseRegexTheme('wdeps.tinting')
			else :
				print("Unknown theme!")
				sys.exit(1)
		elif arg.find ("--") == 0 :
			print("Unknown option or missing parameter:", arg)
			sys.exit(1)
		else :
			if len(components) == 0 :
				components = arg.split(",")
				components = UnGlob (components)
				# don't GetDeps here cause we need to first evaluate *all* parameters
				if len(components) == 0 :
					print("No DLL input found - wrong directory?")
					break
				sGraph = components[0]
			elif len(arg)>0 :
				sOutFilename = arg
		print("arg is:", arg)
	if len(sys.argv) < 2 :
		print(sys.argv[0], "[parameters] <component,s> [dot-output]")
		print("""
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

The most useful switch to anaylze parts of a highly coupled system is
--dont-follow=random.dll (accepting a list as above). This will show
random.dll in the resulting graph in gray. But it will not drag in
any dependencies below.

If you have a small graph it may be useful to not only show the
number of symbols used but their name. Giving --symbols=n all edges
with n or less symbols will have the (mostly demangled) name of the
symbols printed as text.

Finally there is the switch --dump for people prefering the textual
representation over some graph.

For more information read the source.
""")
		sys.exit(0)

	# output ...
	if not sOutFilename :
		f = sys.stdout
	else :
		# ... dot (or something)
		f = open(sOutFilename, "w")

	if bDump : # remember the command line
		f.write ("# " + " ".join (sys.argv) + "\n")

	try :
		import pickle
		deps = pickle.load (open(sPickle))
		print("Input from", sPickle)
	except :
		for s in components:
			if IsWin32() :
				GetDepsWin32 (s, deps, nMaxDepth)
			else :
				GetDepsPosix (s, deps, nMaxDepth)
		# with a bit dirty condition, try to populate from .dump
		if len(list(deps.keys())) == 1 and len(components) == 1:
			deps = ImportDump (components[0], deps)
			RecalculateDepth (deps)

	if len(dllsToRemove) :
		Remove (deps, dllsToRemove)
	if len(symbolsToRemove) > 0 :
		RemoveBySymbols (deps, symbolsToRemove)

	for rr in regexRemoves :
		RemoveRegEx (deps, rr)

	if bRemoveNonLocal :
		RemoveNonLocal (deps)

	while nCutLeafs > 0 :
		# not always iterating here, CutLeafs does too
		if bDump :
			nTotal = len(list(deps.keys()))
			leafs = CutLeafs (deps, 1)
			if len(leafs) < 1 :
				break
			f.write ("CutLeafs " + str(nTotal) + " => " + str(nTotal - len(leafs)) + "\n")
			leafs.sort()
			f.write ("\t" + ",".join (leafs) + "\n")
			nCutLeafs -= 1
		else :
			leafs = CutLeafs (deps, nCutLeafs)
			break

	if bDump :
		topmost = TopMost(deps)
		f.write ("TopMost(%d) : %s\n" % (len(topmost), ",".join(topmost)))

	# the *automatic* reduction is intentionally after cut-leafs
	if bReduce :
		if not sOutFilename :
			f2 = f
		else :
			f2 = open (sOutFilename + ".reduced.txt", "w")
		Reduce (deps, f2)
		if nCutLeafs > 1000 : # magic number two, still infinite ;)
			for d in list(deps.keys()) :
				# kind of arbitrary numbers
				n1 = len(list(deps[d].deps.keys())) # lthis ones dependencies
				n2 = len(list(deps.keys())) # total number of components left
				# write erros/warning like the compiler would do
				f2.write ("%s - %d error(s), %d warning(s)\n" % (deps[d].name, n1, n2))
	if bTred :
		tops = TopMost (deps)
		if len(tops) == 0 :
			print("Transitive reduction without topmost!")
			tops = list(deps.keys ())
		TransitiveReduction (deps, tops)

	if sPickle :
		import pickle
		pickle.dump(deps, open(sPickle,"w"))

	if bDumpSymbolUse :
		DumpSymbolsUse (deps, f)
		sys.exit (0)
	elif bDumpUnused :
		DumpUnusedSymbols (deps, f)
		sys.exit (0)
	elif bDump :
		DumpSymbols (deps, f)
		# no diagram at all
		sys.exit (0)
	if bSaveDt :
		SaveDt (deps, f)
	elif bSaveXml :
		SaveXml (deps, f)
	elif bSaveDsm :
		SaveDsm (deps, f)
	elif bSaveDB :
		SaveDB (deps, sOutFilename)
	elif bDumpReverse :
		DumpReverse (deps, f)
	else :
		SaveDot (deps, sGraph, bByUse, nSymbols, f)

def SaveXml (deps, f) :
	"simple XML dump"
	f.write('<nodes count="%d">\n' % (len(list(deps.keys()))))
	f.write('\t'*1 + '<command>wdeps.py ' + ' '.join (sys.argv[1:]) + '</command>\n')
	deps_keys = list(deps.keys())
	deps_keys.sort()
	for sn in deps_keys :
		node = deps[sn]
		f.write('\t'*1 + '<node name="%s" symbols="%d">\n' % (sn, len(node.exports)))
		edge_keys = list(node.deps.keys())
		if edge_keys :
			edge_keys.sort()
			f.write('\t'*2 + '<edges count="%d">\n' % (len(edge_keys)))
			for en in edge_keys :
				edge = node.deps[en]
				f.write('\t'*3 + '<edge name="%s" weight="%g">\n' % (en, edge.Weight()))
				f.write('\t'*4 + '<symbols>\n')
				symbols = edge.symbols
				symbols.sort()
				for sym in symbols :
					f.write('\t'*5 + '<symbol name="%s"/>\n' % (sym))
				f.write('\t'*4 + '</symbols>\n')
				f.write('\t'*3 + '</edge>\n')
			f.write('\t'*2 + '</edges>')
		f.write('\t'*1 + '</node>\n')
	f.write('</nodes>\n')

def SaveDt (deps, f) :
	""" see: http://dtangler.org """
	deps_keys = list(deps.keys())
	deps_keys.sort()
	for sn in deps_keys :
		node = deps[sn]
		edge_keys = list(node.deps.keys())
		if not edge_keys :
			continue
		edge_keys.sort()
		f.write (sn + " : " + " ".join (edge_keys) + "\n")

def SaveDot (deps, sGraph, bByUse, nSymbols, f) :
	# build the graph
	f.write ('digraph "' + sGraph + '" {\n')
	f.write ('graph [fontsize=8.0 label="wdeps.py ' + " ".join (sys.argv[1:])
			+ '\\n' + time.ctime() + '"]\n')
	f.write ('ratio=0.7\nnode [fontsize=12.0 ]\nedge [fontsize=8.0]\n')
	if bByUse :
		# kind of inverted diagram not showing edependencies but 'users'
		users = {}
		for sn in list(deps.keys()) :
			users[sn] = []
		for sn in list(deps.keys()) :
			node = deps[sn]
			for se in list(node.deps.keys()) :
				edge = node.deps[se]
				users[edge.name].append(node.name)
		for sn in list(users.keys()) :
			for s in users[sn] :
				f.write ('"%s" -> "%s"\n' % (sn, s))
	else :
		# first pass mark don't follows
		dontFollowsDone = {}
		urlsDone = {}
		deps_keys = list(deps.keys())
		deps_keys.sort()
		for sn in deps_keys :
			node = deps[sn]
			edge_keys = list(node.deps.keys())
			edge_keys.sort()
			for se in edge_keys :
				cut_len = len(se)
				if IsWin32() :
					cut_len = se.find(".dll")
				else :
					cut_len = se.find(".so")
				ses = se[:cut_len]
				if se in g_DontFollow :
					if se not in dontFollowsDone :
						# mark as such
						f.write ('"%s" [style=filled,fillcolor="lightgray"]\n' % (se,))
						dontFollowsDone[se] = 1
				else :
					if se not in urlsDone :
						f.write ('"%s" [style=filled,fillcolor="%s",URL="#%s"]\n' % (se, LookupColor(deps[se]), se))
						urlsDone[se] = 1


		for sn in deps_keys :
			# write weighted edges, could also classify the nodes ...
			node = deps[sn]
			edge_keys = list(node.deps.keys())
			edge_keys.sort()
			for se in edge_keys :
				edge = node.deps[se]
				sPrefix = ""
				if edge.reduce : # remove from graph by commenting it out
					sPrefix = "// " + ", ".join(edge.symbols) + "\n// "
				if edge.delayLoad :
					# putting 'weight' and 'constraint' seems to be too much for dot
					sStyle = "weight=%f,style=dotted" % (math.log10(math.sqrt(edge.Weight()+.1)),)
					# even using constraint at all seems to often crash dot
					#sStyle = "style=dotted,constraint=false"
				else :
					sStyle = "weight=%f" % (math.log10(edge.Weight()+.1),)
				if edge.Weight() <= nSymbols :
					#f.write ('"%s" -> "%s" [weight=%f,label=%s]\n' % (node.name, edge.name, math.log(1)-0.5, edge.symbols[0]))
					f.write ('%s"%s" -> "%s" [fontsize=6,label="%s",%s]\n'
							% (sPrefix, node.name, edge.name, "\\n".join(edge.symbols), sStyle))
				else :
					#f.write ('"%s" -> "%s" [weight=%f]\n' % (node.name, edge.name, math.log(edge.Weight())-0.5))
					f.write ('%s"%s" -> "%s" [label="(%d)",%s]\n'
							% (sPrefix, node.name, edge.name, edge.Weight(), sStyle))
	f.write("}\n")

if __name__ == '__main__': main()
