#!/bin/sh

../intltool-update -m

exit 0

# This stuff is obsolete !


cp POTFILES.in POTFILES.in.bak

find ../ -name "*.c" | grep -v /EML/ | xargs grep _\( | cut -d: -f1 | uniq | cut -d/ -f2- > POTFILES.in.new
find ../ -name "*.sheet.in" |uniq| cut -d/ -f2- >> POTFILES.in.new
echo dia.desktop.in >> POTFILES.in.new
cat POTFILES.in.new | sort > POTFILES.in
