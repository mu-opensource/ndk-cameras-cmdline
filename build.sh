#!/bin/sh

clang++ -o ndk-camera src/main.cc -lcamera2ndk -landroid -lmediandk -llog -lnativewindow -lc
