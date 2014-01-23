#!/bin/sh

cp src/libting.* /usr/lib

#copy header files

incDir=/usr/include/ting
mkdir -p $incDir
cp src/ting/*.hpp $incDir
cp src/ting/*.h $incDir

mkdir -p $incDir/fs
cp src/ting/fs/*.hpp $incDir/fs

mkdir -p $incDir/net
cp src/ting/net/*.hpp $incDir/net

mkdir -p $incDir/mt
cp src/ting/mt/*.hpp $incDir/mt
