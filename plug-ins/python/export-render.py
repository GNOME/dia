import sys, dia

# sys.path.insert(0, 'd:/graph/dia/dia')
sys.argv = [ 'dia-python' ]

class DumpRenderer :
	def __init__ (self) :
		pass
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
	def end_render (self) :
		self.f.close()
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
		self.fill_style = style
	def set_font (self, font, size) :
		self.font = font
	def draw_line (self, start, end, color) :
		self.f.write("draw_line:" + str(start) + str(end) + str(color) + "\n")
	def draw_polyline (self, points, color) :
		self.f.write("draw_polyline: " + str(color) + "\n")
		for pt in points :
			self.f.write ("\t" + str(pt) + "\n")
	def draw_polygon (self, points, color) :
		self.f.write("draw_polygon: " + str(color) + "\n")
		for pt in points :
			self.f.write ("\t" + str(pt) + "\n")
	def fill_polygon (self, points, color) :
		self.f.write("fill_polygon: " + str(color) + "\n")
		for pt in points :
			self.f.write ("\t" + str(pt) + "\n")
	def draw_rect (self, rect, color) :
		self.f.write("draw_rect: " + str(rect) + str(color) + "\n")
	def fill_rect (self, rect, color) :
		self.f.write("fill_rect: " + str(rect) + str(color) + "\n")
	def draw_arc (self, center, width, height, angle1, angle2, color) :
		self.f.write("draw_arc: " + str(center) + ";" \
				+ str(width) + "x" + str(height) + ";" \
				+ str(angle1) + "," + str(angle2) + ";" + str(color) + "\n")
	def fill_arc (self, center, width, height, angle1, angle2, color) :
		self.f.write("fill_arc: " + str(center) + ";" \
				+ str(width) + "x" + str(height) + ";" \
				+ str(angle1) + "," + str(angle2) + ";" + str(color) + "\n")
	def draw_ellipse (self, center, width, height, color) :
		self.f.write("draw_ellipse: " + str(center) \
				+ str(width) + "x" +str(height) + ";" + str(color) + "\n")
	def fill_ellipse (self, center, width, height, color) :
		self.f.write("fill_ellipse: " + str(center) \
				+ str(width) + "x" +str(height) + ";" + str(color) + "\n")
	def draw_bezier (self, bezpoints, color) :
		self.f.write("draw_bezier: " + str(color) + "\n")
		for pt in bezpoints :
			self.f.write ("\t" + str(pt) + "\n")
	def fill_bezier (self, bezpoints, color) :
		self.f.write("fill_bezier: " + str(color) + "\n")
		for pt in bezpoints :
			self.f.write ("\t" + str(pt) + "\n")
	def draw_string (self, text, pos, alignment, color) :
		self.f.write("draw_string: [" + text + "]; " + str(pos) \
				+ str(alignment) + "; " +str(color))
	def draw_image (self, point, width, height, image) :
		self.f.write("draw_image: " + str(point) + str(width) + "x" +str(height) \
				+ " " + image.filename + "\n")
		self.f.write("<rgb_data>" + image.rgb_data + "</rgb_data>")
		self.f.write("<mask_data>" + image.mask_data + "</mask_data>")

# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_export ("PyDia Render Export", "diapyr", DumpRenderer())
