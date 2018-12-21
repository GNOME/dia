#!/usr/bin/python
#
# This quick hack gives translation statistics about the various sheets. 
#
# Copyright (C) 2001, Cyrille Chepelov <chepelov@calixo.net>
#
# This quick hack is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This quick hack is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this quick hack; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#


import os,sys,string
print("""*** Sheet translation report: ***
(Absence of a language code means 0% translation for that language)
(Help for translations (and/or much more) is of course welcome !)
""")

try :
	from xml.sax import saxexts
	from xml.sax import saxlib
except ImportError :
	print "Missing dependencies: no translation report"
	print("""E: checktrans failed to run. Please check that python and python-xml are installed
E: on your system. On some systems, python-xml is called PyXML. If in doubt,
E: have a look at http://pyxml.sourceforge.net
""")
	sys.exit(1)

class CounterHandler(saxlib.DocumentHandler):

    def __init__(self):
        self.elemstk = []
        self.langstk = []
        self.namestk = []

    def setDocumentLocator(self,locator):
        self.locator = locator
        saxlib.DocumentHandler.setDocumentLocator(self,locator)
        self.langs = {}
        self.namelangs = {}

    def warning(self,message):
        print "W:%s:L%d:C%d: %s" % (self.locator.getSystemId(),
                                    self.locator.getLineNumber(),
                                    self.locator.getColumnNumber(),
                                    message)
    def _countlang(self,name):
        locdct = self.langstk[-1]
        if locdct.has_key(name):
            self.warning("duplicate description for %s, language code %s" % \
                         (self.namestk[-1],name))
            locdct[name] = locdct[name] + 1
        else:
            locdct[name] = 1
            
        if self.langs.has_key(name):
            self.langs[name] = self.langs[name] + 1
        else:
            self.langs[name] = 1                
        
    def _countnamelang(self,name):
        if self.namelangs.has_key(name):
            self.warning("duplicate name for sheet, language code %s" % name)
            self.namelangs[name] = self.namelangs[name] + 1
        else:
            self.namelangs[name] = 1
            
        if self.langs.has_key(name):
            self.langs[name] = self.langs[name] + 1
        else:
            self.langs[name] = 1                
        
    def startElement(self,name,attrs):
        #print "start of ",name,attrs,attrs.map        
        attmap = attrs.map
        self.elemstk.append(name)
        if (name == "sheet") or (name == "object"):
            self.langstk.append({})
            if attmap.has_key('name'):
                name = 'Object "%s"' % attmap['name']
            else:
                name = 'Sheet "%s"' % self.locator.getSystemId()
            self.namestk.append(name)
        elif (name == "name"):
            if attmap.has_key("xml:lang"):
                lang = attmap["xml:lang"]
            else:
                lang = ""
            self._countnamelang(lang)
        elif (name == "description"):
            if attmap.has_key("xml:lang"):
                lang = attmap["xml:lang"]
            else:
                lang = ""
            self._countlang(lang)
        
    def endElement(self,name):
        popped = self.elemstk.pop()
        if popped != name:
            raise Exception("stack error somewhere...")
        if (name == "sheet") or (name == "object"):
            res = self.langstk.pop()
            self.namestk.pop()
            #print "end of",name, res
        else:
            #print "end of ",name
            pass

##  class BasicEntityResolver(saxlib.EntityResolver):
##      def resolveEntity(name,publicId,systemId):
##          print "D:resolveEntity(%s,%s,%s)" % (name,publicId,systemId),
##          if publicId == "-//FOO//BAR//EN":
##              res = "file:../doc/sheet.dtd"
##          else:
##              res = saxlib.EntityResolver.resolveEntity(name,publicId,systemId)
        
##          print "--> res",res,type(res)
    
##          return res
    
if len(sys.argv)<2:
    print "Usage: %s <sheet.sheet>" % sys.argv[0] 
    print
    print " <sheet.sheet>: file name of the sheet to check"
    sys.exit(1)
    
# Load parser and driver

p=saxexts.make_parser()
#p=saxexts.XMLValParserFactory.make_parser()
ch=CounterHandler()
p.setDocumentHandler(ch)
#p.setEntityResolver(BasicEntityResolver())
    
fnames =sys.argv[1:]

def make_langresult(langdict,colsize):
    langres = map(lambda s,colsize=colsize: string.ljust(s,colsize),
                  map(lambda (cc,count),total=langdict['']: 
                      ("%s:%d%%" % (cc,100*count/total)),
                      filter(lambda (cc,count): cc, langdict.items())))
    langres.sort()
    return langres

def maxlen(a,b):
    if len(a)>len(b): return a
    return b
namelen = len(reduce(maxlen,fnames,""))

globlangs = {}

for name in fnames:
    OK=0
    try:
        p.parse("file://localhost" + os.getcwd() + "/" + name)
        OK=1

        maxlanglen = len(reduce(maxlen,ch.langs.keys(),""))
        langres = make_langresult(ch.langs,maxlanglen+5)
        columns = (79 - (4 + namelen)) / (maxlanglen+5)        
        #print "maxlanglen = ",maxlanglen,"columns=",columns

        langres1,langrest = langres[:columns],langres[columns:]
        sys.stdout.write(("I: %%%ds %%s\n" % namelen) %
                         (name,string.join(langres1)))
        while langrest:
            langresn,langrest = langrest[:columns],langrest[columns:]
            sys.stdout.write("I: %s %s\n" % (' ' * namelen,
                                             string.join(langresn)))
            
        for (k,v) in ch.langs.items():
            if globlangs.has_key(k):
                globlangs[k] = globlangs[k] + v
            else:
                globlangs[k] = v
                
    except IOError,e:
        sys.stderr.write("E: %s: %s\n" % (name,str(e)))
    except saxlib.SAXException,e:
        sys.stderr.write("E: %s\n" % str(e))

maxlanglen = len(reduce(maxlen,globlangs.keys(),""))
langres = make_langresult(globlangs,maxlanglen+5)        
columns = (79 - 12) / (maxlanglen+5)        

langres1,langrest = langres[:columns],langres[columns:]
sys.stdout.write("\nI: OVERALL: %s\n" % string.join(langres1))
while langrest:
    langresn,langrest = langrest[:columns],langrest[columns:]
    sys.stdout.write("I: %s %s\n" % (' ' * 8,
                                     string.join(langresn)))


