#  autolayoutforce.py - graph layout plug-in for Dia
#
#  Copyright (C) 2008 Frederic-Gerald Morcos <fred.mrocos@gmail.com>
#  Copyright (c) 2008 Hans Breuer <hans@breuer.org>
#
#  Playground for the "force based autolayout" algorithm initially implemented
# for Dia in C by Fred Morcos
# See also: http://en.wikipedia.org/wiki/Force-based_algorithms

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
import math, dia

import gettext
_ = gettext.gettext

def bbox_area (o) :
	r = o.bounding_box
	return (r.bottom - r.top) * (r.right - r.left)

def attraction (aconst, node, other) :
	"Calculates the attraction between two connected elements"
	p1 = node.properties["obj_pos"].value
	p2 = other.properties["obj_pos"].value
	x = (p2.x - p1.x) * aconst
	y = (p2.y - p1.y) * aconst
	return (x,y)

def repulsion (rconst, node, other) :
	"Calculates the repulsion between any two elements"
	p1 = node.properties["obj_pos"].value
	p2 = other.properties["obj_pos"].value

	m2 = bbox_area (other)
	m1 = bbox_area (node)

	dx = p1.x - p2.x
	dy = p1.y - p2.y

	denom = math.pow (dx * dx + dy * dy, 1.5) # magic number?
	numer = m1 * m2 * rconst

	try :
		fx = (numer * dx) / denom
		fy = (numer * dy) / denom
	except ZeroDivisionError :
		print("ZeroDivisionError")
		return (0,0)

	return (fx,fy)

def layout_force (nodes, rconst, aconst, timestep, damping) :
	energy = [0.0, 0.0]
	for o in nodes :
		netforce = [0.0, 0.0]
		velocity = [0.0, 0.0]
		for oo in nodes :
			if oo != o :
				r = repulsion (rconst, o, oo)
				netforce[0] += r[0]
				netforce[1] += r[1]
		for cpt in o.connections : # connection points
			for co in cpt.connected : # connected objects in this point
				edge = co
				oo = None
				for h in edge.handles : # the edge handles are connected to ...
					cto = h.connected_to # ... the other objects connection point
					if  cto and cto.object != o : # ... one of it being the current
						oo = cto.object
						# we usually only find _one_ other handle but there are exception
						# e.g. "Network - Bus"
						a = attraction (aconst, o, oo)
						netforce[0] += a[0]
						netforce[1] += a[1]
		#print "Netforce", netforce, timestep, damping
		velocity[0] = timestep * netforce[0] * damping
		velocity[1] = timestep * netforce[1] * damping

		# new position
		p = o.properties["obj_pos"].value
		px = p.x + timestep * velocity[0]
		py = p.y + timestep * velocity[1]
		#print "move", timestep * velocity[0], timestep * velocity[1]
		o.move (px, py)

		mass = bbox_area (o)
		energy[0] += mass * math.pow (velocity[0], 2) / 2
		energy[1] += mass * math.pow (velocity[1], 2) / 2
	return energy

def layout_force_cb(data, flags):
	# the things (nodes) we are moving around are all connected 'elements',
	# connection objects are only moving as a side effect
	nodes = []
	for o in data.selected :
		for cpt in o.connections : # ConnectionPoint
			if len (cpt.connected) > 0 :
				nodes.append (o)
				break
	# this ususally is an iterative process, finished if no energy is left
	#FIXME: layout_force (nodes, 2.0, 2.0, 0.2, 0.5) PASSES 0.0, 0.0
	e = layout_force (nodes, 2.0, 3.0, 2e-1, 5e-1)
	n = 0 # arbitrary limit to avoid endless loop
	while (e[0] > 1 or e[1] > 1) and n < 100 :
		e = layout_force (nodes, 2.0, 3.0, 2e-1, 5e-1)
		n += 1
	for o in nodes :
		data.update_connections (o)
	data.active_layer.update_extents() # data/diagram _update_extents don't recalculate?
	data.update_extents ()
	data.flush()
	print(n, "iterations")

dia.register_action ("LayoutForcePy", _("_Layout (force)"),
                     "/DisplayMenu/Test/TestExtensionStart",
                     layout_force_cb)
