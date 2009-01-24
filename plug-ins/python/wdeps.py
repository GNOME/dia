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

import sys, os, re, string

class Node :
	def __init__ (self, name) :
		self.name = name
		self.deps = {}
	def AddEdge (self, name, symbols) :
		self.deps[name] = Edge(name, symbols)

class Edge :
	def __init__ (self, name, symbols) :
		self.name = name # the target
		self.weight = len(symbols)
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
					print name, len(arr)
					node.AddEdge (name, arr)
					arr = []
					GetDeps (name, dAll)
			s = f.readline()
		# add to all nodes
		dAll[sFrom] = node
	
def main () :
	deps = {}
	GetDeps (sys.argv[1], deps)

	if len(sys.argv) > 2 :
		# ... dot
		f = open(sys.argv[2], "w")
		f.write ('digraph "' + sys.argv[1] + '" {\n')
		for sn in deps.keys() :
			print sn
			# write weighted edges, could also classify the nodes ...
			node = deps[sn]
			for se in node.deps.keys() :
				print "\t" + se
				edge = node.deps[se]
				if edge.weight > 1 :
					f.write ('"%s" -> "%s [weight=%d]"\n' % (node.name, edge.name, edge.weight))
		f.write("}\n")

if __name__ == '__main__': main()
