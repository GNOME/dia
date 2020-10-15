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

import gettext
_ = gettext.gettext

class Klass :
	def __init__ (self, name) :
		self.name = name
		# use a list to preserve the order
		self.attributes = []
		# a list, as java/c++ support multiple methods with the same name
		self.operations = []
		self.comment = ""
		self.parents = []
		self.templates = []
		self.inheritance_type = ""
	def AddAttribute(self, name, type, visibility, value, comment, class_scope) :
		self.attributes.append ((name, (type, visibility, value, comment, class_scope)))
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

##
# \brief Base class of all the code generators
# \extends _DiaPyRenderer
# \ingroup PyDia
class ObjRenderer :
	"Implements the Object Renderer Interface and transforms diagram into its internal representation"
	def __init__ (self) :
		# an empty dictionary of classes
		self.klasses = {}
		self.arrows = []
		self.filename = ""

	def begin_render (self, data, filename) :
		self.filename = filename
		# not only reset the filename but also the other state, otherwise we would accumulate information through every export
		self.klasses = {}
		self.arrows = []
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
							# (name, type, value, comment, kind)
							params.append((par[0], par[1], par[2], par[3], par[4]))
						k.AddOperation (op[0], op[1], op[4], params, op[5], op[2], op[7])
					#print o.properties["attributes"].value
					for attr in o.properties["attributes"].value :
						# see objects/UML/umlattributes.c:umlattribute_props
						#print "\t", attr[0], attr[1], attr[4]
						# name, type, value, comment, visibility, abstract, class_scope
						k.AddAttribute(attr[0], attr[1], attr[4], attr[2], attr[3], attr[6])
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
		self.attributes = []
		self.operations = []

##
# \brief Generate a Python source file from an UML class diagram
class PyRenderer(ObjRenderer) :
	def __init__(self) :
		ObjRenderer.__init__(self)
	def end_render(self) :
		f = open(self.filename, "w")
		for sk in list(self.klasses.keys()) :
			parents = self.klasses[sk].parents + self.klasses[sk].templates
			if not parents:
				f.write ("class %s :\n" % (sk,))
			else:
				f.write ("class %s (%s) :\n" % (sk,", ".join(parents)))
			k = self.klasses[sk]
			if len(k.comment) > 0 :
				f.write ("\t'''" + k.comment + "'''\n")
			f.write ("\tdef __init__(self) :\n")
			for sa, attr in k.attributes :
				value = attr[2] == "" and "None" or attr[2]
				f.write("\t\tself.%s = %s # %s\n" % (sa, value, attr[0]))
			else :
				f.write("\t\tpass\n")
			for so, op in k.operations :
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

##
# \brief Generate C++ source files from an UML class diagram
class CxxRenderer(ObjRenderer) :
	def __init__(self) :
		ObjRenderer.__init__(self)
	def end_render(self) :
		f = open(self.filename, "w")
		f.write("/* generated by dia/codegen.py */\n")
		# declaration
		for sk in list(self.klasses.keys()) :
			k = self.klasses[sk]
			if len(k.comment) > 0 :
				f.write ("/*" + k.comment + "*/\n")
			if len(k.parents) > 0 :
				f.write ("class %s : %s \n{\n" % (sk, ", ".join(k.parents)))
			else :
				f.write ("class %s \n{\n" % (sk,))
			# first sort by visibility
			ops = [[], [], [], []]
			for so, (t, v, p, i, c, s) in k.operations :
				ops[v].append((t,so,p,i,c))
			vars = [[], [], [], []]
			for sa, (t, vi, va, vc, class_scope) in k.attributes :
				#TODO: use 'va'=value 'vc'=comment
				vars[vi].append((t, sa))
			visibilities = ("public:", "private:", "protected:", "/* implementation: */")
			for v in [0,2,1,3] :
				if len(ops[v]) == 0 and len(vars[v]) == 0 :
					continue
				f.write ("%s\n" % visibilities[v])
				for op in ops[v] :
					if op[4] != "" :
						f.write ('\t/* ' + op[4] + ' */\n')
					inh = ""
					if op[3] < 2 :
						inh = "virtual "
					# detect ctor/dtor
					so = ""
					if sk == op[1] or ("~" + sk) == op[1] :
						so = "\t%s%s (" % (inh, op[1])
					else :
						so = "\t%s%s %s (" % (inh, op[0], op[1])
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
					if op[3] == 0 : # abstract ?= pure virtual?
						f.write(") = 0;\n")
					else :
						f.write(");\n")
				for var in vars[v] :
					f.write("\t%s %s;\n" % (var[0], var[1]))
			f.write ("};\n\n")
		# implementation
		# ...
		f.close()
		ObjRenderer.end_render(self)

##
# JsRenderer: export Dia UML diagram to Object style ECMA5 javascript code.
#
# Features:
#  - All methods and attributes are public.
#  - Inheriting from one class.
#  - Comments for classes, attributes and operations are supported.
#  - Method parameter default values are supported.
#  - Order of attributes and operations is preserved.
#  - Auto generate type check.
#
class JsRenderer(ObjRenderer) :
	def __init__(self) :
		ObjRenderer.__init__(self)
	def end_render(self) :
		f = open(self.filename, "w")
		f.write("/* generated by dia/codegen.py */\n\n")
		for sk in list(self.klasses.keys()) :
			f.write("// class definition : %s\n" % (sk, ))
			parents = self.klasses[sk].parents
			#add inheritance
			if parents:
				f.write("// Inherits from %s.\n" % (parents[0], ))
				f.write("%s.prototype = new %s();\n" % (sk, parents[0]))
				f.write("%s.prototype.constructor = %s;\n\n" % (sk, sk))
			f.write ("function %s () {\n" % (sk,))
			k = self.klasses[sk]
			if len(k.comment) > 0 :
				f.write ("\n\t/*" + k.comment + "*/\n")
			#use strict
			f.write("\t\"use strict\";\n")
			if k.attributes:
				f.write("\n")
			#all attributes are public
			for sa, attr in k.attributes :
				value = attr[2] == "" and "null" or attr[2]
				f.write("\tthis.%s = %s;\n" % (sa, value))
				#expected types checks (constructor)
				if attr[0]:
					f.write("\tif (typeof(this.%s) !== '%s') {\n" % (sa, attr[0]));
					f.write("\t\tthrow \"Unexpected type '\"+typeof(this.%s)+\"' for attribute this.%s.\";\n" % (sa, sa))
					f.write("\t}\n")
			f.write("}\n\n")
			#operations are defined out of class definition
			#return the first item of given p (tuple) ie : parameter name
			def p_name(p):
				return p[0]
			for so, op in k.operations :
				# only parameter names needed
				pars = ", ".join(map(p_name,op[2]))
				f.write("%s.prototype.%s = function(%s) {\n" % (sk, so, pars))
				if op[4]: f.write("\t/* %s */\n" % op[4])
				f.write("\t\"use strict\";\n\n")
				for p in op[2]:
					#default values :
					if len(p[2]) > 0 :
						t = (p[0], p[0], p[2])
						f.write("\tif (%s === undefined) {\n\t%s = %s;\n\t}\n" % t)
					#expected types checks
					if len(p[3]) > 0 :
						f.write("\tif (typeof(%s) !== '%s') {\n" % (p[0], p[3]));
						f.write("\t\tthrow \"Unexpected type '\"+typeof(%s)+\"' for parameter %s.\";\n" % (p[0], p[0]))
						f.write("\t}\n");
				f.write("\t// returns %s\n" % (op[0], ))
				f.write("\treturn ;\n")
				f.write("};\n\n")
		f.close()
		ObjRenderer.end_render(self)

##
# PascalRenderer: export Dia UML diagram to Object Pascal (Free Pascal, Delphi)
#
# Please follow some "drawing guidelines" and "naming conventions" so that the
# exporter can do its job.
#  - Use "UML - Generalization" arrows for class inheritance.
#  - Use "UML - Realizes" arrows when implementing an interface.
#  - Set a class to be "abstract" to denote it is an interface definition.
#  - Set Inheritance Type to "abstract" for 'virtual; abstract;' methods, set
#    it to "virtual" for 'virtual;' methods.
#  - Array fields are automatically recognized. If the name ends with "[]"
#    an 'Array of' is used. If the name uses a number with "[1234]", an
#    'Array[0..1233] of' is used. If the name uses a constant with
#    "[MaxEntries]" an 'Array[0..MaxEntries-1] of' is written.
#  - To inherit from classes which are not drawn (e.g. LCL/VCL classes),
#    name then class with the parent class in paranthesis (e.g.
#    "TMainWin(TForm)"
#
# Features
#  - Inheriting from one class and implementing multiple interfaces is
#    supported.
#  - Comments for classes, attributes and operations are supported. They are
#    put in the line before the method declaration with a '///' style comment
#    (Doxygen-like).
#  - Method parameter directions are supported (-> 'Var', 'Out').
#  - Method parameter default values are supported.
#  - 'Function' and 'Procedure' are automatically recognized by whether a
#    return type exists or not.
#  - order of classes is alphabetically
#  - the order of attributes and operations is preserved
#  - Prints a list of forward declarations of all classes at the beginning
#    to avoid declaration order problems.
#
# TODO:
#  - Automatically use the keyword "override" instead of "virtual" in
#    descendant classes.
#  - Automatically define 'Properties'. Unfortunately the UML standard
#    doesn't support this and so the Dia dialog has no option to specify
#    this. So a "code" has to be used.
#  - Mark/recognize Constructors and Destructors
#  - Write comments for method parameters (e.g. by using a big doxygen
#    comment '(** ... *)' before the method)
#  - Use "Packages" to split the classes in separate 'Unit's.
#  - Beautify and comment the export code. Using arrays with "magic number
#    indexes" for certain fields is bad and tedious to work with.
#  - Support defining global constants.
#  - Support defining global types (especially for enums, arrays,
#    records, ...).
#  - Apply some sanity checks to the UML diagram:
#     - multiple inheritance is forbidded
#     - if implementing an interface, all required methods must be
#       implemented; alternative: just put all methods there, so the UML
#       drawer doesn't have to write them twice
#     - visibility for all methods of an interfaces must be "public"
#     - don't write the visibility specifier 'public' for interfaces
#     - no "Attributes" for interface definitions, but properties are
#       allowed
#     - default values for method parameters must be the last parameters
class PascalRenderer(ObjRenderer) :
	def __init__(self) :
		ObjRenderer.__init__(self)
	def end_render(self) :
		f = open(self.filename, "w")
		f.write("/* generated by dia/codegen.py */\n")
		f.write("Type\n")
		# classes
		class_names = list(self.klasses.keys())
		class_names.sort()
		# forward declarations of all classes
		for sk in class_names :
			k = self.klasses[sk]
			# class declaration
			if k.inheritance_type == "abstract" :
				f.write ("  %s = interface;\n" % (sk))
			else :
				f.write ("  %s = class;\n" % (sk))
		f.write("\n");
		# class declarations
		for sk in class_names :
			k = self.klasses[sk]
			# comment
			if len(k.comment) > 0 :
				f.write("  /// %s\n" % (k.comment))
			# class declaration
			if k.inheritance_type == "abstract" :
				f.write ("  %s = interface" % (sk))
			else :
				f.write ("  %s = class %s" % (sk, k.inheritance_type))
			# inherited classes / implemented interfaces
			p = []
			if k.parents :
				p.append(k.parents[0])
			if k.templates :
				p.append(",".join(k.templates))
			if len(p) > 0 :
				f.write("(%s)" % ",".join(p))
			f.write ("\n")
			# first sort by visibility
			ops = [[], [], [], [], [], []]
			for op_name, (op_type, op_visibility, op_params, op_inheritance, op_comment, op_class_scope) in k.operations :
				ops[op_visibility].append((op_type, op_name, op_params, op_comment, op_inheritance))
			vars = [[], [], [], []]
			for var_name, (var_type, var_visibility, var_value, var_comment, class_scope) in k.attributes :   # name, type, visibility, value, comment
				vars[var_visibility].append((var_type, var_name, var_value, var_comment))
			visibilities = ("public", "private", "protected", "/* implementation */")
			for v in [1,2,0,3] :
				if len(ops[v]) == 0 and len(vars[v]) == 0 :
					continue
				# visibility prefix
				f.write ("  %s\n" % visibilities[v])
				# variables
				for var in vars[v] :
					# comment
					if len(var[3]) > 0 :
						f.write ("    /// %s\n" % var[3])
					if var[1].endswith("]") :
						# array; check if this is dynamic or with defined size
						i = var[1].find("[")
						varname = var[1]
						arraysize = varname[i+1:-1]
						varname = varname[:i]
						if len(arraysize) > 0 :
							# array with defined size
							if arraysize.find("..") > 0 :
								f.write("    %s : Array[%s] of %s;\n" % (varname, arraysize, var[0]))
							elif arraysize.isdigit() :
								arraysize = int(arraysize)-1
								f.write("    %s : Array[0..%d] of %s;\n" % (varname, arraysize, var[0]))
							else :
								f.write("    %s : Array[0..%s-1] of %s;\n" % (varname, arraysize, var[0]))
						else :
							# dynamic size
							f.write("    %s : Array of %s;\n" % (varname, var[0]))
					else :
						# normal variable
						f.write("    %s : %s;\n" % (var[1], var[0]))
				# operations
				for op in ops[v] :
					if len(op[3]) > 0 :
						f.write ("    /// %s\n" % op[3])
					if len(op[0]) == 0 :
						f.write ("    Procedure %s" % op[1])
					else :
						f.write ("    Function %s" % op[1])
					if len(op[2]) > 0 :
						f.write ("(")
						i = 0
						m = len(op[2]) - 1
						for p in op[2] :
							if p[4] == 2 :
								f.write ("Out ")
							elif p[4] == 3 :
								f.write ("Var ")
							f.write ("%s:%s" % (p[0], p[1]))
							if len(p[2]) > 0 :
								f.write (":=%s" % p[2])
							if i != m :
								f.write(";")
							i = i + 1
						f.write (")")
					if len(op[0]) == 0 :
						f.write(";")
					else :
						f.write (" : %s;" % op[0])
					# inheritance type
					if op[4] == 0 :
						f.write (" virtual; abstract;");
					elif op[4] == 1 :
						f.write (" virtual;");
					f.write ("\n")
			f.write ("  End;\n\n")
		# implementation
		# ...
		f.close()
		ObjRenderer.end_render(self)

##
# JavaRenderer: export Dia UML diagram to Java
#
# improved by: Manuel Arguelles
#
# Features:
#	* Comments for classes, attributes, methods and methods parameters are
#	  supported. Uses a comment style javaDoc-like. The comments are
#	  divided into lines if they exceed 79 characters.
#	* Splits the classes in separate files. Creates a file that contains
#	  the dia/codegen.py firm :)
#
# Fixes:
#	* Visibilities "private" and "protected" were reversed.
#	* Comments for classes, attributes, methods and methods parameters.
#	* Comments are divided into lines if they exceed 79 characters.
#	* Splits the classes in separate files.
#	* Not write "NULL" in empty comments.
#	* Writes the default values of the arguments of the methods.
#	* Makes public all the classes and interfaces.
#	* Not write "static" in all the methods.
#
class JavaRenderer(ObjRenderer) :
	def __init__(self) :
		ObjRenderer.__init__(self)

	def end_render(self) :
		visibilities = {0:"public", 1:"private", 2:"protected"}

		mainfile = open(self.filename, "w")
		mainfile.write("/* Generated by dia/codegen.py\n *\n * Generated files:\n")
		for name, klass in self.klasses.items() :
			# splits the classes in separate files
			classfile = self.filename[:self.filename.rfind("/")+1] + name.capitalize() + ".java"
			f = open(classfile, "w")
			# class comment
			f.write("/**\n")
			if len(klass.comment) > 1:
				for l in range(len(klass.comment)/77+1):
					f.write(" * %s\n" % klass.comment[l*77:(l+1)*77])
			f.write(" * @author\n *\n */\n")

			if klass.inheritance_type == "template": classtype = "public interface"
			elif klass.inheritance_type == "abstract": classtype = "public abstract class"
			else: classtype = "public class"
			f.write ("%s %s" % (classtype, name))
			if klass.parents:
				f.write (" extends %s" % klass.parents[0])
			if klass.templates:
				f.write (" implements %s" % ", ".join(klass.templates))
			f.write(" {\n")

			# attributes
			for attrname, (type, visibility, value, comment, class_scope) in klass.attributes :
				if comment:
					f.write("\t/**\n")
					for l in range(len(comment)/73+1):
						f.write("\t * %s\n" % comment[l*73:(l+1)*73])
					f.write("\t */\n")
				if visibility in visibilities:
					vis = visibilities[visibility]+" "
				else: vis = ""
				f.write("\t%s%s %s" % (vis, type, attrname))
				if value: f.write(" = %s" % value)
				f.write(";\n")

			# dafault constructor
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
				# method comment
				f.write("\t/**\n")
				if method[4]:
					for l in range(len(method[4])/73+1):
						f.write("\t * %s\n" % method[4][l*73:(l+1)*73])
				if method[2]:
					for param in method[2]:
						f.write("\t * @param %s\n" % param[0]) #param[0]->name
				f.write("\t * @return %s\n\t */\n" % (method[0]=="" and "void" or method[0]))
				# if there are no parameter names, something else should appear
				pars = []
				v = ord("a")
				for name, type, value, comment, kind in method[2]:
					#TODO: also use: kind
					if not name:
						name = chr(v)
						v += 1
					if value:
						value = "=" + value
					pars.append((type, name, value))
				pars = ", ".join([type+" "+name+value for type, name, value in pars])

				vis = method[1] in visibilities and visibilities[method[1]] or ""
				return_type = method[0] == "" and "void" or method[0]
				inheritance_type = method[3] == 0 and "abstract " or ""
				# this is wrong!
				#static = method[4] and "static " or ""
				static = ""
				f.write("\t%s %s%s%s %s(%s)" % (vis, static, inheritance_type, \
												return_type, methodname, pars))
				if klass.inheritance_type == "template" or method[3] == 0:
					f.write(";\n\n")
				else:
					f.write(" {\n\t\t// TODO Auto-generated method stub\n\t}\n\n")
			f.write ("}\n\n")
			f.close()
			mainfile.write(" *\t%s\n" % classfile)
		mainfile.write(" */\n")
		mainfile.close()
		ObjRenderer.end_render(self)

##
# PhpRenderer: export Dia UML diagram to PHP
#
# Added by: Steven Garcia
#
# This is similar to the Java renderer except for PHP
# Added class scope (static) support
class PhpRenderer(ObjRenderer) :
	def __init__(self) :
		ObjRenderer.__init__(self)

	def end_render(self) :
		visibilities = {0:"public", 1:"private", 2:"protected"}

		mainfile = open(self.filename, "w")
		mainfile.write("<?php\n/* Generated by dia/codegen.py\n *\n * Generated files:\n")

		for name, klass in self.klasses.items() :
			# splits the classes in separate files
			classfile = self.filename[:self.filename.rfind("/")+1] + name + ".php"
			f = open(classfile, "w")
			# class comment
			f.write("<?php\n/**\n")
			if len(klass.comment) > 1:
				for l in range(len(klass.comment)/77+1):
					f.write(" * %s\n" % klass.comment[l*77:(l+1)*77])
			f.write(" * @author\n *\n */\n")

			if klass.inheritance_type == "template": classtype = "interface"
			elif klass.inheritance_type == "abstract": classtype = "abstract class"
			else: classtype = "class"
			f.write ("%s %s" % (classtype, name))
			if klass.parents:
				f.write (" extends %s" % klass.parents[0])
			if klass.templates:
				f.write (" implements %s" % ", ".join(klass.templates))
			f.write("\n{\n")

			# attributes
			for attrname, (type, visibility, value, comment, class_scope) in klass.attributes :
				if comment:
					f.write("\t/**\n")
					for l in range(len(comment)/73+1):
						f.write("\t * %s\n" % comment[l*73:(l+1)*73])
					f.write("\t */\n")
				if visibility in visibilities:
					vis = visibilities[visibility]
				else: vis = ""
				static = class_scope and "static " or ""
				f.write("\t%s%s $%s" % (static, vis, attrname))
				if value: f.write(" = %s" % value)
				f.write(";\n")

			# We should automatic implement abstract parent and interface methods
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
				# method comment
				f.write("\t/**\n")
				if method[4]:
					for l in range(len(method[4])/73+1):
						f.write("\t * %s\n" % method[4][l*73:(l+1)*73])
				if method[2]:
					for param in method[2]:
						f.write("\t * @param %s\n" % param[0]) #param[0]->name
				f.write("\t * @return %s\n\t */\n" % (method[0]=="" or method[0]))
				# if there are no parameter names, something else should appear
				pars = []
				v = ord("a")
				for name, type, value, comment, kind in method[2]:
					#TODO: also use: kind
					if not name:
						name = chr(v)
						v += 1
					if value:
						value = " = " + value
					pars.append((type, '$' + name, value))
				pars = ", ".join([type+" "+name+value for type, name, value in pars])

				vis = method[1] in visibilities and visibilities[method[1]] or ""
				return_type = method[0] == "" or method[0]
				# TODO inheritance type is set to leaf/final by default in Dia. A normal type is need for the default
				#inheritance_type = method[3] == 0 and "abstract " or method[3] == 2 and "final " or ""
				inheritance_type = method[3] == 0 and "abstract " or ""
				static = method[5] and "static " or ""
				f.write("\t%s%s%sfunction %s(%s)" % (static, vis + ' ', inheritance_type, methodname, pars))
				if klass.inheritance_type == "template" or method[3] == 0:
					f.write(";\n\n")
				else:
					f.write("\n\t{\n\t\t// TODO Auto-generated method stub\n\t}\n\n")
			f.write ("}\n\n")
			f.close()
			mainfile.write(" *\t%s\n" % classfile)
		mainfile.write(" */\n")
		mainfile.close()
		ObjRenderer.end_render(self)

	def draw_line(self, *args):pass
	def draw_string(self, *args):pass

# dia-python keeps a reference to the renderer class and uses it on demand
dia.register_export (_("PyDia Code Generation (Python)"), "py", PyRenderer())
dia.register_export (_("PyDia Code Generation (C++)"), "cxx", CxxRenderer())
dia.register_export (_("PyDia Code Generation (Pascal)"), "pas", PascalRenderer())
dia.register_export (_("PyDia Code Generation (Java)"), "java", JavaRenderer())
dia.register_export (_("PyDia Code Generation (JavaScript)"), "js", JsRenderer())
dia.register_export (_("PyDia Code Generation (PHP)"), "php", PhpRenderer())
