#!/bin/sh

clang++ -o ndk-camera src/main.cc -I libs/ -lcamera2ndk -landroid -lmediandk -llog -lnativewindow -lc -lgui -lui -lutils 
