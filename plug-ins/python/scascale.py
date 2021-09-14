# PyDia Simple Scale
# Copyright (c) 2003, Hans Breuer <hans@breuer.org>
#
# Experimental scaleing (selected) Objects via property api
#
# Known Issues:
#  - HANDLE_NON_MOVEABLE ?
#  - bezier control points
#  - unsizeable objects (or sizeable via multiple text size changing, e.g. UML Class)
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
import dia

import gettext
_ = gettext.gettext

class CScaleDialog :
	def __init__(self, data) :
		import gi

		gi.require_version('Gtk', '3.0')

		with warnings.catch_warnings():
			warnings.filterwarnings("ignore", category=RuntimeWarning)
			from gi.repository import Gtk

		win = Gtk.Window()
		win.connect("delete_event", self.on_delete)
		win.set_title(_("Simple Scaling"))

		self.diagram = data
		self.win = win

		box1 = Gtk.VBox()
		win.add(box1)
		box1.show()

		box2 = Gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2, True, True, 0)
		box2.show()

		self.entry = Gtk.Entry()
		self.entry.set_text("0.1")
		box2.pack_start(self.entry, True, True, 0)
		self.entry.show()

		separator = Gtk.HSeparator()
		box1.pack_start(separator, 0, True, 0)
		separator.show()

		box2 = Gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2, 0, True, 0)
		box2.show()

		button = Gtk.Button(_("Scale"))
		button.connect("clicked", self.on_scale)
		box2.pack_start(button, True, True, 0)
		button.set_can_default(True)
		button.grab_default()
		button.show()
		win.show()

	def on_scale(self, *args) :
		s = self.entry.get_text()
		scale = float(s)
		if scale > 0.001 and scale < 1000 :
			SimpleScale (self.diagram, float(s))
			self.diagram.update_extents ()
			self.diagram.flush()
		else :
			dia.message(1, "Value out of range!")
		self.win.destroy ()

	def on_delete (self, *args) :
		self.win.destroy ()

def ScaleLens(o, factor) :
	if "line_width" in o.properties :
		o.properties["line_width"] = o.properties["line_width"].value * factor
	if "text_height" in o.properties :
		o.properties["text_height"] = o.properties["text_height"].value * factor

def SimpleScale(data, factor) :
	objs = data.get_sorted_selected()
	if len(objs) == 0 :
		objs = data.active_layer.objects
	scaleFailed = {}
	for o in objs :
		pos = o.properties["obj_pos"].value
		hSE = None # the 'south east' handle to size the object
		if "elem_width" in o.properties :
			hLR = o.handles[7] # HANDLE_RESIZE_SE
			try :
				#if 0 == hLR.type : # HANDLE_NON_MOVABLE
				#	raise RuntimeError, "non moveable handle"
				x = pos.x * factor
				y = pos.y * factor
				x2 = x + (hLR.pos.x - pos.x) * factor
				y2 = y + (hLR.pos.y - pos.y) * factor
				# calculate all points before movement, handle is still connected and move moves it too
				o.move(x, y)
				o.move_handle(hLR, (x2, y2), 0, 0)
				ScaleLens(o, factor)
			except RuntimeError as msg :
				if o.type.name in scaleFailed :
					scaleFailed[o.type.name] += 1
				else :
					scaleFailed[o.type.name] = 1
		else :
			# must move all handles
			try :
				x = pos.x * factor
				y = pos.y * factor
				handles = []
				for h in o.handles :
					#if 0 == h.type : # HANDLE_NON_MOVABLE
					#	continue
					handles.append((h, (x + (h.pos.x - pos.x) * factor,
									  y + (h.pos.y - pos.y) * factor)))
				# handles are not necessary independent
				for h in handles :
					o.move_handle(h[0], h[1],  0, 0)
				ScaleLens(o, factor)
			except RuntimeError as msg :
				if o.type.name in scaleFailed :
					scaleFailed[o.type.name] += 1
				else :
					scaleFailed[o.type.name] = 1
	if len(list(scaleFailed.keys())) > 0 :
		sMsg = "Scaling failed for : "
		for s in list(scaleFailed.keys()) :
			sMsg = sMsg + "\n%s (%d)" % (s, scaleFailed[s])
		dia.message(1, sMsg)
	data.update_extents ()
	dia.active_display().add_update_all()

def scale_cb(data, flags) :
	dlg = CScaleDialog(data)

dia.register_action ("ObjectsSimplescaling", _("Simple _Scaling"),
		     "/DisplayMenu/Objects/ObjectsExtensionStart",
		     scale_cb)
