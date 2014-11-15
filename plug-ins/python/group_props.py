#    Group Properties Prototype
#    Copyright (c) 2003, Hans Breuer <hans@breuer.org>
#
#        with multiple selected (but not grouped) objects builds a dialog
#    which contains widgets to change all different properties of the
#    objects.
#
#    Known Issues :
#	- Currently the options to be changed are represented by strings
#	  This is a limitation of PyDia, which does not wrap Dia's UI
#	  elements yet
#	- ...

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

class CPropsDialog :
	def __init__(self, diagram, data, props) :
		import pygtk
		pygtk.require("2.0")
		import gtk

		self.diagram = diagram
		self.data = data
		self.props = props

		self.win = gtk.Window ()
		self.win.connect("delete_event", self.on_delete)
		self.win.set_title("Group Properties")

		box1 = gtk.VBox()
		self.win.add(box1)
		box1.show()

		box2 = gtk.VBox(spacing=2)
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()

		self.checkboxes = []
		self.optionmenues = []
		if len(props) :
			table = gtk.Table(2, len(props), 0)
		else :
			table = gtk.Table(2, 1, 0)
		table.set_row_spacings(2)
		table.set_col_spacings(5)
		table.set_border_width(5)
		if len(props) :
			y = 0
			for s in props.keys() :
				w = gtk.CheckButton(s)
				self.checkboxes.append(w)
				table.attach(w, 0, 1, y, y+1)
				w.show()
				menu = gtk.Menu()
				milist = None
				for opt in props[s].opts :
					#print opt
					menuitem = gtk.RadioMenuItem (milist, str(opt.value))
					milist = menuitem # GSlist
					menu.append(menuitem)
					menuitem.show()
				menu.show ()
				w = gtk.OptionMenu()
				w.set_menu(menu)
				self.optionmenues.append(w)
				table.attach(w, 1, 2, y, y+1)
				w.show()
				y = y + 1
		else :
			w = gtk.Label("The selected objects don't share any\n properties to change at once.") 
			table.attach(w, 0, 1, y, y+1)
			w.show()
		box2.pack_start(table)
		table.show()

		separator = gtk.HSeparator()
		box1.pack_start(separator, expand=0)
		separator.show()

		box2 = gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2, expand=0)
		box2.show()

		button = gtk.Button("Ok")
		button.connect("clicked", self.on_ok)
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
		self.win.show()

	def on_ok (self, *args) :
		grp = self.diagram.get_sorted_selected()
		# change the requested properties
		for i in range(0, len(self.checkboxes)) :
			cb = self.checkboxes[i]
			om = self.optionmenues[i]
			if cb.get_active() :
				for o in grp :
					s = self.props.keys()[i]
					o.properties[s] = self.props[s].opts[om.get_history()]
		self.data.update_extents ()
		self.diagram.flush()
		self.win.destroy()

	def on_delete (self, *args) :
		self.win.destroy ()

class PropInfo :
	def __init__ (self, t, n, o) :
		self.num = 1
		self.type = t
		self.name = n
		self.opts = [o]
	def __str__ (self) :
		return self.name + ":" + str(self.opts)
	def Add (self, o) :
		self.num = self.num + 1
		self.opts.append(o)

def dia_objects_props_cb (data, flags) :
	d = dia.active_display().diagram
	grp = d.get_sorted_selected()
	allProps = {}
	numProps = len(grp)
	# check for properties common to all select objects
	for o in grp :
		props = o.properties
		for s in props.keys() :
			if props[s].visible :
				if allProps.has_key(s) :
					allProps[s].Add(props[s])
				else :
					allProps[s] = PropInfo(props[s].type, props[s].name, props[s])
	# now eliminate all props not common ...
	for s in allProps.keys() :
		if allProps[s].num < numProps :
			del allProps[s]
	# ... and all props already equal
	for s in allProps.keys() :
		o1 = allProps[s].opts[0]
		for o in allProps[s].opts :
			if o1.value != o.value :
				o1 = None
				break
		if o1 != None :
			del allProps[s]
		else :
			# if there is something left ensure unique values
			uniques = {}
			for o in allProps[s].opts :
				if uniques.has_key(o.value) :
					continue
				uniques[o.value] = o
			allProps[s].opts = []
			for v in uniques.keys() :
				allProps[s].opts.append(uniques[v])
	# display the dialog
	try :
		dlg = CPropsDialog(d, data, allProps)
	except ImportError :
		dia.message(0, "Dialog creation failed. Missing pygtk?")

dia.register_action ("DialogsGroupproperties", "Dia Group Properties", 
                      "/DisplayMenu/Dialogs/DialogsExtensionStart", 
                       dia_objects_props_cb)
