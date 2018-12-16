#!/usr/bin/env sh

# If git is installed and we are in a .git folder, print the current SHA.
# Otherwise, print nothing.

#TODO 1: check that this runs in a non-git folder
#TODO 2: check that this runs on a machine without git installed

# Need to ensure nothing is output.
cd $MESON_SOURCE_ROOT 2>&1 > /dev/null
command -v git > /dev/null && git rev-parse --short=10 HEAD 2>/dev/null || echo ""

