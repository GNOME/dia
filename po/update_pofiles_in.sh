#!/bin/sh

find ../ -name "*.c" | xargs grep _\( | cut -d: -f1 | uniq | cut -d/ -f2- > POTFILES.in
