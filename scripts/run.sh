#! /bin/sh

cd "$(dirname "$0")"

cd ..;

if [ -d "build/" ]
then
	cd build/
	./ArduPlotter
else
    echo "Error: Directory build does not exist."
fi
