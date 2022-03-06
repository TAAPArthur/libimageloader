#!/bin/sh -e

{
cat img_loader.h
echo "#ifdef IMG_LIB_IMPLEMENTATION"
cat img_loader.c
} > single_header.h
sed -n 's/^#include "\(.*h\)"/\1/p' img_loader.c | while read -r name; do
    sed -i "/^#include \"$name\"/r $name" single_header.h
done
sed -i '/^#include "\(.*h\)"/d' single_header.h

echo "#endif" >> single_header.h
