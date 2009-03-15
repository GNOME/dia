#    PyDia Self Documentation Series - Part I : PyDia itself
#    Copyright (c) 2003, 2005 , 2009 Hans Breuer  <hans@breuer.org>
#
#        generates a new diagram which contains all objects
#    of dir(dia). Now fills attributes and operations by using
#    Python reflexion ...
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

import sys, math, dia, types, string

def distribute_objects (objs) :
	width = 0.0
	height = 0.0
	for o in objs :
		if width < o.properties["elem_width"].value :
			width = o.properties["elem_width"].value
		if height < o.properties["elem_height"].value : 
			height = o.properties["elem_height"].value
	# add 20 % 'distance'
	width *= 1.2
	height *= 1.2
	area = len (objs) * width * height
	max_width = math.sqrt (area)
	x = 0.0
	y = 0.0
	dy = 0.0 # used to pack small objects more tightly
	for o in objs :
		if dy + o.properties["elem_height"].value * 1.2 > height :
			x += width
			dy = 0.0
		if x > max_width :
			x = 0.0
			y += height
		o.move (x, y + dy)
		dy += (o.properties["elem_height"].value * 1.2)
		if dy > .75 * height :
			x += width
			dy = 0.0
		if x > max_width :
			x = 0.0
			y += height

def autodoc_cb (data, flags) :
	if not data : # not set when called by the toolbox menu
		diagram = dia.new("PyDiaObjects.dia")
		# passed in data is not necessary valid - we are called from the Toolbox menu
		data = diagram.data
		display = diagram.display()
	else :
		diagram = None
		display = None
	layer = data.active_layer

	oType = dia.get_object_type ("UML - Class")		
	
	theDir = dir(dia)
	# for reflection we need some objects ...
	theObjects = [data, layer, oType]
	try :
		theObjects.append (data.paper)
	except AttributeError :
		pass # no reason to fail with new bindings
	if diagram : 
		theObjects.append (diagram)
	if display : 
		theObjects.append (display)
	# add some objects with interesting properties
	#theObjects.append(dia.DiaImage())
	once = 1
	for s in ["Standard - Image", "Standard - BezierLine", "Standard - Text", 
		"UML - Class", "UML - Dependency"] :
		o, h1, h2 = dia.get_object_type(s).create(0,0)
		for p in o.properties.keys() :
			v = o.properties[p].value
			theObjects.append(v)
			if type(v) is types.TupleType and len(v) > 0 :
				theObjects.append(v[0])
		if once :
			theObjects.append(o)
			theObjects.append(h1)
			theObjects.append(o.bounding_box)
			theObjects.append(o.connections[0])
			theObjects.append(o.handles[0])
			theObjects.append(o.properties)
			theObjects.append(o.properties["obj_pos"])
			once = 0
		# o is leaked here
	#print theObjects
	theTypes = {}
	for s in theDir :
		if s == "_dia" :
			continue # avoid all the messy details ;)
		if theTypes.has_key(s) :
			continue
		for o in theObjects :
			is_a = eval("type(o) is dia." + s)
			#print s, o
			if is_a :
				theTypes[s] = o
				break
		if not theTypes.has_key (s) :
			theTypes[s] = eval ("dia." + s)
	# add UML classes for every object in dir
	#print theTypes

	theGlobals = []
	# remove all objects prefixed with '__'
	for s in theDir :
		if s[:2] == '__' or s[:6] == '_swig_' :
			continue
		if s == "__doc__" or s == "__name__" :
			continue # hidden in the diagram objects but used below
		doc = eval("dia." + s + ".__doc__")
		is_a = eval("type(dia." + s + ") is types.BuiltinMethodType")
		if is_a :
			theGlobals.append((s,doc))
			continue
		o, h1, h2 = oType.create (0,0) # p.x, p.y
		if doc :
			o.properties["comment"] = doc
		layer.add_object (o)
		# set the objects name
		o.properties["name"] = s
		# now populate the object with ...
		if theTypes.has_key(s) :
			t = theTypes[s]
			# ... methods and ...
			methods = []
			# ... attributes
			attributes = []
			members = dir(t)
			stereotype = ""
			if "__getitem__" in members :
				if "__len__" in members :
					stereotype = "sequence"
				elif "has_key" in members and "keys" in members :
					stereotype = "dictionary"
			for m in members :
				# again ignoring underscore prefixed
				if m[:2] == "__" or m == "thisown" : # ignore swig boilerplate
					continue
				try :
					is_m = eval("callable(t." + m + ")")
				except :
					print "type(t." + m + ")?"
					is_m = 0
				doc = ""
				tt = ""
				if 0 : # does not work well enough, giving only sometimes 'int' and often 'property'?
					try : # to detect the (return) type
						if is_m :
							oo = t()
							tt = eval("oo." + m + "().__class__.__name__")
						else :
							tt = eval("t." + m + ".__class__.__name__")
					except TypeError, msg :
						print m, msg
					except AttributeError, msg :
						print m, msg # No constructor defined
				try :
					doc = eval("t." + m + ".__doc__")
				except :
					doc = str(t) + "." + m
				if is_m : # (name, type, comment, stereotype, visibility, inheritance_type, query,class_scope, params)
					methods.append((m,tt,doc,'',0,0,0,0,()))
				else : # (name,type,value,comment,visibility,abstract,class_scope)
					attributes.append((m,tt,'',doc,0,0,0))
			o.properties["operations"] = methods
			o.properties["attributes"] = attributes
			if stereotype != "" :
				o.properties["stereotype"] = stereotype
	# build the module object
	o, h1, h2 = oType.create (0,0) # p.x, p.y
	layer.add_object (o)
	# set the objects name
	o.properties["name"] = "dia"
	o.properties["comment"] = eval("dia.__doc__")
	methods = []
	for s in theGlobals :
		if string.find(s[0], "swigregister") >= 0 :
			continue # just noise
		methods.append((s[0],'',s[1],'',0,0,0,1,()))
	o.properties["operations"] = methods
	# all objects got there bounding box, distribute them
	distribute_objects (layer.objects)

	if diagram :
		diagram.update_extents()
		diagram.flush()
	# work with bindings test
	return data

dia.register_action ("HelpPydia", "PyDia Docs", 
                       "/ToolboxMenu/Help/HelpExtensionStart", 
                       autodoc_cb)
