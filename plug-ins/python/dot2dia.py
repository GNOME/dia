# PyDia DOT Import
# Copyright (c) 2009 Hans Breuer <hans@breuer.org>

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

##
# \file dot2dia.py \brief translate dot ( http://www.graphviz.org/ ) to Dia format
# \ingroup ImportFilters

import re, sys

import gettext
_ = gettext.gettext

# FIXME: keywords are case indepentend
keywords = ['node', 'edge', 'graph', 'digraph', 'strict']
# starts with either a keyword or a node name in quotes.
# BEWARE: (?<-> ) - negative lookbehind to not find nodes a second time in connection definition (and misinterpret the params)
rDecl = re.compile(r'\s*(?<!-> )(?P<cmd>(?:' + ')|(?:'.join(keywords) + ')|(?:\w+' + ')|(?:"[^"]+"))\s+\[(?P<dict>[^\]]+)\];', re.DOTALL | re.MULTILINE)
# dont assume that all node names are in quotes
rEdge = re.compile(r'\s*(?P<n1>("[^"]+")|(\w+))\s*->\s*(?P<n2>("[^"]+")|(\w+))\s+\[(?P<dict>[^\]]+)\];*', re.DOTALL | re.MULTILINE)
# a list of key=value
rParam = re.compile(r'(?P<key>\w+)\s*=(?P<val>(\w+)|("[^"]+")),?\s*', re.DOTALL | re.MULTILINE)

# units in dot are either points or inch
cmInch = 2.54
cmPoints = cmInch/72.0
# dot y up, dia y down

def StripQuotes(s) :
	"strip quotes if any"
	if s[0] == '"' :
		s = s[1:-1]
	return s

def DictFromString (s) :
	#print "KVs", s
	d = {}
	for m in rParam.finditer (s) :
		if m :
			d[m.group ("key")] = StripQuotes(m.group ("val"))
	return d

##
# \brief Accumulating information with _DiaObject
class Object :
	""" will end as a Dia Object """
	def __init__ (self, typename, parms) :
		self.typename = typename
		self.parms = parms
	def FontSize (self) :
		try :
			return float(self.parms['fontsize']) * cmPoints
		except :
			return 0.6
##
# \brief The nodes of the graph - finally represented as _Ellipse
class Node(Object) :
	def __init__ (self, name, parms) :
		Object.__init__(self, "Standard - Ellipse", parms)
		self.name = name
	def Pos(self) :
		'deliver scaled X,Y coordinate'
		x, y = 0.0, 0.0
		try :
			xy = self.parms['pos'].split(',')
			x = float(xy[0]) * cmPoints
			y = float(xy[1]) * cmPoints
		except :
			print("No position on '%s'" % (self.name,))
		return x,-y
	def Size(self) :
		'deliver scaled W,H coordinate'
		w, h = 0.5, 0.5
		try :
			w = float(self.parms['width']) * cmInch #? maybe this is relative to the font size?
			h = float(self.parms['height']) * cmInch
		except :
			print("No size on '%s'" % (self.name,))
		return w,h

##
# \brief The edges of the graph - finally represented as _Bezierline
class Edge(Object) :
	def __init__ (self, src, dest, parms) :
		Object.__init__(self, "Standard - BezierLine", parms)
		self.src = src
		self.dest = dest
	def LabelPos (self) :
		x, y = 0.0, 0.0
		try :
			xy = self.parms['lp'].split(',')
			x = float(xy[0]) * cmPoints
			y = float(xy[1]) * cmPoints
		except :
			if 'label' in self.parms :
				# should be optional otherwise
				print("No label pos on %s" % (self.src + '->' + self.dest,))
		return x, -y
	def Pos (self) :
		# no need to do something smart, it get adjusted anyway
		return 0.0, 0.0
	def SetPoints (self, diaobj) :
		'the property to set must match the type'
		pts = []
		if 'pos' in self.parms :
			s = self.parms['pos']
			if s[:2] == 'e,' :
				sp = s[2:].split(" ")
				# apparently the first point is the end? just put it there!
				sp.append(sp[-1])
				del sp[0]
				bp = []
				for i in range(0, len(sp)) :
					xy = sp[i].replace("\n", "").replace("\\", "").split(",")
					try :
						x = float(xy[0]) * cmPoints
						y = float(xy[1]) * (-cmPoints)
					except ValueError :
						print(xy)
						continue
					bp.append((x,y))
					# must convert to _one_ tuple
					if i == 0 : # first must be move to
						pts.append ((0, bp[0][0], bp[0][1]))
						bp = [] # reset to new point
					elif len(bp) == 3 : # rest is curveto ==2
						pts.append ((2, bp[0][0], bp[0][1], bp[1][0], bp[1][1], bp[2][0], bp[2][1]))
						bp = [] # reset
			if len(bp) > 0 :
				print(len(bp), "bezier points left!")
		if len(pts) > 1 :
			diaobj.properties['bez_points'] = pts
		else :
			print("BezPoints", pts)

def MergeParms (d, extra) :
	for k in list(extra.keys()) :
		if k not in d :
			d[k] = extra[k]

##
# \brief Parsing the given dot file
def Parse(sFile) :
	f = open(sFile, 'r')
	s = f.read()
	extra = {}
	nodes = {}
	edges = []
	if 0 : # debug regex
		dbg = rDecl.findall(s)
		for db in dbg :
			print(db)
	for m in rDecl.finditer(s) :
		if m :
			name = StripQuotes(m.group("cmd"))
			if name in keywords :
				if name in extra :
					MergeParms(extra[name], DictFromString(m.group("dict")))
				else :
					extra[name] = DictFromString(m.group("dict"))
			else : # must be a node
				n = Node(name, DictFromString(m.group("dict")))
				if 'node' in extra :
					MergeParms(n.parms, extra['node'])
				nodes[name] = n
	for m in rEdge.finditer(s) :
		if m :
			# the names given are not necessarily registered as nodes already
			defparams = {}
			if 'node' in extra :
				defparams = extra['node']
			for k in ["n1", "n2"] :
				name = StripQuotes(m.group(k))
				if name in nodes :
					pass # defparms should be set above
				else :
					nodes[name] = Node(name, defparams)
			# remember connection
			edges.append(Edge(StripQuotes(m.group("n1")), StripQuotes(m.group("n2")), DictFromString(m.group("dict"))))
	return [nodes, edges]

##
# \brief Adding a label for the edges
#
# This function could be improved if Dia would allow to
# attach labels to arbitrary objects. For the time being
# only the initial postion does match, but relayouting the
# graph in Dia will loose the position
def AddLabel (layer, pos, label, fontsize, center=0) :
	""" create a Text object an put it into the layer """
	textType = dia.get_object_type("Standard - Text")
	obj, h1, h2 = textType.create(pos[0], pos[1])
	#TODO: transfer font-size
	obj.properties["text"] = label
	obj.properties["text_height"] = fontsize
	if center :
		obj.properties["text_alignment"] = 1
		obj.properties["text_vert_alignment"] = 2
	layer.add_object(obj)

##
# \brief Callback registered for the ImportFilter
def ImportFile (sFile, diagramData) :
	""" read the dot file and create diagram objects """
	nodes, edges = Parse(sFile)
	layer = diagramData.active_layer # can do better, e.g. layer per graph
	for key in list(nodes.keys()) :
		n = nodes[key]
		nodeType = dia.get_object_type(n.typename) # could be optimized out of loop
		x, y = n.Pos()
		w, h = n.Size()
		obj, h1, h2 = nodeType.create(x-w/2, y-h/2) # Dot pos is center, Dia (usually) uses top/left
		# resizing the Ellipse by handle is screwed
		# obj.move_handle(h2, (x+w/2, y+h/2), 0, 0) # resize the object
		obj.properties["elem_width"] = w
		obj.properties["elem_height"] = h
		if 'fillcolor' in n.parms :
			try :
				obj.properties['fill_colour'] = n.parms['fillcolor'] # same color syntax?
			except :
				print("Failed to apply:", n.parms['fillcolor'])
		layer.add_object(obj)
		AddLabel (layer, (x,y), n.name, n.FontSize(), 1)
		obj.properties['meta'] = n.parms # copy all (remaining) parameters
		# after creation replace the node with the object (needed to connect them)
		nodes[key] = obj
	for e in edges :
		edgeType = dia.get_object_type(e.typename) # could be optimized out of loop
		x, y = e.Pos() # just to have a start
		con, h1, h2 = edgeType.create(x,y)
		e.SetPoints(con)
		if 'style' in e.parms : # set line style
			con.properties['line_style'] = (4, 0.5) #FIXME: hard-coded dotted
		if 'weight' in e.parms :
			con.properties['line_width'] = float(e.parms['weight']) / 10.0 # arbitray anyway
		layer.add_object(con)
		if e.src in nodes :
			h = con.handles[0]
			obj = nodes[e.src]
			# by moving to the cp position first, the connection's points get recalculated
			pos = obj.connections[8].pos
			con.move_handle(h, pos, 0, 0)
			h.connect(obj.connections[8]) # connect to mid-point
		if e.dest in nodes :
			h = con.handles[-1]
			obj = nodes[e.dest]
			pos = obj.connections[8].pos
			con.move_handle(h, pos, 0, 0)
			h.connect (obj.connections[8]) # connect to mid-point
		if 'label' in e.parms :
			AddLabel (layer, e.LabelPos(), e.parms['label'], e.FontSize())
	diagram = None # FIXME: get it
	if diagram :
		for n, o in nodes.items() :
			diagram.update_connections(o)
		diagram.update_extents()
	return diagramData

if __name__ == '__main__':
	# just testing at the moment
	nodes, edges = Parse(sys.argv[1])
	for k, n in nodes.items() :
		print("Name:", n.name, "Pos:", n.Pos(), "WxH:", n.Size())
	for e in edges :
		print(e.src, "->", e.dest, e.LabelPos(), e.parms)
else :
	# run as a Dia plug-in
	import dia
	dia.register_import (_("Graphviz Dot"), "dot", ImportFile)
