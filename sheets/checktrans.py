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



from xml.sax import saxexts
from xml.sax import saxlib
import sys,string

class CounterHandler(saxlib.DocumentHandler):

    def __init__(self):
        self.elemstk = []
        self.langstk = []
        self.newdoc()
        
    def newdoc(self):
        self.langs = {}
        
    def _countlang(self,name):
        locdct = self.langstk[-1]
        for dct in self.langs,locdct:
            if dct.has_key(name):
                dct[name] = dct[name] + 1
            else:
                dct[name] = 1
        
    def startElement(self,name,attrs):
        #print "start of ",name,attrs,attrs.map        
        self.elemstk.append(name)
        if (name == "sheet") or (name == "object"):
            self.langstk.append({})
        elif (name == "description"):
            attmap = attrs.map
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
            #print "end of",name, res
        else:
            #print "end of ",name
            pass
        
if len(sys.argv)<2:
    print "Usage: %s <sheet.sheet>" % sys.argv[0]
    print
    print " <sheet.sheet>: file name of the sheet to check"
    sys.exit(1)
    
# Load parser and driver

p=saxexts.make_parser()
ch=CounterHandler()
p.setDocumentHandler(ch)

fnames =sys.argv[1:]

def make_langresult(langdict):
    langres = map(lambda s: string.ljust(s,7),
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
        ch.newdoc()
        p.parse(name)
        OK=1

        langres = make_langresult(ch.langs)
        
        sys.stdout.write(("I: %%%ds %%s\n" % namelen) %
                         (name,string.join(langres)))
        for (k,v) in ch.langs.items():
            if globlangs.has_key(k):
                globlangs[k] = globlangs[k] + v
            else:
                globlangs[k] = v
                
    except IOError,e:
        sys.stderr.write("E: %s: %s\n" % (name,str(e)))
    except saxlib.SAXException,e:
        sys.stderr.write("E: %s\n" % str(e))

langres = make_langresult(globlangs)        

sys.stdout.write("\nI: OVERALL: %s\n" % string.join(langres))


