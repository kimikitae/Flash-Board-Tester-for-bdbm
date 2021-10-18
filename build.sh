#!/bin/sh

(cd project; make -j$(nproc))

if [ -n "$1" ] && [ $1 = "install" ]
then
  (cd project; sudo make install)
fi
