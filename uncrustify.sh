#!/bin/sh

set -ex

cd `dirname $0`

SRC_FILES=$(find . -name *.[ch])

uncrustify -c .uncrustify.cfg --no-backup -l C --replace ${SRC_FILES}
