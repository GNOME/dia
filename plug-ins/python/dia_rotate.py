# PyDia Rotation 
# Copyright (c) 2003, Hans Breuer <hans@breuer.org>
# Copyright (c) 2009  Steffen Macke <sdteffen@sdteffen.de
#
#  This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

import dia, math, string

class CRotateDialog :
	def __init__(self, d, data) :
		import pygtk
		pygtk.require("2.0")
		import gtk
		win = gtk.Window()
		win.connect("delete_event", self.on_delete)
		win.set_title("Rotation")

		self.diagram = d
		self.data = data
		self.win = win

		box1 = gtk.VBox()
		win.add(box1)
		box1.show()

		box2 = gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()

		self.entry = gtk.Entry()
		self.entry.set_text("0.0")
		box2.pack_start(self.entry)
		self.entry.show()

		separator = gtk.HSeparator()
		box1.pack_start(separator, expand=0)
		separator.show()

		box2 = gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2, expand=0)
		box2.show()

		button = gtk.Button("rotate")
		button.connect("clicked", self.on_rotate)
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
		win.show()

	def on_rotate(self, *args) :
		s = self.entry.get_text()
		angle = float(s)
		if angle >= 0 and angle <= 360 :
			SimpleRotate (self.data, angle)
			self.data.update_extents ()
			self.diagram.flush()
		else :
			dia.message(1, "Value out of range!")
		self.win.destroy ()

	def on_delete (self, *args) :
		self.win.destroy ()

def SimpleRotate(data, angle) :
	# Rotation center
	xm = 0.0
	ym = 0.0

	# Convert to radians
	angle_rad = 2*math.pi - 2*math.pi*angle/360

	objs = data.get_sorted_selected()
	if len(objs) == 0 :
		objs = data.active_layer.objects
	scaleFailed = {}
	for o in objs :
		for h in o.handles:
			x = math.cos(angle_rad)*(h.pos.x+xm)-math.sin(angle_rad)*(h.pos.y+ym)
			y = math.sin(angle_rad)*(h.pos.x+xm)+math.cos(angle_rad)*(h.pos.y)
			o.move_handle(h, (x,y), 0, 0)
					
	data.update_extents ()
	dia.active_display().add_update_all()

def rotate_cb(data, flags) :
	dlg = CRotateDialog(dia.active_display().diagram, data)

dia.register_action ("ObjectsSimplerotation", "Simple Rotation",
		     "/DisplayMenu/Objects/ObjectsExtensionStart", 
		     rotate_cb)
