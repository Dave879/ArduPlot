#! /bin/sh

cd "$(dirname "$0")"

cd ..

cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=false -DGLFW_BUILD_DOCS=OFF -S . -B build/
