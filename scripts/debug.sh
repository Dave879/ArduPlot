#! /bin/sh

cd "$(dirname "$0")"

cd ..;

if [ -d "build/" ]
then
	cd build/
	lldb ./ArduPlot
else
    echo "Error: Directory build does not exist."
fi
