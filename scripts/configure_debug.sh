#! /bin/sh

cd "$(dirname "$0")"

cd ..

cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build/
