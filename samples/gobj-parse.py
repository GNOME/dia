'''
Parses class definitions out of GObject based headers

ToDo:
	- better parser aproach ;)
	- respect /*< public,protected,private >*/
	- parse *.c

'''
#    Copyright (c) 2006, Hans Breuer <hans@breuer.org>

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

import glob, os, re

verbose = 0

def strip_whitespace (s) :
	s = s.replace (" ", "")
	return s.replace ("\t", "")

class CMethod :
	def __init__ (self, name, type) :
		self.name = name.strip()
		self.pars = []
		self.retval = type.replace(' ', '')
	def AddPar (self, name, type) :
		self.pars.append ((name.strip(),
					 strip_whitespace(type)))

class CClass :
	def __init__ (self, name) :
		self.name = name.strip()
		self.defs = {'TYPE' : None, 'OBJECT' : None, 'CLASS' : None}
		self.parent = None
		self.attrs = []
		self.methods = {}
		self.signals = []
	def AddAttr (self, name, type) :
		if verbose : print("\t", type, name)
		self.attrs.append ((name.strip(),
					  strip_whitespace(type)))
	def AddMethod (self, name, method) :
		if verbose : print("\t", name, "()")
		self.methods[name.strip()] = method
	def AddSignal (self, name, type) :
		self.signals.append ((name.strip(),
					    strip_whitespace(type)))

class CStripCommentsAndBlankLines :
	def __init__ (self, name) :
		f = open (name)
		lines = f.readlines ()
		self.lines = []
		self.cur = -1
		x1 = -1
		x2 = -1
		incom = 0
		r = re.compile(r"^\s*$") # to remove empty lines
		for s in lines :
			x1 = '/*'.find (s)
			x2 = '*/'.find (s)
			while x2 > x1 and incom != 1 :
				s = s[:x1] + s[x2+2:]
				x1 = '/*'.find (s)
				x2 = '*/'.find (s)
			else :
				if x1 > -1 :
					incom = 1
					s = s[:x1]
				elif x2 > - 1 and incom :
					s = s[x2+2:]
					incom = 0
			if r.match (s) or incom :
				continue
			self.lines.append (s)
		self.lines.append ("")
		f.close ()
	def readline (self) :
		self.cur = self.cur + 1
		try :
			return self.lines[self.cur]
		except :
			return ""
	def seek (self, toPos) :
		if toPos == 0 :
			self.cur = -1

def TestStripFile (name) :
	f = CStripCommentsAndBlankLines(name)
	s = f.readline()
	while s :
		print(s[:-1])
		s = f.readline()

import sys

sPkg = ''
sDir = '.'
if len (sys.argv) < 2 :
	print(sys.argv[0], '<package>', '[directory]')
	sys.exit(0)
else :
	sPkg = sys.argv[1]
if len (sys.argv) > 2 :
	sDir = sys.argv[2]

os.chdir (sDir)
lst  = glob.glob ('*.h')
lst.extend (glob.glob ("*/*.h"))
lst.extend (glob.glob ("*/*/*.h"))

klasses = []
no_klass_headers = []

#
# #define GDK_DRAWABLE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DRAWABLE, GdkDrawable))
#
rClassCast = re.compile (r"^#define " + sPkg.upper () + "_(?P<n1>\w+)" \
				 "\((?P<par>\w+)\)\s+\(" \
				 "((G_TYPE_CHECK_INSTANCE_CAST)|(GTK_CHECK_CAST))\s+" \
				 "\(\((?P=par)\),\s*(?P<type>\w+),\s+" \
				 "(?P<name>\w+)\)\).*")
#
# m.group('type') == 'GDK_TYPE_DRAWABLE'
# m.group('name') == 'GdkTypeDrawable'
#
rVarDecl = re.compile (r"\s*(?P<type>(unsigned)*\s*\w+\s*\**)\s*(?P<name>\w+)\s*:*\s*\d*\s*;\.*")
rSignal = rVarDecl #FIXME: signals are 'methods' to be registered
rMethod = re.compile (r"\s*(?P<type>\w+\s+\**)\s*" \
			    "\(\*\s*(?P<name>\w+)\)\s*\(" \
			    "(?P<ptype>\w+\s*\**)\s*(?P<pname>\w+),*" \
			    "(?P<done>(\);)*)")
# m.group('type') == retval
# m.group('name')
# m.group('ptype'), m.group('pname')
# m.group('done')
rPar = re.compile (r"\s*(const\s*)*(?P<ptype>\w+\s*\**)\s*(?P<pname>\w+),*" \
			 "(?P<done>(\);)*)")

fout = open (sPkg + '-generated.log', 'w')
# the type cast is not necessarily in the same header ;(
unresolved = {}
for sF in lst :
	sName = sF[len(sPkg):-2] # without case (approximation)
	# fin = open (sF)
	fin = CStripCommentsAndBlankLines(sF)
	s = fin.readline ()
	while s :
		m = rClassCast.match (s)
		if m :
			fout.write (m.group('type') + ' ' + m.group('name') + '\n')
			sName = m.group('name')
			unresolved[sName] = CClass (sName)
		s = fin.readline ()
	if len(list(unresolved.keys())) == 0 :
		no_klass_headers.append (sF)
	else :
		iFound = 0
		unresolved_keys = list(unresolved.keys())
		for sK in unresolved_keys :
			klass = unresolved[sK]
			sK = klass.name
			# start from the beginning
			fin.seek (0)
			rObject = re.compile (r"struct\s+_" + sK + "\s*(?P<p>\{*)\s*$")
			rKlass  = re.compile (r"struct\s+_" + sK + "Class\s*(?P<p>\{*)\s*$")
			s = fin.readline ()
			meth = None
			while s :
				mO = rObject.match (s)
				if mO :
					if mO.group('p') != '{' :
						s = fin.readline() # skip line with {
					s = fin.readline() # read parent line
					mV = rVarDecl.match(s)
					try :
						klass.parent = (mV.group('name'), mV.group('type').strip())
						if verbose : print("class", sK, ":", mV.group('type'))
					except :
						print('klass.parent error (', sF, ') :', s)
					s = fin.readline ()
					mV = rVarDecl.match(s)
					if not mV and verbose : print(s)
					while mV :
						klass.AddAttr (mV.group('name'), mV.group('type'))
						s = fin.readline ()
						mV = rVarDecl.match(s)

				else :
					mK = rKlass.match (s)
					if mK :
						iFound = iFound + 1
						klasses.append (unresolved[sK])
						del unresolved[sK]

						if mK.group('p') != '{' :
							s = fin.readline() # skip line with {
						s = fin.readline() # read parent line, validate it?
						s = fin.readline()
						mS = rSignal.match(s)
						mM = rMethod.match(s)
						while (mS or mM or meth) and s :
							if mS :
								klass.AddSignal (mS.group('name'), mS.group('type'))
							elif mM :
								meth = CMethod (mM.group('name'), mM.group('type'))
								klass.AddMethod (mM.group('name'), meth)
								meth.AddPar (mM.group('pname'), mM.group('ptype'))
								if mM.group('done') == ');' :
									meth = None # reset
							elif meth :
								mP = rPar.match (s)
								if mP :
									meth.AddPar (mP.group('pname'), mP.group('ptype'))
									if mP.group('done') == ');' :
										meth = None # reset
								else :
									# fixme: too drastic?
									#meth = None # reset
									pass
							else :
								break
							s = fin.readline ()
							mS = rSignal.match(s)
							mM = rMethod.match(s)
				s = fin.readline ()
		if iFound == 0 :
			no_klass_headers.append (sF)
		else :
			print(sF + " (" + str(iFound) + ")")

for sF in no_klass_headers :
	fout.write ("<No Klass>: " + sF + "\n")
print('Klasses found:', len(klasses), '\nHeaders w/o classes:', len(no_klass_headers))

def CmpParent (a,b) :
	'gets CClass, sort by parent type'
	if a.parent == None and b.parent == None :
		return 0
	elif a.parent == None :
		return 1
	elif b.parent == None :
		return -1
	try :
		i = cmp(a.parent[1],b.parent[1])
		if i == 0 :
			i = cmp(a.name, b.name)
		elif cmp(a.name, b.parent[1]) == 0 :
			#print a.name, ">", b.name
			return 1
		elif cmp(b.name, a.parent[1]) == 0 :
			#print b.name, ">", a.name
			return -1
		return i
	except :
		print("Sort Error:", str(a.parent), str(b.parent))
		return 0

klasses.sort(CmpParent)
# the sorting above does not sort everything, ensure parents are before childs
sorted = []
sorted_klasses = []
# first put in externals
parents = {}
for k in klasses :
	if k.parent :
		parents[k.parent[1]] = 1
for k in klasses :
	if k.name in parents :
		del parents[k.name]
sorted.extend(list(parents.keys()))
# sort the rest
while len(sorted_klasses) < len(klasses) :
	before = len(sorted_klasses)
	for k in klasses :
		sK = k.name
		sP = k.parent
		if sK in sorted :
			continue # don't add tem twice
		elif k.parent is None :
			sorted.append(sK)
		elif k.parent[1] in sorted :
			sorted.append(sK)
		else :
			continue
		#print sK
		sorted_klasses.append(k)
	if len(sorted_klasses) == before :
		if len(sorted_klasses) < len(klasses) :
			unsorted = []
			for k in klasses :
				if not k.name in sorted :
					unsorted.append(k.name)
			print(", ".join(unsorted), "not sorted?")
		break # avoid endless loop

klasses = sorted_klasses

def WritePython (fname) :
	# generate Python declaration for validation
	fpy = open (fname, "w")
	for klass in klasses :
		sParent = ""
		if klass.parent :
			sParent = " (" + klass.parent[1] + ")"
		fpy.write ('class ' + klass.name + sParent + " :\n")
		if len (klass.attrs) > 0 :
			fpy.write ('\tdef __init__ (self) :\n')
			for attr in klass.attrs :
				fpy.write ('\t\tself.' + attr[0] + " = None # " + attr[1] + "\n")
		if len (klass.signals) > 0 :
			fpy.write ('\t\t # Signals\n')
			for attr in klass.signals :
				fpy.write ('\t\tself.' + attr[0] + " = None # " + attr[1] + "\n")
		for s in list(klass.methods.keys()) :
			meth = klass.methods[s]
			fpy.write ('\t#returns: ' + meth.retval + '\n\tdef ' + meth.name + ' (')
			s1 = ''
			s2 = ''
			for par in meth.pars :
				s1 = s1 + par[0] + ', '
				s2 = s2 + par[1] + ', '
			if len(s1) > 0 :
				s1 = s1[:-2]
			if len(s2) > 0 :
				s2 = s2[:-2]
			fpy.write (s1 + ') :\n')
			fpy.write ('\t\t# ' + s2 + '\n')
			fpy.write ('\t\tpass\n')
		if len (klass.attrs) < 1 and len(klass.signals) < 1 and len(klass.methods) < 1 :
			fpy.write ('\tpass\n') # make it valid Python

def WriteDia (fname) :
	sStartDiagram = '''<?xml version="1.0" encoding="UTF-8"?>
<dia:diagram xmlns:dia="http://www.lysator.liu.se/~alla/dia/">
  <dia:layer name="Background" visible="true">'''
	sStartClass = '''
    <dia:object type="UML - Class" version="0" id="O%d">
      <dia:attribute name="elem_corner">
        <dia:point val="%d,%d"/>
      </dia:attribute>
      <dia:attribute name="name">
        <dia:string>#%s#</dia:string>
      </dia:attribute>
      <dia:attribute name="visible_attributes">
        <dia:boolean val="true"/>
      </dia:attribute>
      <dia:attribute name="visible_operations">
        <dia:boolean val="true"/>
      </dia:attribute>'''
	sFillColorAttribute = '''
      <dia:attribute name="fill_color">
        <dia:color val="%s"/>
      </dia:attribute>'''
	sStartAttributes = '''
      <dia:attribute name="attributes">
        <dia:composite type="umlattribute">'''
	sDataAttribute = '''
          <dia:attribute name="name">
            <dia:string>#%s#</dia:string>
          </dia:attribute>
          <dia:attribute name="type">
            <dia:string>#%s#</dia:string>
          </dia:attribute>'''
	sEndAttributes = '''
        </dia:composite>
      </dia:attribute>'''
	sStartOperations = '''
      <dia:attribute name="operations">'''
	sStartOperation = '''
        <dia:composite type="umloperation">
          <dia:attribute name="name">
            <dia:string>#%s#</dia:string>
          </dia:attribute>
          <dia:attribute name="type">
            <dia:string>#%s#</dia:string>
          </dia:attribute>
          <dia:attribute name="parameters">
            <dia:composite type="umlparameter">'''
	sDataParameter = '''
              <dia:attribute name="name">
                <dia:string>#%s#</dia:string>
              </dia:attribute>
              <dia:attribute name="type">
                <dia:string>#%s#</dia:string>
              </dia:attribute>'''
	sEndOperation = '''
            </dia:composite>
          </dia:attribute>
        </dia:composite>'''
	sEndOperations = '''
      </dia:attribute>'''
	sStartConnection = '''
    <dia:object type="UML - Generalization" version="1" id="O%d">
      <dia:attribute name="obj_pos">
        <dia:point val="%d,%d"/>
      </dia:attribute>
      <dia:attribute name="orth_autoroute">
        <dia:boolean val="true"/>
      </dia:attribute>'''
	sOrthPoints = '''
      <dia:attribute name="orth_points">
        <dia:point val="%d,%d"/>
        <dia:point val="%d,%d"/>
        <dia:point val="%d,%d"/>
        <dia:point val="%d,%d"/>
      </dia:attribute>'''
	sEndObject = '''
    </dia:object>'''
	sEndDiagram = '''
  </dia:layer>
</dia:diagram>'''

	fdia = open (fname, "w")
	nObject = 0
	x = 0
	y = 0
	dx = 10
	dy = 15
	# maintain a dictionary of parents positions to at least place not everything above each other
	positions = {}
	connectFrom = {}
	externals = {}
	fdia.write (sStartDiagram)

	for klass in klasses :
		if klass.parent : # add every parent ...
			parentName = klass.parent[1]
			if parentName in externals :
				externals[parentName] += 1
			else :
				externals[parentName] = 1
	for klass in klasses :
		if klass.name in externals : # ... but remove  the internals
			del externals[klass.name]

	# write all 'external' parents
	for s in list(externals.keys()) :
		externals[s] = (nObject, -1)
		fdia.write(sStartClass % (nObject, x, y, s))
		positions[s] = (x,y)
		x += dx
		fdia.write(sFillColorAttribute % ("#ffff00",))
		# fixme: any more attributes?
		fdia.write (sEndObject)
		nObject += 1

	for klass in klasses :
		parentName = ""
		if klass.parent :
			parentName = klass.parent[1]
			connectFrom[klass.name] = (nObject, parentName)
		if parentName in positions :
			x = positions[parentName][0] # same x
			y = positions[parentName][1] + dy # y below
		else :
			x += dx
			y = dy
		#fpy.write ('class ' + klass.name + sParent + " :\n")
		fdia.write(sStartClass % (nObject, x, y, klass.name))
		positions[klass.name] = (x, y)
		if len (klass.attrs) > 0 :
			fdia.write (sStartAttributes)
			for attr in klass.attrs :
				fdia.write (sDataAttribute % (attr[0], attr[1]))
			fdia.write (sEndAttributes)
#		if len (klass.signals) > 0 :
#			fpy.write ('\t\t # Signals\n')
#			for attr in klass.signals :
#				fpy.write ('\t\tself.' + attr[0] + " = None # " + attr[1] + "\n")
		# the differnence between signals and methods is in the attributes
		if len (klass.signals) > 0 or len(list(klass.methods.keys())) > 0  :
			fdia.write (sStartOperations)
		for s in list(klass.methods.keys()) :
			meth = klass.methods[s]
			fdia.write(sStartOperation % (meth.name, meth.retval))
			# first parameter is supposed to be 'this' pointer: leave out
			for par in meth.pars[1:] :
				fdia.write (sDataParameter % (par[0], par[1]))
			fdia.write (sEndOperation)
		if len (klass.signals) > 0 or len(list(klass.methods.keys())) > 0  :
			fdia.write (sEndOperations)
		fdia.write (sEndObject)
		nObject += 1

	# write all connections
	for sFrom in list(connectFrom.keys()) :
		iFrom = connectFrom[sFrom][0]
		sTo = connectFrom[sFrom][1]
		if sTo in connectFrom :
			iTo = connectFrom[sTo][0]
		elif sTo in externals :
			iTo = externals[sTo][0]
		else :
			print("sFrom -> sTo?", sFrom, sTo)
			continue # something wrong?
		nObject += 1
		#fdia.write ('\n\t<!-- %s : %s -->' % (sFrom, sTo))
		# just to give it some position (and stop Dia complaining)
		if sTo in positions :
			x1, y1 = positions[sTo]
		else :
			x1, y1 = (dx, dy)
		if sFrom in positions :
			x2, y2 = positions[sFrom]
		else :
			x2, y2 = (dx, dy)
		fdia.write (sStartConnection % (nObject, x1+dx/2, y1+dy/2,))
		fdia.write (sOrthPoints % (x1,y1, x1,(y1+y2)/2, x2,(y1+y2)/2, x2,y2 ))
		# and connect
		fdia.write ('''
      <dia:connections>
        <dia:connection handle="0" to="O%d" connection="6"/>
        <dia:connection handle="1" to="O%d" connection="1"/>
      </dia:connections>''' % (iTo, iFrom) )
		fdia.write (sEndObject)

	fdia.write (sEndDiagram)
	print(len(list(connectFrom.keys())), " connections")

WritePython (sPkg + "-generated.py")
WriteDia (sPkg + "-generated.dia")
