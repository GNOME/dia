#  PyDia SVG Import
#  Copyright (c) 2003, 2004 Hans Breuer <hans@breuer.org>
#
#  Pure Python Dia Import Filter - to show how it is done.
#  It also tries to be more featureful and robust then the 
#  SVG importer written in C, but as long as PyDia has issues 
#  this will _not_ be the case. Known issues (at least) :
#  - xlink stuff (should probably have some StdProp equivalent)
#  - lack of full transformation dealing
#  - real percentage scaling, is it worth it ? 
#  - see FIXME in this file

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

import string, math, os, re

# Dias unit is cm, the default scale should be determined from svg:width and viewBox
dfPcm = 35.43307
dfUserScale = 1.0
dfFontSize = 0.7
dfViewLength = 32.0 # wrong approach for "% unit" 
dictUnitScales = {
	"em" : 1.0, "ex" : 2.0, #FIXME these should be _relative_ to current font
	"px" : 1.0 / dfPcm, "pt" : 1.25 / dfPcm, "pc" : 15.0 / dfPcm, 
	"cm" : 35.43307 / dfPcm, "mm" : 3.543307 / dfPcm, "in" : 90.0 / dfPcm}

# only compile once
rColor = re.compile(r"rgb\s*\(\s*(\d+)[, ]+(\d+)[, +](\d+)\s*\)")
# not really parsing numbers (Scaled will deal with more)
rTranslate = re.compile(r"translate\s*\(\s*([^,]+),([^)]+)\s*\)")
#FIXME: parse more - e.g. AQT - of the strange path data
rPathWhat = re.compile("[MmLlCcSsZz]")   # what
rPathData = re.compile("[^MmLlCcSsZz]+") # data
rPathValue = re.compile("[\s,]+") # values

def Scaled(s) :
	# em, ex, px, pt, pc, cm, mm, in, and percentages
	if s[-1] in string.digits :
		# use global scale
		return float(s) * dfUserScale
	else :
		unit = s[-2:]
		try :
			if unit[0] == "e" :
				#print "Scaling", unit, dfFontSize
				return float(s[:-2]) * dfFontSize * dictUnitScales[unit]
			else :
				return float(s[:-2]) * dictUnitScales[unit]
		except :
			if s[-1] == "%" :
				return float(s[:-1]) * dfViewLength / 100.0
			# warn about invalid unit ??
			raise NotImplementedError("Unknown unit %s %s" % (s[:-2], s[-2:]))
			return float(s) * dfUserScale
def Color(s) :
	# deliver a StdProp compatible Color (or the original string)
	m = rColor.match(s)
	if m :
		return (int(m.group(1)) / 255.0, int(m.group(2)) / 255.0, int(m.group(2)) / 255.0)
	# any more ugly color definitions not compatible with pango_color_parse() ?
	return string.strip(s)
def _eval (s, _locals) :
	# eval() can be used to execute aribitray code, see e.g. http://bugzilla.gnome.org/show_bug.cgi?id=317637
	# here using *any* builtins is an abuse
	try :
		return eval (s, {'__builtins__' : None }, _locals)
	except NameError :
		try :
			import dia
			dia.message(2, "***Possible exploit attempt***:\n" + s)
		except ImportError :
			print "***Possible exploit***:", s
	return None
class Object :
	def __init__(self) :
		self.props = {"x" : 0, "y" : 0, "stroke" : "none"}
		self.translation = None
		# "line_width", "line_colour", "line_style"
	def style(self, s) :
		sp1 = string.split(s, ";")
		for s1 in sp1 :
			sp2 = string.split(string.strip(s1), ":")
			if len(sp2) == 2 :
				try :
					_eval("self." + string.replace(sp2[0], "-", "_") + "(\"" + string.strip(sp2[1]) + "\")", locals())
				except AttributeError :
					self.props[sp2[0]] = string.strip(sp2[1])
	def x(self, s) :
		self.props["x"] = Scaled(s)
	def y(self, s) :
		self.props["y"] = Scaled(s)
	def width(self, s) :
		self.props["width"] = Scaled(s)
	def height(self, s) :
		self.props["height"] = Scaled(s)
	def stroke(self,s) :
		self.props["stroke"] = s.encode("UTF-8")
	def stroke_width(self,s) :
		self.props["stroke-width"] = Scaled(s)
	def fill(self,s) :
		self.props["fill"] = s
	def fill_rule(self,s) :
		self.props["fill-rule"] = s
	def stroke_dasharray(self,s) :
		# just an approximation
		sp = string.split(s,",")
		n = len(sp)
		if n > 0 :
			# sp[0] == "none" : # ? stupid generator ?
			try :
				dlen = Scaled(sp[0])
			except :
				n = 0
		if n == 0 : # should not really happen
			self.props["line-style"] = (0, 1.0) # LINESTYLE_SOLID,
		elif n == 2 : 
			if dlen > 0.1 : # FIXME:  
				self.props["line-style"] = (1, dlen) # LINESTYLE_DASHED,
			else :
				self.props["line-style"] = (4, dlen) # LINESTYLE_DOTTED
		elif n == 4 :  
			self.props["line-style"] = (2, dlen) # LINESTYLE_DASH_DOT,
		elif n == 6 : 
			self.props["line-style"] = (3, dlen) # LINESTYLE_DASH_DOT_DOT,
	def id(self, s) :
		# just to handle/ignore it
		self.props["meta"] = { "id" : s }
	def transform(self, s) :
		m = rTranslate.match(s)
		if m :
			#print "matched", m.group(1), m.group(2), "->", Scaled(m.group(1)), Scaled(m.group(2))
			self.translation = (Scaled(m.group(1)), Scaled(m.group(2)))
	def __repr__(self) :
		return self.dt + " : " + str(self.props)
	def Dump(self, indent) :
		print " " * indent, self
	def Set(self, d) :
		pass
	def ApplyProps(self, o) :
		pass
	def CopyProps(self, dest) :
		# to be used to inherit group props to childs _before_ they get their own
		# doesn't use the member functions to avoid scaling once more
		for p in self.props.keys() :
			dest.props[p] = self.props[p]
	def Create(self) :
		ot = dia.get_object_type (self.dt)
		o, h1, h2 = ot.create(self.props["x"], self.props["y"])
		# apply common props
		if self.props.has_key("stroke-width") and o.properties.has_key("line_width") :
			o.properties["line_width"] = self.props["stroke-width"]
		if self.props.has_key("stroke") and o.properties.has_key("line_colour") :
			if self.props["stroke"] != "none" :
				try :
					o.properties["line_colour"] = Color(self.props["stroke"])
				except :
					# rgb(192,27,38) handled by Color() but ...
					# o.properties["line_colour"] = self.props["stroke"]
					pass
			else :
				# Dia can't really display stroke none, some workaround :
				if self.props.has_key("fill") and self.props["fill"] != "none" : 
					#does it really matter ?
					try :
						o.properties["line_colour"] = Color(self.props["fill"])
					except :
						pass
				o.properties["line_width"] = 0.0
		if self.props.has_key("fill") and o.properties.has_key("fill_colour") :
			if self.props["fill"] == "none" :
				o.properties["show_background"] = 0
			else :
				color_key = "fill_colour"
				try :
					o.properties["show_background"] = 1
				except KeyError :
					# not sure if this is always true
					color_key = "text_colour"
				try :
					o.properties[color_key] =Color(self.props["fill"])
				except :
					# rgb(192,27,38) handled by Color() but ...
					# o.properties["fill_colour"] =self.props["fill"]
					pass
		if self.props.has_key("line-style") and o.properties.has_key("line_style") :
			o.properties["line_style"] = self.props["line-style"]
		if self.props.has_key("meta") and o.properties.has_key("meta") :
			o.properties["meta"] = self.props["meta"]
		self.ApplyProps(o)
		return o

class Svg(Object) :
	# not a placeable object but similar while parsing
	def __init__(self) :
		Object.__init__(self)
		self.dt = "svg"
		self.bbox_w = None
		self.bbox_h = None
	def width(self,s) :
		global dfUserScale
		d = dfUserScale
		dfUserScale = 0.05
		self.bbox_w = Scaled(s)
		self.props["width"] = self.bbox_w
		dfUserScale = d
	def height(self,s) :
		global dfUserScale
		d = dfUserScale
		# with stupid info Dia still has a problem cause zooming is limited to 5.0%
		dfUserScale = 0.05
		self.bbox_h = Scaled(s)
		self.props["height"] = self.bbox_h
		dfUserScale = d
	def viewBox(self,s) :
		global dfUserScale
		global dfViewLength
		self.props["viewBox"] = s
		sp = string.split(s, " ")
		w = float(sp[2]) - float(sp[0])
		h = float(sp[3]) - float(sp[1])
		# FIXME: the following relies on the call order of width,height,viewBox
		# which is _not_ the order it is in the file
		if self.bbox_w and self.bbox_h :
			dfUserScale = math.sqrt((self.bbox_w / w)*(self.bbox_h / h))
		elif self.bbox_w :
			dfUserScale = self.bbox_w / w
		elif self.bbox_h :
			dfUserScale = self.bbox_h / h
		# FIXME: ugly, simple aproach to "%" unit
		dfViewLength = math.sqrt(w*h)
	def xmlns(self,s) :
		self.props["xmlns"] = s
	def version(self,s) :
		self.props["version"] = s
	def __repr__(self) :
		global dfUserScale
		return Object.__repr__(self) + "\nUserScale : " + str(dfUserScale)
	def Create(self) :
		return None
class Style(Object) :
	# the beginning of a css implementation, ...
	def __init__(self) :
		global cssStyle
		Object.__init__(self)
		self.cdata = ""
		self.styles = None
		cssStyle = self
	def type(self, s) :
		self.props["type"] = s
	def Set(self, d) :
		# consuming all the ugly CDATA
		self.cdata += d
	def Lookup(self, st) :
		if self.styles == None :
			self.styles = {}
			# just to check if we are interpreting correctly (better use regex ?)
			p1 = 0 # position of dot
			p2 = 0 # ... of opening brace
			p3 = 0 # ... closing
			s = self.cdata
			n = len(s) - 1
			while 1 :
				p1 = string.find(s, ".", p3, n)
				p2 = string.find(s, "{", p1+1, n)
				p3 = string.find(s, "}", p2+1, n)
				if p1 < 0 or p2 < 0 or p3 < 0 :
					break
				print s[p1+1:p2-1], s[p2+1:p3]
				self.styles[s[p1+1:p2-1]] = s[p2+1:p3]
		if self.styles.has_key(st) :
			return self.styles[st]
		return ""
	def __repr__(self) :
		self.Lookup("init") # fill the dictionary
		return "Styles:" + str(self.styles)
	def Create(self) :
		return None

cssStyle = Style() # a singleton

class Group(Object) :
	def __init__(self) :
		Object.__init__(self)
		self.dt = "Group"
		self.childs = []
	def Add(self, o) :
		self.childs.append(o)
	def Create(self) :
		lst = []
		for o in self.childs :
			od = o.Create()
			if od :
				#print od
				#DON'T : layer.add_object(od)
				lst.append(od)
		# create group including list objects
		if len(lst) > 0 :
			grp = dia.group_create(lst)
			if self.translation :
				# want to move by top left corner ...
				hNW = grp.handles[0] # HANDLE_RESIZE_NW
				# ... but pos is the point moved
				pos = grp.properties["obj_pos"].value
				#FIXME:  looking at scascale.py this isn't completely correct
				x1 = hNW.pos.x + self.translation[0]
				y1 = hNW.pos.y + self.translation[1]
				grp.move(x1, y1)
			return grp
		else :
			return None
	def Dump(self, indent) :
		print " " * indent, self
		for o in self.childs :
			o.Dump(indent + 1)

# One of my test files is quite ugly (produced by Batik) : it dumps identical image data 
# multiple times into the svg. This directory helps to reduce them to the necessary
# memory comsumption
_imageData = {} 

class Image(Object) :
	def __init__(self) :
		Object.__init__(self)
		self.dt = "Standard - Image"
	def preserveAspectRatio(self,s) :
		self.props["keep_aspect"] = s
	def xlink__href(self,s) :
		#print s
		if s[:8] == "file:///" :
			self.props["uri"] = s.encode("UTF-8")
		elif s[:22] == "data:image/png;base64," :
			if _imageData.has_key(s[22:]) :
				self.props["uri"] = _imageData[s[22:]] # use file reference
			else :
				# an ugly temporary file name, on windoze in %TEMP%
				fname = os.tempnam(None, "diapy-") + ".png"
				dd = s[22:].decode ("base64")
				f = open(fname, "wb")
				f.write(dd)
				f.close()
				# not really an uri but the reader appears to be robust enough ;-)
				_imageData[s[22:]] = "file:///" + fname
		else :
			pass #FIXME how to import data into dia ??
	def Create(self) :
		if not (self.props.has_key("uri") or self.props.has_key("data")) :
			return None
		return Object.Create(self)
	def ApplyProps(self,o) :
		if self.props.has_key("width") :
			o.properties["elem_width"] = self.props["width"]
		if self.props.has_key("width") :
			o.properties["elem_height"] = self.props["height"]
		if self.props.has_key("uri") :
			o.properties["image_file"] = self.props["uri"][8:] 
class Line(Object) :
	def __init__(self) :
		Object.__init__(self)
		self.dt = "Standard - Line"
		# "line_width". "line_color"
		# "start_point". "end_point"
	def x1(self, s) :
		self.props["x"] = Scaled(s)
	def y1(self, s) :
		self.props["y"] = Scaled(s)
	def x2(self, s) :
		self.props["x2"] = Scaled(s)
	def y2(self, s) :
		self.props["y2"] = Scaled(s)
	def ApplyProps(self, o) :
		#pass
		o.properties["end_point"] = (self.props["x2"], self.props["y2"])
class Path(Object) :
	def __init__(self) :
		Object.__init__(self)
		self.dt = "Standard - BezierLine" # or Beziergon ?
		self.pts = []
	def d(self, s) :
		self.props["data"] = s
		#FIXME: parse more - e.g. AQT - of the strange path data
		spd = rPathWhat.split(s)
		spw = rPathData.split(s)
		i = 1
		# current point
		xc = 0.0; yc = 0.0 # the current or second control point - ugly svg states ;(
		for s1 in spw :
			k = 0 # range further adjusted for last possibly empty -k-1
			if s1 == "M" : # moveto
				sp = rPathValue.split(spd[i])
				if sp[0] == "" : k = 1
				xc = Scaled(sp[k]); yc = Scaled(sp[k+1])
				self.pts.append((0, xc, yc))
			elif s1 == "L" : #lineto
				sp = rPathValue.split(spd[i])
				if sp[0] == "" : k = 1
				for j in range(k, len(sp)-k-1, 2) :
					xc = Scaled(sp[j]); yc = Scaled(sp[j+1])
					self.pts.append((1, xc, yc))
			elif s1 == "C" : # curveto
				sp = rPathValue.split(spd[i])
				if sp[0] == "" : k = 1
				for j in range(k, len(sp)-k-1, 6) :
					self.pts.append((2, Scaled(sp[j]), Scaled(sp[j+1]), 
									Scaled(sp[j+2]), Scaled(sp[j+3]),
									Scaled(sp[j+4]), Scaled(sp[j+5])))
					# reflexion second control to current point, really ?
					xc =2 * Scaled(sp[j+4]) - Scaled(sp[j+2])
					yc =2 * Scaled(sp[j+5]) - Scaled(sp[j+3])
			elif s1 == "S" : # smooth curveto
				sp = rPathValue.split(spd[i])
				if sp[0] == "" : k = 1
				for j in range(k, len(sp)-k-1, 4) :
					x = Scaled(sp[j+2])
					y = Scaled(sp[j+3])
					x1 = Scaled(sp[j])
					y1 = Scaled(sp[j+1])
					self.pts.append((2, xc, yc,  # FIXME: current point ?
									x1, y1,
									x, y))
					xc = 2 * x - x1; yc = 2 * y - y1
			elif s1 == "z" or s1 == "Z" : # close
				self.dt = "Standard - Beziergon"
			elif s1 == "" : # too much whitespaces ;-)
				pass
			else :
				print "Huh?", s1
				break
			i += 1
	def ApplyProps(self,o) :
		o.properties["bez_points"] = self.pts
	def Dump(self, indent) :
		print " " * indent, self
		for t in self.pts :
			print " " * indent, t
	#def Create(self) :
	#	return None # not yet
class Rect(Object) :
	def __init__(self) :
		Object.__init__(self)
		self.dt = "Standard - Box"
		# "corner_radius", 
	def ApplyProps(self,o) :
		o.properties["elem_width"] = self.props["width"]	
		o.properties["elem_height"] = self.props["height"]	
class Ellipse(Object) :
	def __init__(self) :
		Object.__init__(self)
		self.dt = "Standard - Ellipse"
		self.props["cx"] = 0
		self.props["cy"] = 0
		self.props["rx"] = 1
		self.props["ry"] = 1
	def cx(self,s) :
		self.props["cx"] = Scaled(s)
		self.props["x"] = self.props["cx"] - self.props["rx"]
	def cy(self,s) :
		self.props["cy"] = Scaled(s)
		self.props["y"] = self.props["cy"] - self.props["ry"]
	def rx(self,s) :
		self.props["rx"] = Scaled(s)
		self.props["x"] = self.props["cx"] - self.props["rx"]
	def ry(self,s) :
		self.props["ry"] = Scaled(s)
		self.props["y"] = self.props["cy"] - self.props["ry"]
	def ApplyProps(self,o) :
		o.properties["elem_width"] = 2.0 * self.props["rx"]	
		o.properties["elem_height"] = 2.0 * self.props["ry"]
class Circle(Ellipse) :
	def __init__(self) :
		Ellipse.__init__(self)
	def r(self,s) :
		Ellipse.rx(self,s)
		Ellipse.ry(self,s)
class Poly(Object) :
	def __init__(self) :	
		Object.__init__(self)
		self.dt = None # abstract class !
	def points(self,s) :
		sp1 = string.split(s)
		pts = []
		for s1 in sp1 :
			sp2 = string.split(s1, ",")
			if len(sp2) == 2 :
				pts.append((Scaled(sp2[0]), Scaled(sp2[1])))
		self.props["points"] = pts
	def ApplyProps(self,o) :
		o.properties["poly_points"] = self.props["points"]
class Polygon(Poly) :
	def __init__(self) :
		Poly.__init__(self)
		self.dt = "Standard - Polygon"
class Polyline(Poly) :
	def __init__(self) :
		Poly.__init__(self)
		self.dt = "Standard - PolyLine"
class Text(Object) :
	def __init__(self) :
		Object.__init__(self)
		self.dt = "Standard - Text"
		self.props["font-size"] = 1.0
		# text_font, text_height, text_color, text_alignment
	def Set(self, d) :
		if self.props.has_key("text") :
			self.props["text"] += d
		else :
			self.props["text"] = d
	def text_anchor(self,s) :
		self.props["text-anchor"] = s
	def font_size(self,s) :
		global dfFontSize
		# ugh, just maintain another global state
		if s[-2:-1] != "e" : # FIXME ???
			dfFontSize = Scaled(s)
			#print "FontSize is", dfFontSize
		self.props["font-size"] = Scaled(s)
		# ?? self.props["y"] = self.props["y"] - Scaled(s)
	def font_weight(self, s) :
		self.props["font-weight"] = s
	def font_style(self, s) :
		self.props["font-style"] = s
	def font_family(self, s) :
		self.props["font-family"] = s
	def ApplyProps(self, o) :
		o.properties["text"] = self.props["text"].encode("UTF-8")
		if self.props.has_key("text-anchor") :
			if self.props["text-anchor"] == "middle" : o.properties["text_alignment"] = 1
			elif self.props["text-anchor"] == "end" : o.properties["text_alignment"] = 2
			else : o.properties["text_alignment"] = 0
		if self.props.has_key("fill") :
			o.properties["text_colour"] = Color(self.props["fill"])
		if self.props.has_key("font-size") :
			o.properties["text_height"] = self.props["font-size"]
class Desc(Object) :
	#FIXME is this useful ?
	def __init__(self) :
		Object.__init__(self)
		self.dt = "UML - Note"
	def Set(self, d) :
		if self.props.has_key("text") :
			self.props["text"] += d
		else :
			self.props["text"] = d
	def Create(self) :
		if self.props.has_key("text") :
			pass
			#dia.message(0, self.props["text"].encode("UTF-8"))
		return None
class Title(Object) :
	#FIXME is this useful ?
	def __init__(self) :
		Object.__init__(self)
		self.dt = "UML - LargePackage"
	def Set(self, d) :
		if self.props.has_key("text") :
			self.props["text"] += d
		else :
			self.props["text"] = d
	def Create(self) :
		if self.props.has_key("text") :
			pass
		return None
class Unknown(Object) :
	def __init__(self, name) :
		Object.__init__(self)
		self.dt = "svg:" + name
	def Create(self) :
		return None

class Importer :
	def __init__(self) :
		self.errors = {}
		self.objects = []
	def Parse(self, sData) :
		import xml.parsers.expat
		ctx = []
		stack = []
		# 3 handler functions
		def start_element(name, attrs) :
			#print "<" + name + ">"
			if 0 == string.find(name, "svg:") :
				name = name[4:]
			if len(stack) > 0 :
				grp = stack[-1]
			else :
				grp = None
			if 'g' == name :
				o = Group()
				stack.append(o)
			elif 'tspan' == name :
				#FIXME: to take all the style coming with it into account 
				# Dia would need to support layouted text ...
				txn, txo = ctx[-1]
				if attrs.has_key("dy") :
					txo.Set("" + "\n") # just a new line (best we can do?)
				elif attrs.has_key("dx") :
					txo.Set(" ")
				ctx.append((txn, txo)) #push the same object
				return
			else :
				s = string.capitalize(name) + "()"
				try :
					# should be safe to use eval() here, by XML rules it can just be a name or would give
					# xml.parsers.expat.ExpatError: not well-formed (invalid token)
					o = eval(s)
				except :
					o = Unknown(name)
			if grp :
				grp.CopyProps(o)
			for a in attrs :
				if a == "class" : # eeek : keyword !
					st = cssStyle.Lookup(attrs[a])
					o.style(st)
					o.props[a] = attrs[a]
					continue
				ma = string.replace(a, "-", "_")
				# e.g. xlink:href -> xlink__href
				ma = string.replace(ma, ":", "__")
				s = "o." +  ma + "(\"" + attrs[a] + "\")"
				try :
					_eval(s, locals())
				except AttributeError, msg :
					o.props["meta"] = { a : attrs[a] }
					if not self.errors.has_key(msg) :
						self.errors[msg] = s
				except SyntaxError, msg :
					if not self.errors.has_key(msg) :
						self.errors[msg] = s
			if grp is None :
				self.objects.append(o)
			else :
				grp.Add(o)
			ctx.append((name, o)) #push
		def end_element(name) :
			if 'g' == name :
				del stack[-1]
			del ctx[-1] # pop
		def char_data(data):
			# may be called multiple times for one string
			ctx[-1][1].Set(data)

		p = xml.parsers.expat.ParserCreate()
		p.StartElementHandler = start_element
		p.EndElementHandler = end_element
		p.CharacterDataHandler = char_data

		p.Parse(sData)

	def Render(self,data) :
		layer = data.active_layer
		for o in self.objects :
			try :
				od = o.Create()
			except TypeError, e :
				od = None
				dia.message(1, "SVG import limited, consider another importer.\n(Error: " + str(e) + ")")
			if od :
				if o.translation :
					pos = od.properties["obj_pos"].value
					#FIXME:  looking at scascale.py this isn't completely correct
					x1 = pos.x + o.translation[0]
					y1 = pos.y + o.translation[1]
					od.move(x1, y1)
				layer.add_object(od)
		# create an 'Unhandled' layer and dump our Unknown
		# create an 'Errors' layer and dump our errors
		if len(self.errors.keys()) > 0 :
			layer = data.add_layer("Errors")
			s = "To hide the error messages delete or disable the 'Errors' layer\n"
			for e in self.errors.keys() :
				s = s + str(e) + " -> " + str(self.errors[e]) + "\n"
		
			o = Text()
			o.props["fill"] = "red"
			o.Set(s)
			layer.add_object(o.Create())
		# create a 'Description' layer 
		data.update_extents ()
		return 1
	def Dump(self) :
		for o in self.objects :
			o.Dump(0)
		for e in self.errors.keys() :
			print e, "->", self.errors[e]

def Test() :
	import sys
	imp = Importer()
	sName = sys.argv[1]
	if sName[-1] == "z" :
		import gzip
		f = gzip.open(sName)
	else :
		f = open(sName)
	imp.Parse(f.read())
	if len(sys.argv) > 2 :
		sys.stdout = open(sys.argv[2], "wb")
	imp.Dump()
	sys.exit(0)

if __name__ == '__main__': Test()

def import_svg(sFile, diagramData) :
	imp = Importer()
	f = open(sFile)
	imp.Parse(f.read())
	return imp.Render(diagramData)

def import_svgz(sFile, diagramData) :
	import gzip
	imp = Importer()
	f = gzip.open(sFile)
	imp.Parse(f.read())
	return imp.Render(diagramData)

import dia
dia.register_import("SVG plain", "svg", import_svg)
dia.register_import("SVG compressed", "svgz", import_svgz)

