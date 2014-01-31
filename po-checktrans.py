#!/usr/bin/python
#
# This quick hack gives translation statistics (from the core translation
# files).
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

import string,os,sys,math

collen = 1

def NoneStr(n):
    if n is None: return (" " * maxlanglen)
    return n

def slurp_po(fname):
    """returns a tuple (language_name,ids,translations) of what has
    been translated."""
    base,ext = os.path.splitext(os.path.basename(fname))
    language_name = base

    idents = 0
    translations = 0
    
    st = ""
    id = ""

    fuzzy = 0
    prev_fuzzy = 0
    fuzzies = 0
    
    fd = open(fname,"r")
    while 1:
        s = fd.readline()
        if not s: break

        if s[0] == '#':
            if s.find('fuzzy') >= 0:
                fuzzy = 1
                
        s = string.strip(string.split(s,'#')[0])
        if not s: continue # empty or comment line.
        
        sl = string.split(s)
        if sl[0] == "msgid":
            #print "id",id,"st",st
            if st:
                if prev_fuzzy:
                    fuzzies = fuzzies + 1
                translations = translations + 1
            id = sl[0]
            st = ""
            del sl[0]
        elif sl[0] == "msgstr":
            #print "id",id,"st",st
            if st:
                idents = idents + 1
            id = sl[0]
            st = ""
            del sl[0]
            prev_fuzzy = fuzzy
            fuzzy = 0
            
        for k in sl:
            if k != '""':
                st = st + k
    #print "translations:",translations,"idents:",idents
    return (language_name, idents, translations,fuzzies)

if len(sys.argv)<3:
    print "Usage: %s <package.pot> <lang.po> ..." % sys.argv[0]
    print
    print " <package.pot>: file name of the identifier reference to check."
    print " <lang.po>: file name of the translation to check"
    sys.exit(1)

def maxlen(a,b):
    if len(a) > len(b):
        return a
    return b

(t,idents,n,f) = slurp_po(sys.argv[1])
del t,n,f

translations = map(slurp_po,sys.argv[2:])
trans = map(lambda (l,i,t,f),ti=idents:
            "%s:%3d%%(%d/%d/%d)%s"%(l,
                                 100*float(t)/idents,
                                 t,f,idents,
                                 "*" * (idents != i)),
            translations)
maxlanglen = len(reduce(maxlen,trans,""))
trans = map(lambda s,mll=maxlanglen: string.ljust(s,mll),trans)

collen = maxlanglen + len("  ")

numcols = int(79 / collen)
ltnc = (len(trans) / numcols) + (len(trans) % numcols)

cols = []
while trans:
    c,trans = trans[:ltnc],trans[ltnc:]
    cols.append(c)

lines = apply(map,tuple([None]+cols))

result = string.join(map(lambda l:string.join(map(NoneStr,list(l)),"  "),
                         lines),
                     "\n")
print result

