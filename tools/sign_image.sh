#!/bin/bash

if [ "$1" == "" ] || [ "$2" == "" ] || [ "$3" == "" ] || [ "$4" == "" ] || [ $# != 4 ]; then
	echo Usage: $0 \<input-image\> \<private-key\> \<output-image\> \<signature-file\>
	exit 0
fi

if [[ `xxd -p -l 12 $1` == "5aa5a55a00f80f007d24a3ef" ]]; then
	echo "$(basename $0): Image contains debug header." >&2
	exit 1
fi

echo Preparing output image...
openssl dgst -sha256 -binary $1 > $3
cat $1 >> $3

echo Signing output image...
openssl dgst -binary -sha256 -keyform DER -sign $2 -out $4 $3

cat $4 | xxd -c 16 -i