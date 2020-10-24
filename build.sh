#!/bin/sh

clang++ -o ndk-camera src/main.cc src/ndk/ndk-camera.cc -I libs/ -lcamera2ndk -landroid -lmediandk -llog -lnativewindow -lc -lgui -lui -lutils 
