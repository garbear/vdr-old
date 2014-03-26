#!/bin/bash

PLATFORM="$1"
DIR=`dirname $0`

if [ -z "$PLATFORM" ]; then
  echo "Usage: $0 [platform]"
  exit 1
fi

if [ ! -f "$DIR/../build/libXBMC_VDR.so" ]; then
  echo "Please build the library first"
  exit 2
fi

mkdir -p "$DIR/../build/addon/service.vdr"
sed "s/@PLATFORM@/$PLATFORM/g" "$DIR/addon.xml.in" > "$DIR/../build/addon/service.vdr/addon.xml"
cp -f "$DIR/../build/libXBMC_VDR.so" "$DIR/../build/addon/service.vdr/libXBMC_VDR.so"
cd "$DIR/../build/addon" && zip -r "../service.vdr-$PLATFORM.zip" service.vdr

exit $?

