# PyDia Rotation
# Copyright (c) 2003, Hans Breuer <hans@breuer.org>
# Copyright (c) 2009, 2011  Steffen Macke <sdteffen@sdteffen.de
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

import warnings
import dia, math

import gettext
_ = gettext.gettext

class CRotateDialog :
	def __init__(self, data) :
		import gi

		gi.require_version('Gtk', '3.0')

		with warnings.catch_warnings():
			warnings.filterwarnings("ignore", category=RuntimeWarning)
			from gi.repository import Gtk

		win = Gtk.Window()
		win.connect("delete_event", self.on_delete)
		win.set_title(_("Rotate counter-clockwise"))

		self.diagram = data
		self.win = win

		box1 = Gtk.VBox()
		win.add(box1)
		box1.show()

		box2 = Gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2, True, True, 0)
		box2.show()

		label1 = Gtk.Label()
		label1.set_text(_('Rotation around (0,0). Rotation angle in degrees:'))
		box2.pack_start(label1, True, True, 0)
		label1.show()

		self.entry = Gtk.Entry()
		self.entry.set_text("0.0")
		box2.pack_start(self.entry, True, True, 0)
		self.entry.show()

		separator = Gtk.HSeparator()
		box1.pack_start(separator, 0, True, 0)
		separator.show()

		box2 = Gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2, 0, True, 0)
		box2.show()

		button = Gtk.Button("rotate")
		button.connect("clicked", self.on_rotate)
		box2.pack_start(button, True, True, 0)
		button.set_can_default(True)
		button.grab_default()
		button.show()
		win.show()

	def on_rotate(self, *args) :
		s = self.entry.get_text()
		angle = float(s)
		if angle >= 0 and angle <= 360 :
			SimpleRotate (self.diagram, angle)
			self.diagram.update_extents()
			self.diagram.flush()
		else :
			dia.message(1, "Please enter an angle between 0 and 360 degrees.")
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
	ptype = dia.get_object_type('Standard - Polygon')
	for o in objs :
		if o.type.name == 'Standard - Box' :
			r = o.properties['obj_bb'].value
			p = ptype.create(0,0)
			p = p[0]
			p.properties['poly_points'] = [(r.left, r.top), (r.right, r.top), (r.right, r.bottom), (r.left, r.bottom)]
			p.properties['line_width'] = o.properties['line_width']
			p.properties['line_colour'] = o.properties['line_colour']
			p.properties['line_style'] = o.properties['line_style']
			p.properties['line_colour'] = o.properties['line_colour']
			p.properties['fill_colour'] = o.properties['fill_colour']
			p.properties['show_background'] = o.properties['show_background']
			data.active_layer.add_object(p)
			data.active_layer.remove_object(o)
			o = p
		for h in o.handles:
			x = math.cos(angle_rad)*(h.pos.x+xm)-math.sin(angle_rad)*(h.pos.y+ym)
			y = math.sin(angle_rad)*(h.pos.x+xm)+math.cos(angle_rad)*(h.pos.y)
			o.move_handle(h, (x,y), 0, 0)

	data.update_extents ()
	dia.active_display().add_update_all()

def rotate_cb(data, flags) :
	dlg = CRotateDialog(data)

dia.register_action ("ObjectsSimplerotation", _("Simple _Rotation"),
		     "/DisplayMenu/Objects/ObjectsExtensionStart",
		     rotate_cb)
