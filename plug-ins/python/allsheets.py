#    PyDia Self Documentation Series - Part VII : All Sheets
#    Copyright (c) 2014  Hans Breuer  <hans@breuer.org>
#
#        generates something form the list of sheets
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
import dia, os
import tempfile
import webbrowser

import gettext
_ = gettext.gettext

# Given a list of "sheet objects" return the common namespace of the object types
def so_get_namespace (sol) :
	names = {}
	if len(sol) < 1 :
		return "Empty"
	for ot, descr, fname in sol :
		if ot :
			sp = ot.name.split(" - ")
			if len(sp) > 1 :
				if sp[0] in names:
					names[sp[0]] += 1
				else :
					names[sp[0]] = 1
	return ",".join (list(names.keys()))

def check_objecttype_overlap (sheets) :
	types = dia.registered_types()
	# remove Standard objects, they do not have or need a sheet
	del types["Group"]
	for s in ["Arc", "Box", "BezierLine", "Beziergon", "Ellipse", "Image", "Line",
		  "Outline", "Path", "Polygon", "PolyLine", "Text", "ZigZagLine"] :
		del types["Standard - %s" % (s,)]
	# got through all the sheets to match against registered types
	missing = []
	for sheet in sheets:
		for ot, descr, fname in sheet.objects:
			if ot.name in types:
				if ot == types[ot.name]:
					del types[ot.name]
				else:
					print("Mix-up:", ot.name)
			else:
				# sheet referencing a type not available
				missing.append(ot.name)
	# from the dictionary removed every type referenced just once?
	print(types)

def isheets_cb (data, flags) :
	sheets = dia.registered_sheets ()
	check_objecttype_overlap (sheets)
	path = tempfile.gettempdir() + os.path.sep + "dia-sheets.html"
	f = open (path, "w")
	f.write ("""
<html><head><title>Dia Sheets</title></head><body>
<table>
""")
	for sheet in sheets :
		info = "Namespace: [%s]<br>%i object types" % (so_get_namespace (sheet.objects), len(sheet.objects))
		sname = sheet.name
		if not sheet.user :
			sname = "<b>" + sname + "</b>"
		f.write ("<tr><td>%s</td><td>%s</td><td>%s</td></tr>\n" % (sname, sheet.description, info))
	f.write ("""</table>
</body></html>
""")
	dia.message(0, "'" + path + "' saved.")
	webbrowser.open('file://' + os.path.realpath(path))

dia.register_action ("HelpInspectSheets", _("Dia _Sheets Inspection"),
                     "/ToolboxMenu/Help/HelpExtensionStart",
                     isheets_cb)
