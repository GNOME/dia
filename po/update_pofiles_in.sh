#!/bin/sh

INTLDIRS=../app
for DIR in $INTLDIRS; do
  grep _\( $DIR/*.c | cut -d: -f1 | uniq | cut -d/ -f2- > POTFILES.in
done
