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

import sys, os, re, string, math

g_maxWeight = 1

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
		 		 self.symbols = symbols
		 		 
def GetDeps (sFrom, dAll) :
		 "calculates the dependents of the passed in dll"
		 sFrom = string.lower(sFrom)
		 if not dAll.has_key (sFrom) :
		 		 node = Node (sFrom)
		 		 f = os.popen ("dumpbin /imports " + sFrom)
		 		 name = None
		 		 arr = []
		 		 s = f.readline ()
		 		 while s :
		 		 		 r = re.match ("^    (.*\.dll)", s)
		 		 		 if r :
		 		 		 		 name = string.lower(r.group(1))
		 		 		 		 #print name
		 		 		 else :
		 		 		 		 # import by name
		 		 		 		 r2 = re.match ("^[ ]+[0123456789ABCDEF]+[ ]+(\w+)$", s)
		 		 		 		 if not r2 :
		 		 		 		 		 r2 = re.match ("^[ ]+Ordinal[ ]+([1234567890]+)$", s)
		 		 		 		 if r2 :
		 		 		 		 		 arr.append (r2.group(1))
		 		 		 		 elif s[:-1] == "" and name != None and len(arr) > 0 :
		 		 		 		 		 #print name, len(arr)
		 		 		 		 		 node.AddEdge (name, arr)
		 		 		 		 		 arr = []
		 		 		 		 		 GetDeps (name, dAll)
		 		 		 s = f.readline()
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
		 		 		 		 
# some predefined sets of DLLs, either for hiding from the dependencies or maybe to tin them
dllsSysWin32 = [
		 "version.dll", "winmm.dll", 
		 "kernel32.dll", "user32.dll", "gdi32.dll", "comdlg32.dll", "advapi32.dll", "shell32.dll",
		 "comctl32.dll", "ole32.dll", "oleaut32.dll", "winspool.drv", "imm32.dll",
		 "wsock32.dll", "mpr.dll",
		 "uxtheme.dll"]
dllsCrts = [
		 "msvcrt.dll", "msvcrtd.dll", "msvcp60.dll",
		 "msvcr71.dll", "msvcr71d.dll", "msvcp71.dll"
		 ]
dllsGtk = [
		 "libglib-2.0-0.dll", "libgmodule-2.0-0.dll", "libgobject-2.0-0.dll", "libgthread-2.0-0.dll",
		 "libpango-1.0-0.dll", "libpangowin32-1.0-0.dll", "libpangoft2-1.0-0.dll", "libpangocairo-1.0-0.dll",
		 "libgdk_pixbuf-2.0-0.dll", "libgdk-win32-2.0-0.dll", "libgtk-win32-2.0-0.dll", "libatk-1.0-0.dll", 
		 "libcairo.dll", "libintl-1.dll", "iconv.dll"
		 ]
def main () :
		 deps = {}
		 GetDeps (sys.argv[1], deps)

		 #print deps
		 Remove (deps, dllsSysWin32)
		 Remove (deps, dllsCrts)
		 #Remove (deps, dllsGtk)
		 # output ...
		 if len(sys.argv) > 2 :
		 		 # ... dot
		 		 f = open(sys.argv[2], "w")
		 		 f.write ('digraph "' + sys.argv[1] + '" {\nratio=0.7\n')
		 		 for sn in deps.keys() :
		 		 		 # write weighted edges, could also classify the nodes ...
		 		 		 node = deps[sn]
		 		 		 for se in node.deps.keys() :
		 		 		 		 edge = node.deps[se]
		 		 		 		 if edge.weight == 1 :
		 		 		 		 		 #f.write ('"%s" -> "%s" [weight=%f,label=%s]\n' % (node.name, edge.name, math.log(1)-0.5, edge.symbols[0]))
		 		 		 		 		 f.write ('"%s" -> "%s" [fontsize=8,label=%s]\n' % (node.name, edge.name, edge.symbols[0]))
		 		 		 		 else :
		 		 		 		 		 #f.write ('"%s" -> "%s" [weight=%f]\n' % (node.name, edge.name, math.log(edge.weight)-0.5))
		 		 		 		 		 f.write ('"%s" -> "%s" [label="(%d)",weight=%d]\n' % (node.name, edge.name, edge.weight, edge.weight))
		 		 f.write("}\n")

if __name__ == '__main__': main()
