#!/bin/sh
set -e

if [ -z "$1" ];
then
    echo "usage: $0 <grammar>"
    exit 1
fi

EXT=${1##*.}
if [ -n "$EXT" ]
then
    BASENAME=`basename $1 .$EXT`
else
    BASENAME=`basename $1`
fi

JSON="$BASENAME.json"
RS="$BASENAME.rs"

./unicc -R json -P $1 >$JSON
python3 codegen.py -v unicc $JSON -r rust.rs >$RS
cat $RS
rustc $RS
./$BASENAME
