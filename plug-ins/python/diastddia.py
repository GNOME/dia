#  PyDia Standard Dia Renderer
#  Copyright (c) 2009 Hans Breuer <hans@breuer.org>
#
#  A renderer which outputs Dia "Standard - *" Objects in Dia XML
#  by implementing the renderer interface.

#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

import sys, string, os, dia

class StandardDiaRenderer :
	def __init__ (self) :
		self.f = None
		self.line_width = 0.1
		self.line_caps = 0
		self.line_join = 0
		self.line_style = 0
		self.dash_length = 0
		# every object should have a unique id - mainly for connections definition
		self.oid = 0
		self.filename = ""
	def _open(self, filename) :
		self.f = open(filename, "w")
	def begin_render (self, data, filename) :
		self.filename = filename
		self._open (filename)
		self.f.write('''<?xml version="1.0" encoding="UTF-8"?>
<dia:diagram xmlns:dia="http://www.lysator.liu.se/~alla/dia/">
  <!-- Created by dia_standard_dia.py -->
  <dia:diagramdata>
    <!-- almost everything is optional here -->
  </dia:diagramdata>
  <dia:layer name="Background" visible="true" >
''')
	def end_render (self) :
		self.f.write('''<!-- no connections in the renderer interface -->
  </dia:layer>
</dia:diagram>''')
		self.f.close()
	# some helpers
	def _rgb(self, color) :
		# given a dia color convert to svg color string
		rgb = "#%02X%02X%02X" % (int(255 * color.red), int(color.green * 255), int(color.blue * 255))
		return rgb
	#FIXME: http://bugzilla.gnome.org/show_bug.cgi?id=576468 makes this evem less desirable
	def _tinting (self, color, fill=None) :
		# maybe this is too smart: fill=0 != fill=None
		if fill == None :
			attrs = '''
      <dia:attribute name="line_color">
        <dia:color val="%s"/>
      </dia:attribute>
      <dia:attribute name="line_width">
        <dia:real val="%.3f"/>
      </dia:attribute>
'''  % (self._rgb(color), self.line_width)
		elif fill :
			attrs = '''
      <dia:attribute name="border_color">
        <dia:color val="%s"/>
      </dia:attribute>
      <dia:attribute name="show_background">
        <dia:boolean val="true"/>
      </dia:attribute>
      <!-- FIXME: folding missing -->
      <dia:attribute name="inner_color">
        <dia:color val="%s"/>
      </dia:attribute>
      <dia:attribute name="border_width">
        <dia:real val="0"/>
      </dia:attribute>
''' % (self._rgb(color), self._rgb(color))
		else :
			attrs = '''
      <dia:attribute name="border_color">
        <dia:color val="%s"/>
      </dia:attribute>
      <dia:attribute name="show_background">
        <dia:boolean val="false"/>
      </dia:attribute>
      <dia:attribute name="border_width">
        <dia:real val="%.3f"/>
      </dia:attribute>
''' % (self._rgb(color), self.line_width)
		return attrs
	def _stroke_style (self) :
		return '''
      <dia:attribute name="line_style">
        <dia:enum val="%d"/>
      </dia:attribute>
      <dia:attribute name="dashlength">
        <dia:real val="%.3f"/>
      </dia:attribute>
''' % (self.line_style, self.dash_length)
	# just remember the given state to use it during object creation
	def set_linewidth (self, width) :
		self.line_width = width
	def set_linecaps (self, mode) :
		self.line_caps = mode
	def set_linejoin (self, mode) :
		self.line_join = mode
	def set_linestyle (self, style) :
		self.line_style = style
	def set_dashlength (self, length) :
		self.dash_length = length
	def set_fillstyle (self, style) :
		# currently only 'solid' so not used anywhere else
		self.fill_style = style
	def set_font (self, font, size) :
		self.font = font
		self.font_size = size
	# now to the actual object creation - it should be 'delayed' to fold draw&fill again, 
	# split in the rendering process but the Dia's 'Standard' objects can do both in one
	def _box (self, rect, color, fill) :
		self.oid = self.oid + 1
		self.f.write('''
    <dia:object type="Standard - Box" version="0" id="O%d">
      <dia:attribute name="elem_corner">
        <dia:point val="%.3f,%.3f"/>
      </dia:attribute>
      <dia:attribute name="elem_width">
        <dia:real val="%.3f"/>
      </dia:attribute>
      <dia:attribute name="elem_height">
        <dia:real val="%.3f"/>
      </dia:attribute>
%s%s
    </dia:object>''' % (self.oid, rect.left,rect.top, rect.right - rect.left, rect.bottom - rect.top, 
			self._tinting(color, fill), self._stroke_style()))
	def draw_rect (self, rect, color) :
		self._box(rect, color, 0)
	def fill_rect (self, rect, color) :
		self._box(rect, color, 1)

	def _ellipse (self, center, width, height, color, fill) :
		self.oid = self.oid + 1
		self.f.write('''
    <dia:object type="Standard - Ellipse" version="0" id="O%d">
      <dia:attribute name="elem_corner">
        <dia:point val="%.3f,%.3f"/>
      </dia:attribute>
      <dia:attribute name="elem_width">
        <dia:real val="%.3f"/>
      </dia:attribute>
      <dia:attribute name="elem_height">
        <dia:real val="%.3f"/>
      </dia:attribute>
%s%s
    </dia:object>''' % (self.oid, center.x-width/2, center.y-height/2, width, height, 
			self._tinting(color, fill), self._stroke_style()))
	def draw_ellipse (self, center, width, height, color) :
		self._ellipse(center, width, height, color, 0)
	def fill_ellipse (self, center, width, height, color) :
		self._ellipse(center, width, height, color, 0)

	def _arc(self, center, width, height, angle1, angle2, color, fill) :
		pass #FIXME
	def draw_arc (self, center, width, height, angle1, angle2, color) :
		self._arc(center, width, height, angle1, angle2, color, 0)
	def fill_arc (self, center, width, height, angle1, angle2, color) :
		self._arc(center, width, height, angle1, angle2, color, 1)

	def _poly (self, type_name, points, color, fill) :
		self.oid = self.oid + 1
		self.f.write('''
    <dia:object type="Standard - %s" version="0" id="O%d">
%s%s
      <dia:attribute name="poly_points">''' % (type_name, self.oid, self._tinting(color, fill), self._stroke_style()))
		for p in points :
			self.f.write('''
        <dia:point val="%.3f,%.3f"/>''' % (p.x, p.y))
		self.f.write('''
      </dia:attribute>
    </dia:object>''')
	def draw_polyline (self, points, color) :
		self._poly("PolyLine", points, color, None)
	def draw_polygon (self, points, color) :
		self._poly("Polygon", points, color, 0)
	def fill_polygon (self, points, color) :
		self._poly("Polygon", points, color, 1)

	def _bezier (self, type_name, points, color, fill) :
		self.oid = self.oid + 1
		self.f.write('''
    <dia:object type="Standard - %s" version="0" id="O%d">
%s%s
      <dia:attribute name="bez_points">''' % (type_name, self.oid, self._tinting(color, fill), self._stroke_style()))
		# first point is just move
		for bp in points :
			if bp.type == 0 : # BEZ_MOVE_TO
				#FIXME: moveto must be first
				self.f.write('''
        <dia:point val="%.3f,%.3f"/>''' % (bp.p1.x, bp.p1.y))
			elif bp.type == 1 : # BEZ_LINE_TO
				# simulated by curveto
				self.f.write('''
        <dia:point val="%.3f,%.3f"/>
        <dia:point val="%.3f,%.3f"/>
        <dia:point val="%.3f,%.3f"/>''' % (last.x, last.y, bp.p1.x, bp.p1.y, bp.p1.x, bp.p1.y))
			elif bp.type == 2 : # BEZ_CURVE_TO
				self.f.write('''
        <dia:point val="%.3f,%.3f"/>
        <dia:point val="%.3f,%.3f"/>
        <dia:point val="%.3f,%.3f"/>''' % (bp.p1.x, bp.p1.y, bp.p2.x, bp.p2.y, bp.p3.x, bp.p3.y))
			last = bp.p1
		self.f.write('''
      </dia:attribute>
    </dia:object>''')
	def draw_bezier (self, points, color) :
		self._bezier("BezierLine", points, color, None)
	def fill_bezier (self, points, color) :
		self._bezier("Beziergon", points, color, 1)

	def draw_string (self, text, pos, alignment, color) :
		pass #FIXME

	def draw_image (self, point, width, height, image) :
		fname = image.filename
		# do something better than absolute pathes
		common = os.path.commonprefix ([fname, self.filename])
		if len(common) > 0 and string.find(self.filename[len(common):], os.path.pathsep) == -1 :
			fname = fname[len(common):]
		self.oid = self.oid + 1
		if abs(width/height - image.width/image.height) < 0.001 :
			keep_aspect = "true"
		else :
			keep_aspect = "false"
		self.f.write('''
    <dia:object type="Standard - Image" version="0" id="O%d">
      <dia:attribute name="elem_corner">
        <dia:point val="%.3f,%.3f"/>
      </dia:attribute>
      <dia:attribute name="elem_width">
        <dia:real val="%.3f"/>
      </dia:attribute>
      <dia:attribute name="elem_height">
        <dia:real val="%.3f"/>
      </dia:attribute>
      <dia:attribute name="file">
        <dia:string>#%s#</dia:string>
      </dia:attribute>
      <dia:attribute name="draw_border">
        <dia:boolean val="false"/>
      </dia:attribute>
      <dia:attribute name="keep_aspect">
        <dia:boolean val="%s"/>
      </dia:attribute>
    </dia:object>''' % (self.oid, point.x,point.y, width, height, fname, keep_aspect))

	# defining only this line drawer makes almost everything interpolated
	def draw_line (self, start, end, color) :
		self.oid = self.oid + 1
		self.f.write('''
    <dia:object type="Standard - Line" version="0" id="O%d">
      <dia:attribute name="conn_endpoints">
        <dia:point val="%.3f,%.3f"/>
        <dia:point val="%.3f,%.3f"/>
      </dia:attribute>
%s%s
    </dia:object>''' % (self.oid, start.x,start.y, end.x, end.y, 
			self._tinting(color), self._stroke_style()))
		
# register the renderer
dia.register_export ("Dia plain", "dia", StandardDiaRenderer())

