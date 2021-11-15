#!/bin/sh -xe
for lib in Imlib2 libspng zip curl; do
    if  echo "int main(){}" | ${CC:-cc} "-l$lib" -o /dev/null -x c - 2>/dev/null ; then
        echo "LDFLAGS += -l${lib}";
    else
        echo "CXXFLAGS += NO_${lib}_LOADER" | tr '[:lower:]' '[:upper:]' ;
    fi
done > auto_detect.mk
