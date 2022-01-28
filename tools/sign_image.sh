#!/bin/bash

if [ "$1" == "" ] || [ "$2" == "" ] || [ "$3" == "" ] || [ "$4" == "" ] || [ $# != 4 ] ; then
	echo Usage: $0 \<input-image\> \<private-key\> \<output-image\> \<signature-file\>
	exit 0
fi

echo preparing output image...
openssl dgst -sha256 -binary $1 > $3
cat $1 >> $3
echo signing output image...
openssl dgst -binary -sha256 -keyform DER -sign $2 -out $4 $3
cat $4 | xxd -c 16 -i