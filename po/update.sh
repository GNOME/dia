#!/bin/sh

xgettext --default-domain=dia --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f dia.po \
   || ( rm -f ./dia.pot \
    && mv dia.po ./dia.pot )
