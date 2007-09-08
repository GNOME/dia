#!/usr/bin/env python

#   Copyright (c) 2007, Hans Breuer <hans@breuer.org>
#
#    using Doxygen's XML to reverse engineer C++
#
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

#
# Improvements/ToDo
#
#  - actually call doxygen to make the xml
#    - for directories, additional INCLUDE, etc. with GUI
#    - delete temporary files or better make doxgen output them on stdout? 
#    - have a way in the plug-in interface to import multiple files at once?
#    - another way could be to let doxygen put everything in one file?
#
# - for single class import connect by searching base classes in the diagram
#
# - some layout algorithm based on inheritance
#   - maybe this should be done as an extra script/plug-in becasue it could be 
#     useful on already existing diagrams as well
#   - some smart way to handle size change with 'visible comments', maybe
#     also tweaking wtih wrap-after-char
#

import glob, os, string

# debug spew
g_verbose = 0

# base class, not a tag
class Element :
	def __init__ (self) :
		self.name = ""
		self.kind = ""
		self.type = ""
		self.props = {}
		self.attrs = None
	def __str__ (self) :
		return "<" + self.__class__.__name__ + ">" + self.kind + ":" + self.name 
	def Set (self, key, value) :
		if key in ["compoundname", "name", "declname", "array"] :
			if len(self.name) == 0 :
				self.name = value
			else :
				self.name += " " + value
		elif key in ["type"] : # retval for functions
			if len (self.type) == 0 :
				self.type = value
			else :
				self.type += " " + value
		elif key == "ref" :
			if self.__class__.__name__ in ["Param", "Memberdef"] :
				if len(self.type) == 0 :
					self.type = value
				else :
					self.type += " " + value
		else :
			self.props[key] = value
	def SetAttrs (self, name, attrs) :
		# just a copy of the tags attributes
		if self.__class__.__name__ == string.capitalize(name) :
			self.attrs = attrs
		elif len(attrs.keys()) > 0 :
			if g_verbose :
				print "Ignoring", name, "attrs on", self.kind
	def Dump (self) :
		print self.kind, self.name, self.props
# still no tag, fallback
class Unknown(Element) :
	def __init__ (self, name, n) :
		self.name = name
		self.kind = name
		print "Unknown", " " * n, name
	def Dump (self) :
		pass
	def Set (self, key, value) :
		print "???", key, value
	def SetAttrs (self, name, attrs) :
		pass

# e.g. class, struct but also file, the last one not handled here
class Compounddef(Element) :
	def __init__ (self, kind) :
		Element.__init__(self)
		self.kind = kind
		self.visibilities = []
		self.id = ""
		self.bases = []
	def Add (self, o) :
		self.visibilities.append(o)
	def SetAttrs (self, name, attrs) :
		if name == "compounddef" :
			self.id = attrs["id"]
		elif name == "basecompoundref" :
			if g_verbose : print "Base", attrs["refid"]
			self.bases.append (attrs["refid"])
		elif name == "derivedcompoundref" :
			pass
		else :
			Element.SetAttrs (self, name, attrs)
	def Dump (self) :
		print self.kind, self.name
		for v in self.visibilities :
			v.Dump ()
	# these functions are dia specific (layout in the lists) alsthough not using dia facilities
	def GetAttributes (self) :
		attributes = []
		for v in self.visibilities :
			if v.__class__.__name__ != "Sectiondef" :
				if not v.__class__.__name__ in ["Briefdescription", "Detaileddescription", "Listofallmembers"] :
					print "***", v.__class__.__name__
				continue
			visibility = v.GetVisibility()
			value = ""
			for m in v.members :
				if m.kind != "variable" :
					break # todo?
				comment = ""
				for p in m.parameters :
					# FIXME: wrong list, there are no parameters on an attribute
					comment += p.text					
				attributes.append ((m.name, m.type, value, comment, visibility, 0, m.IsStatic()))
		return attributes
	def GetMethods (self) :
		methods = []
		for v in self.visibilities :
			if v.__class__.__name__ != "Sectiondef" :
				if not v.__class__.__name__ in ["Briefdescription", "Detaileddescription", "Listofallmembers"] :
					print "***", v.__class__.__name__
				continue
			visibility = v.GetVisibility() 
			if visibility is None :
				continue # friend does not belong here
			inheritance_type = 0 # (abstract, virtual, final)
			for m in v.members :
				if m.kind != "function" :
					break # todo?
				params = []
				comment = ""
				for p in m.parameters :
					# name, type, value, comment, kind
					if p.kind == "param" :
						# (name, type, value, comment, kind (undef, in, out, ...))
						params.append ((p.name, p.type, None, p.GetComment(), 0))
					elif p.kind == "comment" : # FIXME: wrong list
						comment += p.text
				# op : a tuple with fixed placing, see: objects/UML/umloperations.c:umloperation_props
				if m.GetVisibility () :
					visibility = m.GetVisibility()
				# (name, type, comment, stereotype, visibility, inheritance_type, query, class_scope, params)
				methods.append ((m.name, m.type, comment, "", visibility, 
						m.GetInheritanceType(), m.IsQuery(), m.IsStatic(), params))
		return methods
	def IsAbstract (self) :
		return 0

# stuff like public-type (visibility)
class Sectiondef(Element) :
	def __init__ (self, kind) :
		Element.__init__(self)
		self.kind = kind
		self.members = []
	def Add (self, o) :
		self.members.append (o)
	def Dump (self) :
		print " ", self.kind, ":"
		for m in self.members :
			m.Dump ()
	# Dia specifics, map tags/attribs to Dia enums
	def GetVisibility (self) :
		# (0 : public, 1 : protected, 2 : private)
		if "public" in self.kind : return 0
		elif "protected" in self.kind : return 1
		elif "private" in self.kind : return 2
		elif "user-defined" in self.kind : return 0 # FIXME: sectiondef not really defines visibility
		else :
			print self.kind, "visibility?"
			return None
# e.g. function, enum
class Memberdef(Element) :
	def __init__ (self, kind) :
		Element.__init__(self)
		if kind in ["enum"] : 
			self.type = kind 
		self.kind = kind
		self.parameters = []
	def Add (self, o) :
		self.parameters.append (o)
	def Dump (self) :
		print "\t", self.type, self.name
		for p in self.parameters :
			p.Dump ()
	# again Dia dependent, although not using it
	def GetInheritanceType (self) :
		# inheritance_type (0: abstract, 1 : virtual, 2:leaf)
		if self.attrs and self.attrs.has_key ("virt") :
			if self.attrs["virt"] == "pure-virtual" :
				return 0
			elif self.attrs["virt"] == "non-virtual" :
				return 2 # leaf, final
			elif self.attrs["virt"] == "virtual" :
				return 1
			else :
				print 'virt="' + self.attrs["virt"] + '" ???'
				return None
		return 0
	def GetVisibility (self) :
		if self.attrs and self.attrs.has_key ("prot") :
			if self.attrs["prot"] == "public" : return 0
			elif self.attrs["prot"] == "protected" : return 1
			elif self.attrs["prot"] == "private" : return 2
		return None
	def IsStatic (self) :
		# class_scope
		if self.attrs and self.attrs.has_key ("static") :
			if self.attrs["static"] != "no" :
				return 1
		return 0
	def IsQuery (self) :
		# const members
		if self.attrs and self.attrs.has_key ("const") :
			if self.attrs["const"] != "no" :
				return 1
		return 0
				
class Enumvalue(Element) :
	def __init__ (self) :
		Element.__init__(self)
		self.kind = ""
	def Dump (self) :
		print "\t" * 2, self.name
	def Add (self, o) :
		pass # throwing away a comment
class Param(Element) :
	"Parameter, only as _function_ parameter? Check e.g. template parameters some day ..."
	def __init__ (self) :
		Element.__init__(self)
		self.kind = "param"
		self.comments = []
	def _Set (self, key, value) :
		if key == "ref" :
			self.type = value
	def Add (self, o) :
		self.comments.append (o)
	def GetComment (self) :
		s = ""
		for o in self.comments :
			s += o.text
	def Dump (self) :
		print "\t" * 2, self.type, self.name
class Parameterlist(Element) :
	def __init__ (self, kind) :
		Element.__init__(self)
		self.kind = kind
	def Set (self, key, value) :
		# to be implemented if we are interested in parameter details
		pass
	def Add (self, o) :
		pass
	def Dump (self) :
		pass
class Comment :
	"Base class for all kinds of comment 'tags', maybe at some point they should be distinguishable?"
	def __init__ (self) :
		self.kind = "comment"
		self.text = ""
	def Set (self, key, value) :
		self.text += " " + value
	def Add (self, o) :
		# no matter what's below, everything consumed by Set()
		pass
	def SetAttrs (self, name, attrs) :
		pass
	def Dump (self) :
		print "\t#", self.text
class Briefdescription(Comment) :
	def __init__ (self) :
		Comment.__init__(self)
class Detaileddescription(Comment) :
	def __init__ (self) :
		Comment.__init__(self)
class Parameterdescription(Comment) :
	def __init__ (self) :
		Comment.__init__(self)


# only to avoid it overwriting all the previous accumulated data
class Dummy(Element) :
	def __init__ (self) :
		Element.__init__(self)
	def Set (self, key, value) :
		pass
	def Dump (self) :
		pass
class Listofallmembers(Dummy) :
	def __init__ (self) :
		Dummy.__init__(self)
	def Add (self, o) :
		pass
class Member(Dummy) :
	def __init__ (self) :
		Dummy.__init__(self)

def find_object (ctx) :
	o = None
	for i in range(1,len(ctx)) :
		# find an element not None
		if ctx[-i][1] :
			o = ctx[-i][1]
			break
	return o

# a SAX based parser turning XML tags into Python code
def Parse (sData) :
	import xml.parsers.expat
	unhandled = {}
	ctx = []
	classes = []
	# 3 handler functions
	def start_element(name, attrs) :
		parent = find_object (ctx)
		s = string.capitalize(name)
		try :
			o = eval (s + '("' + attrs["kind"] + '")')
		except NameError :
			print "Handle?", s
			o = Unknown(name, len(ctx))
		except KeyError :
			try :
				o = eval (s + "()")
			except NameError :
				o = None
		if o :
			o.SetAttrs (name, attrs)
		elif parent :
			# this is not the original object, i.e. o.kind != name
			# so we don't need an own python class for every tag to be handled
			parent.SetAttrs (name, attrs)
		ctx.append((name, o)) #push
		# build containers
		if parent and o :
			try :
				parent.Add (o)
			except AttributeError :
				print "Handle(?)", parent, "Add(", o, ")"
				pass
	def char_data(data) :
		# we are not interested in pure whitespace
		data = string.strip (data)
		if len(data) == 0 :
			return
		try :
			name = ctx[-1][0]
			o = find_object (ctx)
			if o :
				#print "\t" * 4, o, "<", name, data, ">"
				o.Set (name, data)
			else :
				if not unhandled.has_key (ctx[-1][0]) :
					print "ToDo?", ctx[-1][0]
					unhandled[ctx[-1][0]] = 1
				#ctx[-1][1].Set (data)
		except KeyError :
			pass
	def end_element(name) :
		o = ctx[-1][1]
		if o :
			if o.kind == "class" or o.kind == "struct" :
				classes.append (o)
		del ctx[-1] # pop
	# do it
	p = xml.parsers.expat.ParserCreate()
	p.StartElementHandler = start_element
	p.EndElementHandler = end_element
	p.CharacterDataHandler = char_data

	p.Parse(sData)

	return classes

def GetClasses (files) :
	classes = []
	for s in files :
		f = open (s)
		try :
			classes.extend (Parse (f.read()))
		except xml.parsers.expat.ExpatError :
			print "XML Error:", s
	# before adding them to the diagram sort by inheritance (less first)
	return classes

def GenerateXML (sPath) :
	# run doxygen to create the XML to be imported by this plu-in
	os.chdir (sPath)
	f = open (sPath + "/doxyfile.rev", "w")
	f.write ("""
INPUT = .
# change defaults
GENERATE_HTML = NO
GENERATE_LATEX = NO
GENERATE_XML = YES
# against advise (on windoze)
CASE_SENSE_NAMES = YES
# xml specifics
XML_PROGRAMLISTING = NO
""")
	p = os.popen ("doxygen doxyfile.rev")
	s = p.readline()
	while s :
		print s[:-1]
		s = p.readline()
	p.close ()

if __name__ == '__main__':
	GenerateXML (os.getcwd())
	files = glob.glob("xml/class*.xml")
	classes = GetClasses (files)
	for o in classes :
		o.Dump()
		print o.GetMethods()
		print o.GetAttributes()
else :
	import dia
	
	def import_classes (classes, diagramData) :
		class_map = {}
		layer = diagramData.active_layer
		# create the classes
		for c in classes :
			o, h1, h2 = dia.get_object_type("UML - Class").create(0,0)
			# origin and dia object mapped by id
			class_map[c.id] = (c, o)
			# set some properties 
			o.properties["name"] = str(c.name)
			o.properties["abstract"] = c.IsAbstract ()
			try :
				o.properties["operations"] = c.GetMethods()
			except TypeError, msg :
				print "Failed assigning 'operations':", msg, c.GetMethods()
			o.properties["attributes"] = c.GetAttributes()

			layer.add_object(o)
		# create the links (inhertiance, maybe containement, refernces, factory, ...)
		for s in class_map.keys () :
			c, o = class_map[s]
			print s, c, c.bases
			for b in c.bases :
				if class_map.has_key (b) :
					k, p = class_map[b]
					print c.id, "->", k.id
					con, h1, h2 = dia.get_object_type("UML - Generalization").create(0,0)
					layer.add_object (con)
					h1.connect (p.connections[6])
					h2.connect (o.connections[1])
		# ... and move appropriately
		
		# update placement depending on number of parents ?
		layer.update_extents()

	def import_files (sFile, diagramData) :
		# maybe we should select single file/directory, add inlude directories here
		files = glob.glob (os.path.dirname (sFile) + "/class*.xml")
		files.extend (glob.glob (os.path.dirname (sFile) + "/struct*.xml"))
		classes = GetClasses (files)
		return import_classes (classes, diagramData)

	def import_file (sFile, diagramData) :
		classes = GetClasses ([sFile])
		return import_classes (classes, diagramData)

	dia.register_import("Dox2UML", "xml", import_file)
	dia.register_import("Dox2UML (multiple)", "xml", import_files)
	#dia.register_import("Dox2UML", "h", import_headers)
