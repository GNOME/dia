import sys, dia

import gettext
_ = gettext.gettext

# sys.path.insert(0, 'd:/graph/dia/dia')

##
# \brief A simple example renderer implemented in Python
# \extends _DiaPyRenderer
# \ingroup PyDia
class DumpRenderer :
	## \brief Constructor
	def __init__ (self) :
		pass
	## \brief Start rendering
	# For non-interactive renderers it is guaranteed that this function is
	# called before every drawing function. It should be used to clear the
	# state from previous use, because the renderer class gets only created
	# once per Dia session
	def begin_render (self, data, filename) :
		# DiagramData
		self.f = open(filename, "w")
		self.f.write("Data: " + str(data) + "\n")
		self.f.write("Extents: "+ str(data.extents) + "\n")
		self.f.write("active_layer: " + str(data.active_layer) + " " \
				+ data.active_layer.name + "\n")
		#self.f.write("grid .width: " + str(data.grid.width) \
		#			+ " .height" + str(data.grid.height) \
		#			+ "visible: " + str(data.visible) + "\n")
	## \brief End rendering
	# For non-interactive renderers it is guaranteed that all drawing
	# is finished with this call
	def end_render (self) :
		self.f.close()
	## \brief Remember the line width
	def set_linewidth (self, width) :
		self.line_width = width
	## \brief Remember the line caps
	def set_linecaps (self, mode) :
		self.line_caps = mode
	## \brief Remember the line join
	def set_linejoin (self, mode) :
		self.line_join = mode
	## \brief Remember the line style and dash length
	def set_linestyle (self, style, length) :
		self.line_style = style
		self.dash_length = length
	## \brief Remember the fill style
	def set_fillstyle (self, style) :
		self.fill_style = style
	## \brief Remember the font
	def set_font (self, font, size) :
		self.font = font
	## \brief Draw a straight line from start to end with color
	# @param start A point in diagram coordinate system
	# @param end A point in diagram coordinate system
	# @param color The color to use for the line
	# The addtional paramters needed for the drawing are given
	# by one of the set_*() function above
	def draw_line (self, start, end, color) :
		self.f.write("draw_line:" + str(start) + str(end) + str(color) + "\n")
	## \brief Draw a polyline
	# @param points An array of points in the diagram coordinate system
	# @param color The color to use for the line
	def draw_polyline (self, points, color) :
		self.f.write("draw_polyline: " + str(color) + "\n")
		for pt in points :
			self.f.write ("\t" + str(pt) + "\n")
	## \brief Draw a polygon
	# @param points An array of points in the diagram coordinate system
	# @param fill The color to use for the interior
	# @param stroke The color to use for the line
	def draw_polygon (self, points, fill, stroke) :
		self.f.write("draw_polygon: " + str(fill) + str(stroke) + "\n")
		for pt in points :
			self.f.write ("\t" + str(pt) + "\n")
	## \brief Draw a rectangle
	def draw_rect (self, rect, color) :
		self.f.write("draw_rect: " + str(rect) + str(fill) + str(stroke) + "\n")
	## \brief Draw an arc
	def draw_arc (self, center, width, height, angle1, angle2, color) :
		self.f.write("draw_arc: " + str(center) + ";" \
				+ str(width) + "x" + str(height) + ";" \
				+ str(angle1) + "," + str(angle2) + ";" + str(color) + "\n")
	## \brief Fill an arc
	def fill_arc (self, center, width, height, angle1, angle2, color) :
		self.f.write("fill_arc: " + str(center) + ";" \
				+ str(width) + "x" + str(height) + ";" \
				+ str(angle1) + "," + str(angle2) + ";" + str(color) + "\n")
	## \brief Draw an ellipse
	def draw_ellipse (self, center, width, height, fill, stroke) :
		self.f.write("draw_ellipse: " + str(center) \
				+ str(width) + "x" +str(height)
				+ ";" + str(fill) + ";" + str(stroke) + "\n")
	## \brief Draw a bezier line
	def draw_bezier (self, bezpoints, color) :
		self.f.write("draw_bezier: " + str(color) + "\n")
		for pt in bezpoints :
			self.f.write ("\t" + str(pt) + "\n")
	## \brief Fill a bezier shape
	def draw_beziergon (self, bezpoints, fill, stroke) :
		self.f.write("draw_bezier: " + str(fill) + "; " + str(stroke) + "\n")
		for pt in bezpoints :
			self.f.write ("\t" + str(pt) + "\n")
	## \brief Draw a string
	def draw_string (self, text, pos, alignment, color) :
		self.f.write("draw_string: [" + text + "]; " + str(pos) \
				+ str(alignment) + "; " +str(color))
	## \brief Draw an image
	def draw_image (self, point, width, height, image) :
		self.f.write("draw_image: " + str(point) + str(width) + "x" +str(height) \
				+ " " + image.filename + "\n")
		self.f.write("<rgb_data>" + image.rgb_data + "</rgb_data>")
		self.f.write("<mask_data>" + image.mask_data + "</mask_data>")

## \brief Register the renderer with Dia's export system
# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_export (_("PyDia Render Export"), "diapyr", DumpRenderer())
