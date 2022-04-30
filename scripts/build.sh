#! /bin/sh

cd "$(dirname "$0")"

cd ..;

if [ -d "build/" ]
then
	cd build/
	make $1
else
    echo "Error: Directory build does not exist."
fi
