# ndk-cameras-cmdline
Command line tools to operate Android cameras, using the NDK cameras API and termux (Android commandline window app).

This is some code extracted from NDK-samples/camera (https://github.com/android/ndk-samples/tree/master/camera) to be compiled into a stand-alone program using the clang compiler under termux.

Run the "build.sh" script to compile the code into binary.

Even though the original NDK camera example works fine with getting captured images from the android camera api, this code will run with no errors but no image is every available.

To run the binary, you will need to obtain the camera permission for termux by 
1. installing termux:API app (https://wiki.termux.com/wiki/Termux:API)
2. pkg install termux-api  (to install the termux-api commandline tools)
3. termux-camera-photo test.jpg (to invoke the camera capturing which will ask for camera permission first)

Once you grant the termux app with camera permission, you can run the binary from the temux commandline.

The current output looks like this (Running on a XiaoMi MI8 with Android P):
```
:/data/ndk-cameras-cmdline # ./build.sh 
:/data/ndk-cameras-cmdline # ./ndk-camera
D src/main.cc:163 Found camera ID 0
D src/main.cc:131 Camera available: True
D src/main.cc:131 Camera available: True
D src/main.cc:131 Camera available: True
D src/main.cc:131 Camera available: True
D src/main.cc:131 Camera available: True
D src/main.cc:131 Camera available: True
D src/main.cc:297 No Image found...
D src/main.cc:74 session 0x7cd60a2b40 is active
D src/main.cc:297 No Image found...
D src/main.cc:297 No Image found...
```

From top, I could see that the camera service is running at 25% of one core, which suggest something is running. A normal ndk sample camera app will run at about 100% of one core to display image to the ANativeWindow.
