#   Copyright (c) 2006, Hans Breuer <hans@breuer.org>
#
#   Draws bounding boxes of the objects in the active layer
#   into a new layer. Helps analyzing the font size problems
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

import sys, dia, string

import gettext
_ = gettext.gettext

def bbox_cb (data, flags) :

	layer = data.active_layer
	dest = data.add_layer ("BBox of '%s' (%s)" % (layer.name, sys.platform), -1)
	box_type = dia.get_object_type ("Standard - Box")

	for o in layer.objects :
		bb = o.bounding_box
		b, h1, h2 = box_type.create (bb.left, bb.top)
		b.move_handle (b.handles[7], (bb.right, bb.bottom), 0, 0)
		b.properties["show_background"] = 0
		b.properties["line_width"] = 0
		b.properties["line_colour"] = 'red'
		dest.add_object (b)

def annotate_cb (data, flags) :

	layer = data.active_layer
	dest = data.add_layer ("Annotated '%s' (%s)" % (layer.name, sys.platform), -1)
	ann_type = dia.get_object_type ("Standard - Text")

	for o in layer.objects :
		bb = o.bounding_box
		a, h1, h2 = ann_type.create (bb.right, bb.top)

		a.properties["text"] = "h: %g w: %g" % (bb.bottom - bb.top, bb.right - bb.left)

		dest.add_object (a)

dia.register_action ("DrawBoundingbox", "_Draw BoundingBox",
                     "/DisplayMenu/Debug/DebugExtensionStart",
                     bbox_cb)

dia.register_action ("AnnotateMeasurements", "_Annotate",
                     "/DisplayMenu/Debug/DebugExtensionStart",
                     annotate_cb)
