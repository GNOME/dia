#
# Dissect a diagram by rendering it and accumulating invalid
# renderer calls to object selection.
#
# Copyright (c) 2014 Hans Breuer <hans@breuer.org>
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
# \brief A dissecting renderer for Dia
#
# Check the diagram by rendering it and report anomalies with their
# call and the causing object.
#
# \extends _DiaPyRenderer
# \ingroup ExportFilters
class DissectRenderer :
	def __init__ (self) :
		self.f = None
		self.current_objects = []
		self.warnings = []
		self.errors = []
		self.font = None
	def _open(self, filename) :
		self.f = open(filename, "w")
	def begin_render (self, data, filename) :
		self._open (filename)
		self.extents = data.extents
		try :
			# this can fail when data is not interative,
			# e.g. when running from pure bindings
			self.f.write ("# Dissect %s\n" % (data.filename,))
		except :
			self.f.write ("# Dissect %s\n" % (filename,))
	def end_render (self) :
		self.f.write('%d error(s) %d warning(s)\n' % (len(self.errors), len(self.warnings)))
		self.f.close ()
	def Warning (self, msg) :
		self.warnings.append ((self.current_objects[-1], msg))
		if self.f :
			self.f.write ("Warning: %s, %s\n" % (self.current_objects[-1], msg))
	def Error (self, msg) :
		self.errors.append ((self.current_objects[-1], msg))
		if self.f :
			self.f.write ("Error: %s, %s\n" % (self.current_objects[-1], msg))
	def draw_object (self, object, matrix) :
		self.current_objects.append (object)
		# XXX: check matrix
		# don't forget to render the object
		object.draw (self)
		del self.current_objects[-1]
	def set_linewidth (self, width) :
		# width==0 is hairline
		if width < 0 or width > 10 :
			self.Warning ("linewidth out of range")
	def set_linecaps (self, mode) :
		if mode < 0 or mode > 2 :
			self.Error ("linecaps '%d' unknown" % (mode,))
	def set_linejoin (self, mode) :
		if mode < 0 or mode > 2 :
			self.Error ("linejoin '%d' unknown" % (mode,))
	def set_linestyle (self, style, dash_length) :
		if style < 0 or style > 4 :
			self.Error ("linestyle '%d' unknown" % (style,))
		if dash_length < 0.001 or dash_length > 1 :
			self.Warning ("dashlength '%f' out of range" % (dash_length,))
	def set_fillstyle (self, style) :
		# currently only 'solid' so not used anywhere else
		if style != 0 :
			self.Error ("fillstyle '%d' unknown" % (style,))
	def set_font (self, font, size) :
		self.font = font
		self.font_size = size
	def draw_line (self, start, end, color) :
		pass # can anything go wrong here ?
	def draw_polyline (self, points, color) :
		if len(points) < 2 :
			self.Error ("draw_polyline with too few points")
	def _polygon (self, points, fun) :
		if len(points) < 3 :
			self.Error ("%s with too few points" % (fun,))
	def draw_polygon (self, points, fill, stroke) :
		self._polygon(points, "draw_polygon")
	# obsolete with recent Dia
	def fill_polygon (self, points, color) :
		self._polygon(points, "draw_polygon")
	def _rect (self, rect, fun) :
		if rect.top > rect.bottom :
			self.Warning ("%s negative height" % (fun,))
		if rect.left > rect.right :
			self.Warning ("%s negative width" % (fun,))
	def draw_rect (self, rect, fill, stroke) :
		self._rect (rect, "draw_rect")
	def draw_rounded_rect (self, rect, fill, stroke, rounding) :
		# XXX: check rounding to be positive (smaller than half width, height?)
		self._rect (rect, "draw_rect")
	def _arc (self, center, width, height, angle1, angle2, fun) :
		if width <= 0 :
			self.Warning ("%s width too small" % (fun,))
		if height <= 0 :
			self.Warning ("%s height too small" % (fun,))
		# angles
		rot = 0.0
		if angle1 < angle2 :
			rot = angle2 - angle1
		else :
			rot = angle1 - angle2
		if rot <= 0 or rot >= 360 :
			self.Warning ("%s bad rotation %g,%g" % (fun, angle1, angle2))
	def draw_arc (self, center, width, height, angle1, angle2, color) :
		self._arc(center, width, height, angle1, angle2, "draw_arc")
	def fill_arc (self, center, width, height, angle1, angle2, color) :
		self._arc(center, width, height, angle1, angle2, "fill_arc")
	def draw_ellipse (self, center, width, height, fill, stroke) :
		self._arc(center, width, height, 0, 360, "draw_ellipse")
	def _bezier (self, bezpoints, fun) :
		nMoves = 0
		for bp in bezpoints :
			if bp.type == 0 : # BEZ_MOVE_TO
				nMoves = nMoves + 1
				if nMoves > 1 :
					self.Warning ("%s move-to within", (fun,))
			elif bp.type == 1 : # BEZ_LINE_TO
				pass
			elif bp.type == 2 : # BEZ_CURVE_TO
				pass
			else :
				self.Error ("%s invalid BezPoint type='%d'" % (fun, bp.type,))
	def draw_bezier (self, bezpoints, color) :
		if len(bezpoints) < 2 :
			self.Error ("draw_bezier too few points");
		self._bezier (bezpoints, "draw_bezier")
	def fill_bezier (self, bezpoints, color) :
		if len(bezpoints) < 3 :
			self.Error ("fill_bezier too few points");
		self._bezier (bezpoints, "fill_bezier")
	def draw_string (self, text, pos, alignment, color) :
		if len(text) < 1 :
			self.Warning ("draw_string empty text")
		if alignment < 0 or alignment > 2 :
			self.Error ("draw_string unknown alignmern '%d'" % (alignment,))
	def draw_image (self, point, width, height, image) :
		if width <= 0 :
			self.Warning ("draw_image width too small")
		if height <= 0 :
			self.Warning ("draw_image height too small")
		# XXX: check image, e.g. existing file name
# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_export (_("Dissect"), "dissect", DissectRenderer())
