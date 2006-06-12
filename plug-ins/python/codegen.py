# PyDia Code Generation from UML Diagram
# Copyright (c) 2005  Hans Breuer <hans@breuer.org>

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

class Klass :
	def __init__ (self, name) :
		self.name = name
		self.attributes = {}
		# a list, as java/c++ support multiple methods with the same name
		self.operations = []
		self.comment = ""
		self.parents = []
		self.templates = []
		self.inheritance_type = ""
	def AddAttribute(self, name, type, visibility, value) :
		self.attributes[name] = (type, visibility, value)
	def AddOperation(self, name, type, visibility, params, inheritance_type, comment, class_scope) :
		self.operations.append((name,(type, visibility, params, inheritance_type, comment, class_scope)))
	def SetComment(self, s) :
		self.comment = s
	def AddParrent(self, parrent):
		self.parents.append(parrent)
	def AddTemplate(self, template):
		self.templates.append(template)
	def SetInheritance_type(self, inheritance_type):
		self.inheritance_type = inheritance_type

class ObjRenderer :
	"Implements the Object Renderer Interface and transforms diagram into its internal representation"
	def __init__ (self) :
		# an empty dictionary of classes
		self.klasses = {}
		self.arrows = []
		self.filename = ""
		
	def begin_render (self, data, filename) :
		self.filename = filename
		for layer in data.layers :
			# for the moment ignore layer info. But we could use this to spread accross different files
			for o in layer.objects :
				if o.type.name == "UML - Class" :
					#print o.properties["name"].value
					k = Klass (o.properties["name"].value)
					k.SetComment(o.properties["comment"].value)
					if o.properties["abstract"].value:
						k.SetInheritance_type("abstract")
					if o.properties["template"].value:
						k.SetInheritance_type("template")
					for op in o.properties["operations"].value :
						# op : a tuple with fixed placing, see: objects/UML/umloperations.c:umloperation_props
						# (name, type, comment, stereotype, visibility, inheritance_type, class_scope, params)
						params = []
						for par in op[8] :
							# par : again fixed placement, see objects/UML/umlparameter.c:umlparameter_props
							params.append((par[0], par[1]))
						k.AddOperation (op[0], op[1], op[4], params, op[5], op[2], op[7])
					#print o.properties["attributes"].value
					for attr in o.properties["attributes"].value :
						# see objects/UML/umlattributes.c:umlattribute_props
						#print "\t", attr[0], attr[1], attr[4]
						k.AddAttribute(attr[0], attr[1], attr[4], attr[2])
					self.klasses[o.properties["name"].value] = k
					#Connections
				elif o.type.name == "UML - Association" :
					# should already have got attributes relation by names
					pass
				# other UML objects which may be interesting
				# UML - Note, UML - LargePackage, UML - SmallPackage, UML - Dependency, ...
		
		edges = {}
		for layer in data.layers :
			for o in layer.objects :
				for c in o.connections:
					for n in c.connected:
						if not n.type.name in ("UML - Generalization", "UML - Realizes"):
							continue
						if str(n) in edges:
							continue
						edges[str(n)] = None
						if not (n.handles[0].connected_to and n.handles[1].connected_to):
							continue
						par = n.handles[0].connected_to.object
						chi = n.handles[1].connected_to.object
						if not par.type.name == "UML - Class" and chi.type.name == "UML - Class":
							continue
						par_name = par.properties["name"].value
						chi_name = chi.properties["name"].value
						if n.type.name == "UML - Generalization":
							self.klasses[chi_name].AddParrent(par_name)
						else: self.klasses[chi_name].AddTemplate(par_name)
					
	def end_render(self) :
		# without this we would accumulate info from every pass
		self.attributes = {}
		self.operations = {}

class PyRenderer(ObjRenderer) : 
	def __init__(self) :
		ObjRenderer.__init__(self)
	def end_render(self) :
		f = open(self.filename, "w")
		for sk in self.klasses.keys() :
			parents = self.klasses[sk].parents + self.klasses[sk].templates
			if not parents:
				f.write ("class %s :\n" % (sk,))
			else:
				f.write ("class %s (%s) :\n" % (sk,", ".join(parents)))
			f.write ("\tdef __init__(self) :\n")
			for sa in self.klasses[sk].attributes.keys() :
				attr = self.klasses[sk].attributes[sa]
				value = attr[2] == "" and "None" or attr[2]
				f.write("\t\tself.%s = %s # %s\n" % (sa, value, attr[0]))
			else :
				f.write("\t\tpass\n")
			for so, op in self.klasses[sk].operations :
				# we only need the parameter names
				pars = "self"
				for p in op[2] :
					pars = pars + ", " + p[0]
				f.write("\tdef %s (%s) :\n" % (so, pars))
				if op[4]: f.write("\t\t\"\"\" %s \"\"\"\n" % op[4])
				f.write("\t\t# returns %s\n" % (op[0], ))
				f.write("\t\tpass\n")
		f.close()
		ObjRenderer.end_render(self)

class CxxRenderer(ObjRenderer) :
	def __init__(self) :
		ObjRenderer.__init__(self)
	def end_render(self) :
		f = open(self.filename, "w")
		f.write("/* generated by dia/codegen.py */\n")
		# declaration
		for sk in self.klasses.keys() :
			f.write ("class %s \n{\n" % (sk,))
			k = self.klasses[sk]
			# first sort by visibility
			ops = [[], [], [], []]
			for so, (t, v, p, i, c, s) in k.operations :
				ops[v].append((t,so,p))
			vars = [[], [], [], []]
			for sa, (t, vi, va) in k.attributes.iteritems() :
				vars[vi].append((t, sa))
			visibilities = ("public:", "private:", "protected:", "/* implementation: */")
			for v in [0,2,1,3] :
				if len(ops[v]) == 0 and len(vars[v]) == 0 :
					continue
				f.write ("%s\n" % visibilities[v])
				for op in ops[v] :
					# detect ctor/dtor
					so = ""
					if sk == op[1] or ("~" + sk) == op[1] :
						so = "\t%s (" % (op[1])
					else :
						so = "\t%s %s (" % (op[0], op[1])
					f.write (so)
					# align parameters with the opening brace
					n = len(so)
					i = 0
					m = len(op[2]) - 1
					for p in op[2] :
						linefeed = ",\n\t" + " " * (n - 1) 
						if i == m :
							linefeed = ""
						f.write ("%s %s%s" % (p[1], p[0], linefeed))
						i = i + 1
					f.write(");\n")
				for var in vars[v] :
					f.write("\t%s %s;\n" % (var[0], var[1]))
			f.write ("};\n\n")
		# implementation
		# ...
		f.close()
		ObjRenderer.end_render(self)

class JavaRenderer(ObjRenderer) :
	def __init__(self) :
		ObjRenderer.__init__(self)
		
	def end_render(self) :
		f = open(self.filename, "w")
		
		visibilities = {0:"public", 2:"private", 1:"protected"}

		for name, klass in self.klasses.iteritems() :
			if klass.inheritance_type == "template": classtype = "interface"
			elif klass.inheritance_type == "abstract": classtype = "abstract class"
			else: classtype = "class"
			f.write ("%s %s" % (classtype, name))
			if klass.parents:
				f.write (" extends %s" % klass.parents[0])
			if klass.templates:
				f.write (" implements %s" % ", ".join(klass.templates))
			f.write(" {\n")
			
			for attrname, (type, visibility, value) in klass.attributes.iteritems() :
				if visibility in visibilities:
					vis = visibilities[visibility]+" "
				else: vis = ""
				f.write("\t%s%s %s" % (vis, type, attrname))
				if value != "": f.write(" = %s" % value)
				f.write(";\n")
			
			if not klass.inheritance_type == "template":
				f.write ("\n\tpublic %s() {\n\t\t\n\t}\n\n" % name)
			
			# We should automatic implement abstract parrent and interface methods
			parmethods = []
			if klass.parents:
				parmethods = [(n,m[:3]+(1,)+m[4:]) for n,m in \
						self.klasses[klass.parents[0]].operations if m[3] == 0]
			for template in klass.templates:
				parmethods.extend(self.klasses[template].operations)
			
			
			for pName, pMethod in parmethods:
				pTypes = [p[1] for p in pMethod[2]]
				for name, pars in [(n,m[2]) for n,m in klass.operations]:
					types = [p[1] for p in pars]
					if pars == pMethod[2] and types == pTypes:
						break
				else: klass.operations.append((pName,pMethod))
			
			for methodname, method in klass.operations :
				if method[4]: f.write("\t/** %s */\n" % method[4])
				# if there are no parameter names, something else should appear
				pars = []
				v = ord("a")
				for name, type in method[2]:
					if not name:
						pars.append((type,chr(v)))
						v += 1
					else: pars.append((type,name))
				pars = ", ".join([type+" "+name for type, name in pars])
				
				vis = method[1] in visibilities and visibilities[method[1]] or ""
				returntype = method[0]=="" and "void" or method[0]
				inheritance_type = method[3]==0 and "abstract " or ""
				static = method[4] and "static " or ""
				f.write("\t%s %s%s%s %s (%s)" % (vis, static, inheritance_type, returntype, methodname, pars))
				if klass.inheritance_type == "template" or method[3]==0:
					f.write(";\n\n")
				else: f.write(" {\n\t\t\n\t}\n\n")
			f.write ("}\n\n")
		f.close()
		ObjRenderer.end_render(self)

# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_export ("PyDia Code Generation (Python)", "py", PyRenderer())
dia.register_export ("PyDia Code Generation (C++)", "cxx", CxxRenderer())
dia.register_export ("PyDia Code Generation (Java)", "java", JavaRenderer())
