#! /bin/sh

cd "$(dirname "$0")"

cd ..

cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/
