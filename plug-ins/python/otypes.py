#    PyDia Self Documentation Series - Part II : Object Types
#    Copyright (c) 2003, Hans Breuer <hans@breuer.org>
#
#        generates a new diagram which contains all the currently
#    registered object types sorted by their containing package
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

def _log(s, append=1) :
	pass
	if append :
		mode = "a"
	else :
		mode = "w"
	f = open("c:\\temp\\otypes.log", mode)
	f.write(s)

def otypes_cb(data, flags) :

	diagram = dia.new("Object Types.dia")
	layer = diagram.layers[0]

	otypes = dia.registered_types()
	keys = otypes.keys()
	keys.sort()

	packages = {}
	for s in keys :
		kt = string.split(s, " - ")
		if len(kt) == 2 :
			if len(kt[0]) == 0 :
				sp = "<unnamed>"
			else :
				sp = kt[0]
			st = kt[1]
		else :
			sp = "<broken>"
			st = kt[0]
		if packages.has_key(sp) :
			packages[sp].append(st)
		else :
			packages[sp] = [st] 

	dtp = dia.get_object_type("UML - LargePackage")
	dtc = dia.get_object_type("UML - Class")
	np = 0

	for sp in packages.keys() :
		pkg = packages[sp]
		op, h1, h2 = dtp.create(0, np * 10.0)
		op.properties["name"] = sp
		layer.add_object(op)
		nc = 0
		for st in pkg :
			oc, h3, h4 = dtc.create(nc * 10.0 + 1.0, np * 10.0 + 1.0)
			oc.properties["name"] = st
			layer.add_object(oc)
			nc = nc + 1
		h = op.handles[7]
		op.move_handle(h,(nc * 10.0 - 3.0, h.pos[0] + 3.0), 0, 0)
		np = np + 1
	diagram.display()
	diagram.update_extents()
	diagram.flush()

dia.register_callback ("Dia Object Types", 
                       "<Toolbox>/Help/Self Doc/Object Types", 
                       otypes_cb)
