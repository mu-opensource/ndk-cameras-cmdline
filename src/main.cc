// Bare minimal code extracted from NDK samples/camera to reproduce
// the unexplainable ndk camera api behavior: camera opens fine but no image
// is sent through the api.

#include "ndk/ndk-camera.h"

int main(int argc, char** argv) {
  bool rear_camera = false;

  // Create camera
  NDKCamera ndk_camera;
  ndk_camera.Init(rear_camera);
  ndk_camera.CreateSession(1280, 720);
  ndk_camera.StartPreview(true);

  while (true) {
    android::GraphicBuffer* graphic_buffer = ndk_camera.GetLatestFrame();
    if (!graphic_buffer) {
      PRINTD("No Image found...");
      sleep(1);
    }
  }
  
  return 0;
}
