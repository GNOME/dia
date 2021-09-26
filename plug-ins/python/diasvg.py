#  PyDia SVG Renderer
#  Copyright (c) 2003, 2004 Hans Breuer <hans@breuer.org>
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

import sys, string, dia

import gettext
_ = gettext.gettext

##
# \brief The second SvgRenderer implementation for Dia
#
# A full blown SVG(Z) renderer. As of the initial writing less bugs in the output
# than the Dia SVG renderer written in C. Nowadays the _SvgRenderer is on par,
# but this one is still easier to extend and experiment with.
#
# \extends _DiaPyRenderer
# \ingroup ExportFilters
class SvgRenderer :
	def __init__ (self) :
		self.f = None
		self.line_width = 0.1
		self.line_caps = 0
		self.line_join = 0
		self.line_style = 0
		self.dash_length = 0
	def _open(self, filename) :
		self.f = open(filename, "w")
	def begin_render (self, data, filename) :
		self._open (filename)
		r = data.extents
		xofs = - r[0]
		yofs = - r[1]
		self.f.write('''<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!-- Created by diasvg.py -->
<svg width="%.3fcm" height="%.3fcm" viewBox="%.3f %.3f %.3f %.3f"
 xmlns="http://www.w3.org/2000/svg"
 xmlns:xlink="http://www.w3.org/1999/xlink">
''' % (r.right - r.left, r.bottom - r.top, r[0], r[1], r[2] - r[0], r[3] - r[1]))
		#self.f.write("<!-- %s -->\n" % (str(data.extents)))
		#self.f.write("<!-- %s -->\n" % (data.active_layer.name))
	def end_render (self) :
		self.f.write('</svg>')
		self.f.close()
	def is_capable_to (self, cap) :
		if cap == 1 : # RENDER_HOLES
			return 1
		elif cap == 2 : # RENDER_ALPHA
			return 1
		elif cap == 4 : #  RENDER_AFFINE
			return 1
		elif cap == 8 : # RENDER_PATTERN
			return 0
		return 0
	def draw_layer (self, layer, active, update) :
		self.f.write ("<!-- Layer: " + layer.name + " -->\n")
		self.f.write ('<g id="' + self._escape (layer.name) + '">\n')
		layer.render (self)
		self.f.write ('</g>\n')
	def draw_object (self, object, matrix) :
		self.f.write("<!-- " + object.type.name + " -->\n")
		odict = object.properties["meta"].value
		if "url" in odict :
			self.f.write('<a xlink:href="' + self._escape(odict["url"]) + '">\n')
		if "id" in odict or matrix :
			attrs = ''
			if matrix :
				attrs += 'transform="matrix' + str(matrix) + '" '
			if "id" in odict :
				attrs += 'id="' + self._escape(odict['id']) + '"'
			self.f.write('<g ' + attrs + '>\n')
		# don't forget to render the object
		object.draw (self)
		if "id" in odict or matrix :
			self.f.write('</g>\n')
		if "url" in odict :
			self.f.write('</a>\n')
	def set_linewidth (self, width) :
		if width < 0.001 : # zero line width is invisible ?
			self.line_width = 0.001
		else :
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
	def draw_line (self, start, end, color) :
		self.f.write('<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" stroke="%s" stroke-width="%.3f" %s/>\n' \
					% (start.x, start.y, end.x, end.y, self._rgb(color), self.line_width, self._stroke_style()))
	def draw_polyline (self, points, color) :
		self.f.write('<polyline fill="none" stroke="%s" stroke-width="%.3f" %s points="' \
					% (self._rgb(color), self.line_width, self._stroke_style()))
		for pt in points :
			self.f.write ('%.3f,%.3f ' % (pt.x, pt.y))
		self.f.write('"/>\n')
	def draw_polygon (self, points, fill, stroke) :
		self.f.write('<polygon fill="%s" stroke="%s" stroke-width="%.3f" %s points="' \
					% (self._rgb(fill), self._rgb(stroke), self.line_width, self._stroke_style()))
		for pt in points :
			self.f.write ('%.3f,%.3f ' % (pt.x, pt.y))
		self.f.write('"/>\n')
	def draw_rect (self, rect, fill, stroke) :
		self.f.write('<rect x="%.3f" y="%.3f" width="%.3f" height="%.3f" fill="%s" stroke="%s" stroke-width="%.3f" %s/>\n' \
					% (	rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
				  		self._rgb(fill), self._rgb(stroke), self.line_width, self._stroke_style()))
	def draw_rounded_rect (self, rect, fill, stroke, rounding) :
		self.f.write('<rect x="%.3f" y="%.3f" width="%.3f" height="%.3f" fill="%s" stroke="%s" stroke-width="%.3f" %s rx="%.3f" />\n' \
					% (	rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
				  		self._rgb(fill), self._rgb(stroke), self.line_width, self._stroke_style(), rounding))
	def _arc (self, center, width, height, angle1, angle2, color, fill=None) :
		# not in the renderer interface
		import math
		mPi180 = math.pi / 180.0
		rx = width / 2.0
		ry = height / 2.0
		sx = center.x + rx * math.cos(mPi180 * angle1)
		sy = center.y - ry * math.sin(mPi180 * angle1)
		ex = center.x + rx * math.cos(mPi180 * angle2)
		ey = center.y - ry * math.sin(mPi180 * angle2)
		largearc = abs(angle2 - angle1) >= 180
		sweep = 0 # 0: draw in negative direction
		if angle1 > angle2 :
			sweep = 1
		if not fill :
			self.f.write('<path stroke="%s" fill="none" stroke-width="%.3f" %s' \
				% (self._rgb(color), self.line_width, self._stroke_style()))
		else :
			self.f.write('<path stroke="none" fill="%s"' % (self._rgb(color)))
		# moveto sx,sy arc rx,ry x-axis-rotation large-arc-flag,sweep-flag ex,ey
		self.f.write(' d ="M %.3f,%.3f A %.3f,%.3f 0 %d,%d %.3f,%.3f ' % (sx, sy, rx, ry, largearc, sweep, ex, ey))
		self.f.write('"/>\n')
	def draw_arc (self, center, width, height, angle1, angle2, color) :
		self._arc(center, width, height, angle1, angle2, color)
	def fill_arc (self, center, width, height, angle1, angle2, color) :
		self._arc(center, width, height, angle1, angle2, color, 1)
	def draw_ellipse (self, center, width, height, fill, stroke) :
		self.f.write('<ellipse cx="%.3f" cy="%.3f" rx="%.3f" ry="%.3f" ' \
				'fill="%s" stroke="%s"  stroke-width="%.3f" %s/>\n' \
				% (center.x, center.y, width / 2, height / 2,
				   self._rgb(fill), self._rgb(stroke),
				   self.line_width, self._stroke_style()))
	def _bezier (self, bezpoints) :
		for bp in bezpoints :
			if bp.type == 0 : # BEZ_MOVE_TO
				self.f.write('M %.3f,%.3f ' % (bp.p1.x, bp.p1.y))
			elif bp.type == 1 : # BEZ_LINE_TO
				self.f.write('L %.3f,%.3f ' % (bp.p1.x, bp.p1.y))
			elif bp.type == 2 : # BEZ_CURVE_TO
				self.f.write ('C %.3f,%.3f %.3f,%.3f %.3f,%.3f ' \
					% (bp.p1.x, bp.p1.y, bp.p2.x, bp.p2.y, bp.p3.x, bp.p3.y))
			else :
				dia.message(2, "Invalid BezPoint type (%d)" * bp.type)
	def draw_bezier (self, bezpoints, color) :
		self.f.write('<path stroke="%s" fill="none" stroke-width="%.3f" %s d="' \
					% (self._rgb(color), self.line_width, self._stroke_style()))
		self._bezier (bezpoints)
		self.f.write('"/>\n')
	# old version, wont be called anymore with newer Dia
	def fill_bezier (self, bezpoints, color) :
		self.f.write('<path stroke="none" fill="%s" stroke-width="%.3f" d="' \
					% (self._rgb(color), self.line_width))
		self._bezier (bezpoints)
		self.f.write('"/>\n')
	def draw_beziergon (self, bezpoints, fill, stroke) :
		self.f.write('<path stroke="%s" fill="%s" stroke-width="%.3f" %s d="' \
				% (self._rgb(stroke), self._rgb(fill), self.line_width, self._stroke_style()))
		self._bezier (bezpoints)
		self.f.write('z"/>\n')
	def draw_string (self, text, pos, alignment, color) :
		if len(text) < 1 :
			return # shouldn'this be done at the higher level
		talign = ('start', 'middle', 'end') [alignment]
		fstyle = ('normal', 'italic', 'oblique') [self.font.style & 0x03]
		fweight = (400, 200, 300, 500, 600, 700, 800, 900) [(self.font.style  >> 4)  & 0x7]
		self.f.write('<text x="%.3f" y="%.3f"  fill="%s" text-anchor="%s" font-size="%.2f" font-family="%s" font-style="%s" font-weight="%d">\n' \
			% (pos.x, pos.y, self._rgb(color), talign, self.font_size, self.font.family, fstyle,  fweight))
		self.f.write(self._escape(text))
		self.f.write('</text>\n')
	def draw_image (self, point, width, height, image) :
		#FIXME : do something better than absolute pathes ?
		self.f.write('<image x="%.3f" y="%.3f"  width="%.3f" height="%.3f" xlink:href="%s"/>\n' \
			% (point.x, point.y, width, height, image.uri))
	# Helpers, not in the DiaRenderer interface
	def _escape (self, text) :
		# avoid writing XML special characters (ampersand must be first to not break the rest)
		for rep in [('&', '&amp;'), ('<', '&lt;'), ('>', '&gt;'), ('"', '&quot;'), ("'", '&apos;')] :
			text = text.replace (rep[0], rep[1])
		return text
	def _rgb(self, color) :
		# given a dia color convert to svg color string
		if not color :
			return "none"
		rgb = "#%02X%02X%02X" % (int(255 * color.red), int(color.green * 255), int(color.blue * 255))
		return rgb
	def _stroke_style(self) :
		# return the current line style as svg string
		dashlen =self.dash_length
		# dashlen/style interpretation like the DiaGdkRenderer
		dotlen = dashlen * 0.1
		caps = self.line_caps
		join = self.line_join
		style = self.line_style
		st = ""
		if style == 0 : # DIA_LINE_STYLE_SOLID
			pass
		elif style == 1 : # DASHED
			st = 'stroke-dasharray="%.2f,%.2f"' % (dashlen, dashlen)
		elif style == 2 : # DASH_DOT,
			gaplen = (dashlen - dotlen) / 2.0
			st = 'stroke-dasharray="%.2f,%.2f,%.2f,%.2f"' % (dashlen, gaplen, dotlen, gaplen)
		elif style == 3 : # DASH_DOT_DOT,
			gaplen = (dashlen - dotlen) / 3.0
			st = 'stroke-dasharray="%.2f,%.2f,%.2f,%.2f,%.2f,%.2f"' % (dashlen, gaplen, dotlen, gaplen, dotlen, gaplen)
		elif style == 4 : # DOTTED
			st = 'stroke-dasharray="%.2f,%.2f"' % (dotlen, dotlen)

		if join == 0 : # MITER
			pass # st = st + ' stroke-linejoin="bevel"'
		elif join == 1 : # ROUND
			st = st + ' stroke-linejoin="round"'
		elif join == 2 : # BEVEL
			st = st + ' stroke-linejoin="bevel"'

		if caps == 0 : # BUTT
			pass # default stroke-linecap="butt"
		elif caps == 1 : # ROUND
			st = st + ' stroke-linecap="round"'
		elif caps == 2 : # PROJECTING
			st = st + ' stroke-linecap="square"' # is this the same ?

		return st

class SvgzRenderer(SvgRenderer) :
	def _open(self, filename) :
		# There is some (here) not wanted behaviour in gzip.open/GzipFile :
		# the filename with path is not only used to adress the file but also
		# completely stored in the file itself. Correct it here.
		import os, os.path, gzip
		path, name = os.path.split(filename)
		os.chdir(path)
		self.f = gzip.open (name, "wb")

# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_export (_("SVG plain"), "svg", SvgRenderer())
dia.register_export (_("SVG compressed"), "svgz", SvgzRenderer())
