#!/bin/sh

set -ex

cd `dirname $0`

if [ ! -e .uncrustify/build/uncrustify ]; then
 ./build-uncrustify.sh
fi

SRC_FILES=$(find . -name "*.[ch]" ! -path "./.uncrustify/*" )

.uncrustify/build/uncrustify -c .uncrustify.cfg --no-backup -l C --replace ${SRC_FILES}
