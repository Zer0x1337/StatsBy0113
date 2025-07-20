#!/bin/bash

BUILD_DIR="build"

if [ "$1" == "clean" ]; then
  echo "Cleaning build directory..."
  rm -rf "$BUILD_DIR"
  echo "Clean complete."
  exit 0
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || exit

echo "Configuring CMake..."
cmake ..

echo "Building project..."
make

if [ $? -eq 0 ]; then
  echo "Build successful!"
else
  echo "Build failed."
fi
