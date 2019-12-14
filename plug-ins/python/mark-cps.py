# A little debug helper marking and numbering the connection points
# of selected objects
# Copyright (c) 2013  Hans Breuer <hans@breuer.org>

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

import dia

import gettext
_ = gettext.gettext

def mark_cps (data, flags) :
	objs = data.get_sorted_selected()
	layer = data.active_layer
	if len(objs) == 0 :
		dia.message (1, "Select objects for marking it's connection points!")
		return
	textType = dia.get_object_type("Standard - Text")
	for o in objs:
		for i in range(0, len(o.connections)):
			cp = o.connections[i]
			t, h1, h2 = textType.create (cp.pos.x, cp.pos.y)
			t.properties["text"] = str(i)
			# align the text based on the given directions
			if cp.flags & 0x3 : #CP_FLAGS_MAIN
				t.properties["text_vert_alignment"] = 3 # first line
			if cp.directions & 0x1: # north
				t.properties["text_vert_alignment"] = 1 # bottom
			elif cp.directions & 0x4: # south
				t.properties["text_vert_alignment"] = 0 # top
			else :
				t.properties["text_vert_alignment"] = 2 # center
			if cp.flags & 0x3 : #CP_FLAGS_MAIN
				t.properties["text_alignment"] = 1 # middle
			elif cp.directions & 0x2: # east
				t.properties["text_alignment"] = 0 # left
			elif cp.directions & 0x8: # west
				t.properties["text_alignment"] = 2 # right
			else :
				t.properties["text_alignment"] = 1 # middle
			# tint it with the connection point color
			if cp.flags & 0x3 : #CP_FLAGS_MAIN
				t.properties["text_colour"] = "red"
			elif cp.directions == 0 : # not necessarily a bug
				t.properties["text_colour"] = "green"
			else :
				t.properties["text_colour"] = "blue"
			# add it to the diagram
			layer.add_object(t)
			# connect the object with the cp at hand
			h2.connect(cp)
		# update the object and it's connected?
	data.update_extents ()
	adisp = dia.active_display()
	if adisp :
		adisp.diagram.update_extents()
		adisp.diagram.flush()


dia.register_action("DebugMarkConnectionPoints", _("_Mark Connection Points"),
		    "/DisplayMenu/Debug/DebugExtensionStart", mark_cps)
