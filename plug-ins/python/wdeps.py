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
			if self.exports.has_key(s) :
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
	for sDir in string.split (myPath, os.pathsep) :
		p = sDir + os.sep + sName
		if os.path.exists (p) :
			return p
	# safety
	return sName

def GetDepsWin32 (sFrom, dAll, nMaxDepth, nDepth=0) :
	"calculates the dependents of the passed in dll"
	if nMaxDepth <= nDepth :
		return
	sFrom = string.lower(sFrom)
	global g_DontFollow
	if sFrom in g_DontFollow :
		dAll[sFrom] = Node (sFrom, nDepth) # needed here so other algoritm can remove it ;)
		return
	if not dAll.has_key (sFrom) :
		node = Node (sFrom, nDepth)
		sPath = sFrom
		if g_path :
			sPath = FindInPath (sFrom)
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
			r = re.match ("^    (.*\.dll)", s, re.IGNORECASE)
			# the delay load switch is flipped once
			if s.find ("delay load imports") > 0 :
				delayLoad = 1
			if r :
				name = string.lower(r.group(1))
				# print name
			else :
				# import by name
				# The system dlls seem to follow a diferent pattern (or is this for know-dlls? Delay load?)  thus (?:[0123456789ABCDEF]{8}[ ]+)?
				r2 = re.match ("^[ ]+(?:[\dA-F]{8}[ ]+)?[\dA-F]{1,5}[ ]+([\w@?$]+)$", s)
				if not r2 :
					r2 = re.match ("^[ ]+(?:[\dA-F]{8}[ ]+)?Ordinal[ ]+([\d]+)$", s)
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
		# restore original depth (independent of how the recurison works)
		for sd in directDeps :
			if sd in dAll.keys() :
				try :
					dAll[sd].depth = nDepth + 1 
				except AttributeError, msg :
					print "FIXME:", sd, msg
	
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
	if not sFromName in dAll.keys() :
		#print "Creating", sFromName, nDepth
		node = Node (sFromName, nDepth)
		sPath = sFrom
		#TODO: work with relative pathes? Current dir?
		f = os.popen ('ldd "' + sPath + '"')
		sModules = f.readlines ()
		# avoids multiple instances of dumpbin running simultaneously
		f = None
		# avoids infinite recursion on circular dependencies
		dAll[sFromName] = None
		# sFrom imports
		fImports = os.popen ('nm --extern-only --dynamic --undefined-only ' + sPath)
		sImports = fImports.readlines()
		dImports = {}
		for si in sImports :
			mi = re.match("\s+U (\S+)", si)
			if mi :
				dImports[mi.group(1)] = 1
		toRecurse = []
		for s in sModules :
			r = re.match ("\s+(?P<name>\S+) => (?P<path>\S+).*", s)
			if r :
				# print r.group("name"), r.group("path")
				# now find symbols between sFrom and any of the SOs
				usedSymbols = []
				unusedSymbols = []
				fExports = os.popen ('nm --extern-only --dynamic --defined-only ' + r.group("path"))
				sExports = fExports.readlines() 
				for se in sExports :
					me = re.match ("\w+ T (\S+)", se)
					if me :
						symbol = me.group(1)
						# print me.group(1), r.group('name') 
						if symbol in dImports.keys() :
							# direct connection
							usedSymbols.append (symbol)
						else :
							unusedSymbols.append(symbol)
				# now we have complete symbols for node r.group('name'), 
				# should remember the node although it may not be connected yet
				# OTOH the recursion would become more difficult to understand, favor KISS
				if len(usedSymbols) > 0 :
					node.AddEdge(r.group("name"), usedSymbols, 0)
					toRecurse.append(r.group("path"))
					print sFromName, "(", nDepth, "):", r.group("path")
		# add to all nodes
		dAll[sFromName] = node
		for sd in toRecurse :
			print sFromName, "->", sd, "level:", nDepth+1
			sdn = os.path.split(sd)[-1]
			GetDepsPosix (sd, dAll, nMaxDepth-nDepth+2, nDepth+1)
			if sdn in dAll.keys() :
				#print "Adjusting depth", sd, nDepth+1
				dAll[sdn].depth = nDepth+1 # adjust to original depth (not how our recursion found it)

def IsWin32 () :
	return os.name in ["nt"]

def Remove (deps, list) :
	"From the deps tree remove the nodes matching list"
	for s in list :
		if deps.has_key (s) :
			del deps[s]
		for sn in deps.keys() :
			node = deps[sn]
			if node.deps.has_key (s) :
				del node.deps[s]
def RemoveRegEx (deps, reg_ex) :
	"If a module matches reg_ex remove it"
	import re
	rr = re.compile (reg_ex)
	for k1 in deps.keys() :
		if rr.match (k1) :
			del deps[k1]
		else :
			node = deps[k1]
			for k2 in node.deps.keys () :
				if rr.match (k2) :
					del node.deps[k2]
def RemoveBySymbols (deps, list) :
	"If a connection is conly caused by some symbol in 'list' it is removed"
	for k in deps.keys() :
		node = deps[k]
		for c in node.deps.keys () :
			edge = node.deps[c]
			kills = []
			for s2 in edge.symbols :
				for s1 in list :
					# robustness, dont compare the empty string
					if s1 == "" : continue
					# only comparing the start of the symbol
					if string.find (s2, s1) == 0 :
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
def Reduce (deps, f, bHintOnly = 1) :
	"Automatically remove connections until there is only something reasonable left"
	# first iteration: two components are connected in both directions
	# if one connection is much weaker than the other one, the weaker one is considered bad
	# if both have similar weight they are both treated as bad and removed
	keys = deps.keys()
	keys.sort()
	nReduced = 0
	for k in keys :
		node = deps[k]
		for c in node.deps.keys () :
			edge = node.deps[c]
			if deps.has_key (c) :
				node2 = deps[c]
				if node2.deps.has_key(k) :
					edge2 = node2.deps[k]
					# if one already is reduced dont do it again
					if edge.reduce or edge2.reduce :
						continue
					n = len(edge.symbols)
					n2 = len(edge2.symbols)
					if  abs(n - n2) < 3 :
						f.write ("Remove (" + str(n) + "," + str(n2) +  ") " + edge.name + " <-> " + edge2.name +
							"\n\t" + string.join (edge.symbols, ", ") +
							"\n\t" + string.join (edge2.symbols, ", ") + "\n")
						# if we now remove *both* connections some components could become completely disconnected. 
						# Instead may try some more guessing. Maybe the component with less deps should drop one more ?
						if bHintOnly :
							edge.reduce = 1
							edge2.reduce = 1
						else :
							if node.deps.has_key(c) : del node.deps[c]
							if node2.deps.has_key(k) : del node2.deps[k]
						nReduced += (n + n2)
					elif n > n2 :
						f.write ("Remove (" + str(n2) + ") " + edge.name + " -> " + edge2.name +
							"\n\t" + string.join (edge2.symbols, ", ") + "\n")
						if bHintOnly :
							edge2.reduce = 1
						else :
							if node2.deps.has_key(k) : del node2.deps[k]
						nReduced += n2
					else : 
						f.write("Remove (" + str(n) + ") " + edge2.name + " -> " + edge.name + 
							"\n\t" + string.join (edge.symbols, ", ") + "\n")
						if bHintOnly :
							edge.reduce = 1
						else :
							if node.deps.has_key(c) : del node.deps[c]
						nReduced += n
	f.write("Remove total: %d\n" % (nReduced))

def CutLeafs (deps, nCuts) :
	"Remove components without connections. After some iterations only circulars remain (or nothing)"
	leafs = {}
	cut = [] # what we have cut in the last pass
	while nCuts > 0 :
		cut = leafs.keys()
		leafs = {}
		# first pass: find leafs
		for k in deps.keys() :
			node = deps[k]
			if len(node.deps.keys ()) == 0 :
				leafs[k] = 0
		if len(leafs.keys()) == 0 :
			break # nothing remaining
		nCuts -= 1
		# second pass : remove references to leafs
		for k in deps.keys() :
			node = deps[k]
			for c in leafs.keys() :
				if node.deps.has_key(c) :
					leafs[c] += 1
					del node.deps[c]
		# third pass: remove the leafs
		for k in leafs.keys() :
			# we either need to reset leafs in every round or 
			if deps.has_key(k) :
				del deps[k]
	if len(leafs.keys()) > 0 :
		return leafs.keys()
	return cut
def TransitiveReduction (deps, tops, treds=[]) :
	"""If a component A has B and C as dependency and B also depends on C reduce A->C.
	   This algorithm is available with dot, too. But here we can adjust the respective
	   'inherited' symbols and 'depth' as well.
	"""
	for a in tops :
		bs = deps[a].deps.keys()
		cs = {}
		for b in bs :
			for c in deps[b].deps.keys() :
				if not c in cs.keys() :
					cs[c] = 1
		# bs and cs are now complete, remove all a->b, where b in cs
		extra_syms = {}
		for c in cs.keys() :
			if c in bs :
				print "Cut", a, "->", c
				# we need to know which edge a->b is taking the extra symbols
				extra_syms[c] = deps[a].deps[c].symbols
				del deps[a].deps[c]
		# fill remaining bs having a c dependency with the extra symbols
		e_keys = extra_syms.keys()
		for e in e_keys :
			bs = deps[a].deps.keys() # they are reduced above
			for b in bs :
				if e in deps[b].deps.keys () :
					print "Adjust", a, "->", b, "+", len(extra_syms[e]), "from", e
					#NOT: deps[a].deps[b].symbols.extend(extra_syms[e]) # producing duplicates
					for s in extra_syms[e] :
						if not s in deps[a].deps[b].symbols :
							deps[a].deps[b].symbols.append(s)
					del extra_syms[e]
					break
		#FIXME: something we have cut before may not be visible anymore (would need a deeper search or an ordered reduction)
		if len(extra_syms.keys()) :
			print "Not adjusted:", a, "->", string.join(extra_syms.keys(), ", ")
	# recurse into remaining bs
	for a in tops :
		bs = deps[a].deps.keys()
		treds.append(a) # rember already done to avoid endless recursion
		TransitiveReduction (deps, bs, treds)

def TopMost (deps) :
	"everything not used by something else"
	keys = deps.keys ()
	topmost = []
	used = {}
	for k in keys :
		for d in deps[k].deps.keys() :
			if not used.has_key (d) :
				used[d] = 1
	for k in keys :
		if not used.has_key (k) :
			topmost.append (k)
	return topmost
def UnGlob (comps) :
	import glob
	comp_dict = {}
	for p in comps :
		g = glob.glob (p)
		for d in g :
			if not comp_dict.has_key (d) :
				comp_dict[d] = 1
	return comp_dict.keys()

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
	"ws2_32.dll", "ws2help.dll", "wsock32.dll", "ntdll.dll", "mpr.dll", 
	"rpcrt4.dll", "shlwapi.dll", "netapi32.dll", "msimg32.dll", "oledlg.dll",
	"setupapi.dll", "secur32.dll", "avifil32.dll", "msvfw32.dll", 
	"uxtheme.dll"]
dllsCrts = [
	"msvcrt.dll", "msvcrtd.dll", "msvcp60.dll",
#	"msvcrt20.dll",
#	"msvcr70.dll", "msvcp70.dll",
	"msvcr71.dll", "msvcr71d.dll", "msvcp71.dll", "msvcp71d.dll",
	"msvcr80.dll", "msvcr80d.dll", "msvcp80.dll", "msvcp80d.dll",
	# only one on Linux? (or am I missing th C++rt, and what about librt.so.1?)
	"libc.so.5", "libc.so.6"
	]
dllsMfc = [
	"mfc71u.dll", "mfc71.dll", "mfc71ud.dll",
	"mfc80u.dll", "mfc80.dll", "mfc80ud.dll"
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
			except re.error, msg :
				print "Parser error\n", s
				sys.exit(2)
	if len(colorMatcher) == 0 :
		print "no colors parsed:", fname
		sys.exit(3)
	else :
		print len(colorMatcher), "colors parsed:", fname

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
	except KeyError, msg :
		print msg
		return "#ef2929" # scarlet red
	except IndexError, msg :
		print "Dependency", node.name, 'level', node.depth
		return "#cc0000" # scarlet ret (a bit darker)

def DumpSymbols (deps, f) :
	Symbols = {}
	Modules = {}
	Imports = {}
	node_keys = deps.keys()
	node_keys.sort()
	for sn in node_keys :
		node = deps[sn]
		if len(node.deps.keys()) == 0 :
			continue
		f.write (sn + "\n")
		Imports[sn] = 0
		edge_keys = node.deps.keys()
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
	f.write ( "***** Modules with users (symbols) *****\n" )
	for s, i in Sorted (Imports) :
		if Modules.has_key (s) :
			f.write (s + " (" + str(i) + ") : " + string.join (Modules[s], ",") + "\n")
		else :
			f.write (s + " (0) : <no users>\n")

def main () :
	deps = {}
	dllsToRemove = []
	regexRemoves = []
	symbolsToRemove = []
	nMaxDepth = 10000 # almost unlimited
	components = []
	bDump = 0
	bByUse = 0
	bReduce = 0
	bTred = 0
	sOutFilename = None
	sPickle = None

	nSymbols = 0
	nCutLeafs = 0

	if IsWin32() :
		# check if we are running from the right environment
		# HB: completely bogus, only works with vc2003? 
		#try :
		#	print "FrameworkSDKDir=" + os.environ["FrameworkSDKDir"]
		#except :
		#	print "The script needs to be called from a 'VS command prompt'"
		#	sys.exit (1)
		if FindInPath ("dumpbin.exe") == "dumpbin.exe" :
			print "dumpbin.exe not found"
			sys.exit (1)
	for arg in sys.argv[1:] :
		if string.find (arg, "--remove") == 0 :
			if arg == "--remove-sys" : dllsToRemove.extend (dllsSysWin32)
			elif arg == "--remove-crt" : dllsToRemove.extend (dllsCrts)
			elif arg == "--remove-mfc" : dllsToRemove.extend (dllsMfc)
			elif arg == "--remove-gtk" : dllsToRemove.extend (dllsGtk)
			elif string.find (arg, "--remove-regex=") == 0 :
				regexRemoves.append (arg[len("--remove-regex="):])
			elif string.find (arg, "--remove-symbols=") == 0 :
				noSyms = string.split(arg[len("--remove-symbols="):], ",")
				symbolsToRemove.extend (noSyms)
			else :
				noDeps = string.split(arg[len("--remove="):], ",")
				dllsToRemove.extend(noDeps)
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
		elif string.find (arg, "--cut-leafs") == 0 :
			if string.find (arg, "--cut-leafs=") == 0 :
				nCutLeafs = int(arg[len("--cut-leafs="):])
			else :
				nCutLeafs = 10000 # infinite ;)
		elif arg == "--dump" :
			bDump = 1
		elif arg == "--reduce" :
			bReduce = 1
		elif arg == "--tred" :
			bTred = 1
		elif arg == "--by-use" :
			bByUse = 1
		elif string.find (arg, "--pickle=") == 0 :
			sGraph = arg[len("--pickle="):]
			sPickle = arg[len("--pickle="):] + ".pickle"
		elif string.find (arg, "--path") == 0 :
			global g_path
			if string.find (arg, "--path=") == 0 :
				g_path = arg[len("--path="):]
			else :
				g_path = os.environ['PATH']
		elif string.find (arg, "--theme=") == 0 :
			theme = arg[len("--theme="):]
			if theme == 'regex' :
				ParseRegexTheme('wdeps.tinting')
			else :
				print "Unknown theme!"
				sys.exit(1)
		elif string.find (arg, "--") == 0 :
			print "Unknown option or missing parameter:", arg
		else :
			if len(components) == 0 :
				components = string.split(arg, ",")
				components = UnGlob (components)
				# don't GetDeps here cause we need to first evaluate *all* parameters
				if len(components) == 0 :
					print "No DLL input found - wrong directory?"
					sys.exit(1)
				sGraph = components[0]
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
"""
		sys.exit(0)

	# output ...
	if not sOutFilename :
		f = sys.stdout
	else :
		# ... dot
		f = open(sOutFilename, "w")

	if bDump : # remember the command line
		f.write ("# " + string.join (sys.argv, " ") + "\n")

	try :
		import pickle
		deps = pickle.load (open(sPickle))
		print "Input from", sPickle
	except :
		for s in components:
			if IsWin32() :
				GetDepsWin32 (s, deps, nMaxDepth)
			else :
				GetDepsPosix (s, deps, nMaxDepth)

	if len(dllsToRemove) :
		Remove (deps, dllsToRemove)
	if len(symbolsToRemove) > 0 :
		RemoveBySymbols (deps, symbolsToRemove)

	for rr in regexRemoves :
		RemoveRegEx (deps, rr)

	while nCutLeafs > 0 :
		# not always iterating here, CutLeafs does too
		if bDump :
			nTotal = len(deps.keys())
			leafs = CutLeafs (deps, 1)
			if len(leafs) < 1 :
				break
			f.write ("CutLeafs " + str(nTotal) + " => " + str(nTotal - len(leafs)) + "\n")
			leafs.sort()
			f.write ("\t" + string.join (leafs, ",") + "\n")
			nCutLeafs -= 1
		else :
			leafs = CutLeafs (deps, nCutLeafs)
			break

	if bDump :
		topmost = TopMost(deps)
		f.write ("TopMost(%d) : %s\n" % (len(topmost), string.join(topmost, ",")))

	# the *automatic* reduction is intentionally after cut-leafs
	if bReduce :
		if not sOutFilename :
			f2 = f
		else :
			f2 = open (sOutFilename + ".reduced.txt", "w")
		Reduce (deps, f2)
		if nCutLeafs > 1000 : # magic number two, still infinite ;)
			for d in deps.keys() :
				# kind of arbitrary numbers
				n1 = len(deps[d].deps.keys()) # lthis ones dependencies
				n2 = len(deps.keys()) # total number of components left
				# write erros/warning like the compiler would do
				f2.write ("%s - %d error(s), %d warning(s)\n" % (deps[d].name, n1, n2))
	if bTred :
		tops = TopMost (deps)
		if len(tops) == 0 :
			print "Transitive reduction without topmost!"
			tops = deps.keys ()
		TransitiveReduction (deps, tops)

	if sPickle :
		import pickle
		pickle.dump(deps, open(sPickle,"w"))

	if bDump :
		DumpSymbols (deps, f)
		# no diagram at all
		sys.exit (0)

	f.write ('digraph "' + sGraph + '" {\n')
	f.write ('graph [fontsize=8.0 label="wdeps.py ' + string.join (sys.argv[1:], " ") 
			+ '\\n' + time.ctime() + '"]\n') 
	f.write ('ratio=0.7\nnode [fontsize=12.0 ]\nedge [fontsize=8.0]\n')
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
		urlsDone = {}
		deps_keys = deps.keys()
		deps_keys.sort()
		for sn in deps_keys :
			node = deps[sn]
			edge_keys = node.deps.keys()
			edge_keys.sort()
			for se in edge_keys :
				cut_len = len(se)
				if IsWin32() :
					cut_len = string.find(se, ".dll")
				else :
					cut_len = string.find(se, ".so") 
				ses = se[:cut_len]
				if se in g_DontFollow :
					if not dontFollowsDone.has_key(se) :
						# mark as such
						f.write ('"%s" [style=filled,fillcolor="lightgray"]\n' % (se,))
						dontFollowsDone[se] = 1
				else :
					if not urlsDone.has_key (se) :
						f.write ('"%s" [style=filled,fillcolor="%s",URL="#%s"]\n' % (se, LookupColor(deps[se]), se))
						urlsDone[se] = 1
						
					
		for sn in deps_keys :
			# write weighted edges, could also classify the nodes ...
			node = deps[sn]
			edge_keys = node.deps.keys()
			edge_keys.sort()
			for se in edge_keys :
				edge = node.deps[se]
				sPrefix = ""
				if edge.reduce : # remove from graph by commenting it out
					sPrefix = "// " + string.join(edge.symbols, ", ") + "\n// "
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
							% (sPrefix, node.name, edge.name, string.join(edge.symbols, "\\n"), sStyle))
				else :
					#f.write ('"%s" -> "%s" [weight=%f]\n' % (node.name, edge.name, math.log(edge.Weight())-0.5))
					f.write ('%s"%s" -> "%s" [label="(%d)",%s]\n' 
							% (sPrefix, node.name, edge.name, edge.Weight(), sStyle))
	f.write("}\n")

if __name__ == '__main__': main()

