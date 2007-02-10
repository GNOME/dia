#    Select Objects By Common Properties
#    Copyright (c) 2003, Hans Breuer <hans@breuer.org>
#
#        different methods (menu entries) to select all objects with given
#    property, that is :
#	- a find dialog to select by name
#	- menu entries for different color types. The common color is read
#	  from already selected objects

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

import sys, dia

class CFindDialog :
	def __init__(self, d, data) :
		import pygtk
		pygtk.require("2.0")
		import gtk

		win = gtk.Window()
		win.connect("delete_event", self.on_delete)
		win.set_title("Select by name")

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
		self.entry.set_text("enter name")
		box2.pack_start(self.entry)
		self.entry.show()

		separator = gtk.HSeparator()
		box1.pack_start(separator, expand=0)
		separator.show()

		box2 = gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2, expand=0)
		box2.show()

		button = gtk.Button("find")
		button.connect("clicked", self.on_find)
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
		win.show()

	def on_find(self, *args) :
		s = self.entry.get_text()
		select_by (self.diagram, self.data, "name", s)
		self.data.update_extents ()
		self.diagram.flush()

	def on_delete (self, *args) :
		self.win.destroy ()

def do_dialog (d, data) :
	# let the dialog do it's work
	dlg = CFindDialog (d, data)

def select_by (diagram, data, name, value) :
	objs = data.active_layer.objects
	for o in objs :
		if o.properties.has_key (name) :
			print "<==", o.properties[name].value
			if value == None or o.properties[name].value == value :
				diagram.select (o)

def select_by_name_cb (data, flags) :
	d = dia.active_display().diagram
	do_dialog (d, data)

def select_by_selected (data, name) :
	d = dia.active_display().diagram
	grp = data.get_sorted_selected()
	bFoundAny = 0
	for o in grp :
		if o.properties.has_key(name) :
			select_by (d, data, name, o.properties[name].value)
			bFoundAny = 1
	if not bFoundAny :
		dia.message(0, "No selected object has the property '%s'." % name)
	data.update_extents ()
 
def select_by_fill_color_cb (data, flags) :
	select_by_selected (data, "fill_colour")
def select_by_line_color_cb (data, flags) :
	select_by_selected (data, "line_colour")
def select_by_text_color_cb (data, flags) :
	select_by_selected (data, "text_colour")

dia.register_action ("SelectByName", "Name", 
                       "/DisplayMenu/Select/SelectBy/SelectByExtensionStart", 
                       select_by_name_cb)
dia.register_action ("SelectByFillcolor", "Fill Color", 
                       "/DisplayMenu/Select/SelectBy/SelectByExtensionStart", 
                       select_by_fill_color_cb)
dia.register_action ("SelectByLinecolor", "Line Color", 
                       "/DisplayMenu/Select/SelectBy/SelectByExtensionStart", 
                       select_by_line_color_cb)
dia.register_action ("SelectByTextcolor", "Text Color", 
                       "/DisplayMenu/Select/SelectBy/SelectByExtensionStart", 
                       select_by_text_color_cb)
