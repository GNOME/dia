#    Select Objects By Common Properties
#    Copyright (c) 2003, 2013 Hans Breuer <hans@breuer.org>
#
#        different methods (menu entries) to select all objects with given
#    property, that is :
#	- a find dialog to select by name
#	- menu entries for different color types. The common color is read
#	  from already selected objects
#	- a size range given by the current selection

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

import warnings
import dia

import gettext
_ = gettext.gettext

class CFindDialog :
	def __init__(self, data) :
		import gi

		gi.require_version('Gtk', '3.0')

		with warnings.catch_warnings():
			warnings.filterwarnings("ignore", category=RuntimeWarning)
			from gi.repository import Gtk

		win = Gtk.Window()
		win.connect("delete_event", self.on_delete)
		win.set_title(_("Select by name"))

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
		self.entry.set_text(_("Enter name"))
		box2.pack_start(self.entry, True, True, 0)
		self.entry.show()

		separator = Gtk.HSeparator()
		box1.pack_start(separator, 0, True, 0)
		separator.show()

		box2 = Gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2, 0, True, 0)
		box2.show()

		button = Gtk.Button(_("Find"))
		button.connect("clicked", self.on_find)
		box2.pack_start(button, True, True, 0)
		button.set_can_default(True)
		button.grab_default()
		button.show()
		win.show()

	def on_find(self, *args) :
		s = self.entry.get_text()
		select_by (self.diagram, "name", s)
		self.diagram.update_extents ()
		self.diagram.flush()

	def on_delete (self, *args) :
		self.win.destroy ()

def do_dialog(data):
	# let the dialog do it's work
	dlg = CFindDialog(data)

def select_by (diagram, name, value) :
	objs = diagram.active_layer.objects
	for o in objs :
		if name in o.properties :
			print("<==", o.properties[name].value)
			if value == None or o.properties[name].value == value :
				diagram.select (o)

def select_by_name_cb (data, flags):
	do_dialog (data)

def select_by_selected (data, name) :
	grp = data.get_sorted_selected()
	bFoundAny = 0
	for o in grp :
		if name in o.properties :
			select_by (data, name, o.properties[name].value)
			bFoundAny = 1
	if not bFoundAny :
		dia.message(0, "No selected object has the property '%s'." % name)
	data.update_extents ()
	data.flush()

def select_by_fill_color_cb (data, flags) :
	select_by_selected (data, "fill_colour")
def select_by_line_color_cb (data, flags) :
	select_by_selected (data, "line_colour")
def select_by_text_color_cb (data, flags) :
	select_by_selected (data, "text_colour")

def select_by_size_cb (data, flags) :
	""" From the diagrams selection derive the minimum and maximum object size.
	    Select every other object within this range.
	"""
	grp = data.get_sorted_selected()
	smin = 100000
	smax = 0
	for o in grp :
		w = o.bounding_box.right - o.bounding_box.left
		h = o.bounding_box.bottom - o.bounding_box.top
		s = w * h
		if s > smax :
			smax = s
		if s < smin :
			smin = s
	objs = data.active_layer.objects
	for o in objs :
		w = o.bounding_box.right - o.bounding_box.left
		h = o.bounding_box.bottom - o.bounding_box.top
		s = w * h
		if s >= smin and s <= smax :
			data.select(o)
	data.flush()


dia.register_action ("SelectByName", _("_Name"),
                       "/DisplayMenu/Select/SelectBy/SelectByExtensionStart",
                       select_by_name_cb)
dia.register_action ("SelectByFillcolor", _("_Fill Color"),
                       "/DisplayMenu/Select/SelectBy/SelectByExtensionStart",
                       select_by_fill_color_cb)
dia.register_action ("SelectByLinecolor", _("_Line Color"),
                       "/DisplayMenu/Select/SelectBy/SelectByExtensionStart",
                       select_by_line_color_cb)
dia.register_action ("SelectByTextcolor", _("_Text Color"),
                       "/DisplayMenu/Select/SelectBy/SelectByExtensionStart",
                       select_by_text_color_cb)
dia.register_action ("SelectBySize", _("_Size"),
                       "/DisplayMenu/Select/SelectBy/SelectByExtensionStart",
                       select_by_size_cb)
