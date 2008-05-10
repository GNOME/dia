#    Select empyt objects
#    Copyright (c) 2008, Hans Breuer <hans@breuer.org>

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

def select_empty_cb (data, flags) :
	diagram = dia.active_display().diagram
	objs = data.active_layer.objects
	for o in objs :
		if o.bounding_box.right == o.bounding_box.left \
			or o.bounding_box.top == o.bounding_box.bottom :
			diagram.select (o)

dia.register_action ("SelectEmpty", "Empty", 
                       "/DisplayMenu/Select/By/SelectByExtensionStart", 
                       select_empty_cb)
