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

import sys, string, os, math, dia

import gettext
_ = gettext.gettext

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
		portrait = "false"
		if data.paper.is_portrait :
			portrait = "true"
		# data.paper doesn't provide margins yet, data.paper.width is _useable_ width
		self.f.write('''<?xml version="1.0" encoding="UTF-8"?>
<dia:diagram xmlns:dia="http://www.lysator.liu.se/~alla/dia/">
  <!-- Created by dia_standard_dia.py -->
  <dia:diagramdata>
    <!-- everything is optional here -->
    <dia:attribute name="paper">
      <dia:composite type="paper">
        <dia:attribute name="name">
          <dia:string>#%s#</dia:string>
        </dia:attribute>
        <dia:attribute name="is_portrait">
          <dia:boolean val="%s"/>
        </dia:attribute>
        <dia:attribute name="scaling">
          <dia:real val="%.3f"/>
        </dia:attribute>
      </dia:composite>
    </dia:attribute>
  </dia:diagramdata>
  <dia:layer name="Background" visible="true" >
''' % (data.paper.name, portrait, data.paper.scaling))
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
	# http://bugzilla.gnome.org/show_bug.cgi?id=576468 makes this even less desirable
	# see also: samples/Self/dia-standard-objects.dia for inconsistency mapping
	def _tinting (self, color, fill=None, stroke_key="line_color", width_key="line_width", fill_key="inner_color") :
		# maybe this is too smart: fill=0 != fill=None
		if fill == None :
			attrs = '''
      <dia:attribute name="%s">
        <dia:color val="%s"/>
      </dia:attribute>
      <dia:attribute name="%s">
        <dia:real val="%.3f"/>
      </dia:attribute>
'''  % (stroke_key, self._rgb(color), width_key, self.line_width)
		elif fill :
			attrs = '''
      <dia:attribute name="%s">
        <dia:color val="%s"/>
      </dia:attribute>
      <dia:attribute name="show_background">
        <dia:boolean val="true"/>
      </dia:attribute>
      <!-- TODO: folding missing -->
      <dia:attribute name="%s">
        <dia:color val="%s"/>
      </dia:attribute>
      <dia:attribute name="%s">
        <dia:real val="0"/>
      </dia:attribute>
''' % (fill_key, self._rgb(color), stroke_key, self._rgb(color), width_key)
		else :
			attrs = '''
      <dia:attribute name="%s">
        <dia:color val="%s"/>
      </dia:attribute>
      <dia:attribute name="show_background">
        <dia:boolean val="false"/>
      </dia:attribute>
      <dia:attribute name="%s">
        <dia:real val="%.3f"/>
      </dia:attribute>
''' % (stroke_key, self._rgb(color), width_key, self.line_width)
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
	def set_linestyle (self, style, length) :
		self.line_style = style
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
			self._tinting(color, fill, 'border_color', 'border_width'), self._stroke_style()))
	def draw_rect (self, rect, fill, stroke) :
		if fill :
			self._box(rect, fill, 1)
		if stroke :
			self._box(rect, stroke, 0)

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
			self._tinting(color, fill, 'border_color', 'border_width'), self._stroke_style()))
	def draw_ellipse (self, center, width, height, fill, stroke) :
		if fill :
			self._ellipse(center, width, height, fill, 1)
		if stroke :
			self._ellipse(center, width, height, stroke, 0)

	def _arc(self, center, width, height, angle1, angle2, color, fill) :
		self.oid = self.oid + 1
		mPi180 = math.pi / 180.0
		rx = width / 2.0
		ry = height / 2.0
		sx = center.x + rx * math.cos(mPi180 * angle1)
		sy = center.y - ry * math.sin(mPi180 * angle1)
		ex = center.x + rx * math.cos(mPi180 * angle2)
		ey = center.y - ry * math.sin(mPi180 * angle2)
		# the middle control point
		cx = center.x + rx * math.cos(mPi180 * (angle1 + angle2)/2.0)
		cy = center.y - ry * math.sin(mPi180 * (angle1 + angle2)/2.0)
		dx = (sx + ex) / 2.0 - cx
		dy = (sy + ey) / 2.0 - cy
		dist = math.sqrt(dx * dx + dy * dy)
		if angle1 > angle2 :
			dist = -dist
		self.f.write('''
    <dia:object type="Standard - Arc" version="0" id="O%d">
      <dia:attribute name="conn_endpoints">
        <dia:point val="%.3f,%.3f"/>
        <dia:point val="%.3f,%.3f"/>
      </dia:attribute>
      <dia:attribute name="curve_distance">
        <dia:real val="%.3f"/>
      </dia:attribute>
%s%s
    </dia:object>''' % (self.oid, sx, sy, ex, ey, dist,
			self._tinting(color, fill, 'arc_color'), self._stroke_style()))
	def draw_arc (self, center, width, height, angle1, angle2, color) :
		self._arc(center, width, height, angle1, angle2, color, None)
	def fill_arc (self, center, width, height, angle1, angle2, color) :
		self._arc(center, width, height, angle1, angle2, color, 1)

	def _poly (self, type_name, points, color, fill) :
		self.oid = self.oid + 1
		self.f.write('''
    <dia:object type="Standard - %s" version="0" id="O%d">
%s%s
      <dia:attribute name="poly_points">''' % (type_name, self.oid,
						self._tinting(color, fill), self._stroke_style()))
		for p in points :
			self.f.write('''
        <dia:point val="%.3f,%.3f"/>''' % (p.x, p.y))
		self.f.write('''
      </dia:attribute>
    </dia:object>''')
	def draw_polyline (self, points, color) :
		self._poly("PolyLine", points, color, None)
	def stroke_polygon (self, points, color) :
		self._poly("Polygon", points, color, 0)
	def fill_polygon (self, points, color) :
		self._poly("Polygon", points, color, 1)
	def draw_polygon (self, points, fill, stroke) :
		if fill :
			self.fill_polygon(points, fill)
		if stroke :
			self.stroke_polygon(points, fill)

	def _bezier (self, type_name, points, color, fill) :
		self.oid = self.oid + 1
		self.f.write('''
    <dia:object type="Standard - %s" version="0" id="O%d">
%s%s
      <dia:attribute name="bez_points">''' % (type_name, self.oid, self._tinting(color, fill), self._stroke_style()))
		for bp in points :
			if bp.type == 0 : # BEZ_MOVE_TO
				# moveto must be first (and only first) otherwise the file format is broken
				self.f.write('''
        <dia:point val="%.3f,%.3f"/>''' % (bp.p1.x, bp.p1.y))
			elif bp.type == 1 : # BEZ_LINE_TO
				# simulated by curveto (there is no lineto on the file format level)
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
		self.oid = self.oid + 1
		self.f.write('''
    <dia:object type="Standard - Text" version="1" id="O%d">
      <dia:attribute name="obj_pos">
        <dia:point val="%.3f,%.3f"/>
      </dia:attribute>
      <dia:attribute name="text">
        <dia:composite type="text">
          <dia:attribute name="string">
            <dia:string>#%s#</dia:string>
          </dia:attribute>
          <dia:attribute name="font">
            <dia:font family="%s" style="%d"/>
          </dia:attribute>
          <dia:attribute name="height">
            <dia:real val="%.3f"/>
          </dia:attribute>
          <dia:attribute name="color">
            <dia:color val="%s"/>
          </dia:attribute>
          <dia:attribute name="alignment">
            <dia:enum val="%d"/>
          </dia:attribute>
        </dia:composite>
      </dia:attribute>
      <dia:attribute name="valign">
        <dia:enum val="3"/>
      </dia:attribute>
      </dia:object>''' % (self.oid, pos.x, pos.y, text.replace("&", "&amp;").replace(">", "&gt;").replace("<", "&lt;"),
			self.font.family, self.font.style, self.font_size,
			self._rgb(color), alignment))
	def draw_image (self, point, width, height, image) :
		fname = image.filename
		# do something better than absolute pathes
		common = os.path.commonprefix ([fname, self.filename])
		if len(common) > 0 and self.filename[len(common):].find(os.path.pathsep) == -1 :
			fname = fname[len(common):]
		# ensure <broken> does not get through
		fname = fname.replace("&", "&amp;").replace(">", "&gt;").replace("<", "&lt;")
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
dia.register_export (_("Dia plain"), "dia", StandardDiaRenderer())

