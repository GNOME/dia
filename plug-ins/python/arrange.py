# Copyright (c) 2007  Hans Breuer <hans@breuer.org>

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
import math

import gettext
_ = gettext.gettext

def DeepCalc (dict, key, seen = None) :
	# calculate all (deep) dependencies
	# give a directory with component name as key and dependency components as list in dict[key][2]
	if not seen :
		seen = {}
	for k in dict[key][2] :
		if seen and k in seen:
			continue
		seen[k] = 1
		DeepCalc (dict, k, seen)
	return len(list(seen.keys()))

##
# \brief Callback function to be invoked by Dia's menu
#
# Implements a simple layout algorithm based on object connections
#
def arrange_connected (data, flags) :
	objs = data.get_sorted_selected()
	if len(objs) == 0 :
		objs = data.active_layer.objects
	# all objects having at least one connection
	bs = {}
	print("objs", len(objs))
	edges = {}
	for o in objs :
		for c in o.connections: # ConnectionPoint
			for n in c.connected: # connection object
				if not (n.handles[0].connected_to and n.handles[1].connected_to):
					continue
				a = n.handles[0].connected_to.object
				b = n.handles[1].connected_to.object
				ak = a.properties["name"].value
				bk = b.properties["name"].value
				# create an edge key
				ek = ak + "->" + bk
				if ek in edges:
					continue # already seen
				print(ek)
				edges[ek] = 1
				if ak in bs:
					use = bs[ak]
					use[2].append (bk)
					bs[ak] = use
				else :
					bs[ak] = [a, 0, [bk]]
				if bk in bs:
					use = bs[bk]
					use[1] += 1
					bs[bk] = use
				else :
					bs[bk] = [b, 1, []]
	# sort by number of connections
	bst = []
	dx = 0
	dy = 0
	for key in list(bs.keys()) :
		o = bs[key][0]
		if not o.properties["elem_width"] :
			print(o)
			continue
		if o.properties["elem_width"].value > dx : dx = o.properties["elem_width"].value
		if o.properties["elem_height"].value > dy : dy = o.properties["elem_height"].value
		n = bs[key][1] # (use count, dependencies)
		bst.append ((o, n, DeepCalc (bs, key)))
	bst.sort(key=lambda a: a[1])
	if len(bs) < 2 :
		return
	# average weight gives the number of rows
	aw = 0.0
	for t in bst :
		aw += (t[1] + t[2])
	aw = aw / len(bst)
	rows = int (len(bst) / math.sqrt(aw))
	if rows < 2 :
		rows = 2
	offsets = []
	for i in range(0, rows) : offsets.append(0)
	dx *= 1.4
	dy *= 1.8
	for t in bst :
		y = t[1] # t[1] = 0 :  start, noone uses it
		# correction to move further down
		c = rows - (rows * t[2] / aw)
		if t[1] : # the less users the far below
			if t[2] == 0 :
				y = rows - 1
			elif t[1] >= rows :
				# compensate for some of the weight, FIXME: better guess needed
				y = int(rows - 2 * aw / (t[2] + t[1]))
		print(t[0].properties["name"].value, t[1], t[2], y, c)
		# move the object to it's new place
		x = offsets[y]
		t[0].move (x * dx, y * dy)
		offsets[y] += 1
	data.update_extents ()

##
# \file arrange.py \brief Arrange Objects Plugin
#
# this module is loaded by some other plug-ins but can also work on it's own
# if it is loaded first as Dia plug-in and later as Python module everything works
# fine due to Pythoninitializing the module only once
#
# \ingroup PyDia
dia.register_callback (_("Arrange _Objects"),
                       "<Display>/Objects/Arrange",
                       arrange_connected)
