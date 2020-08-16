// Bare minimal code extracted from NDK samples/camera to reproduce
// the unexplainable ndk camera api behavior: camera opens fine but no image
// is sent through the api.

#include <android/log.h>
#include <android/native_window.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include <string>
#include <unistd.h>
#include <vector>

#include <ui/DisplayInfo.h>
#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include "ndk-camera.h"

void NDKCamera::OnSessionState(ACameraCaptureSession* ses,
                               CaptureSessionState state) {
  if (!ses || ses != captureSession_) {
    LOGW("CaptureSession is %s", (ses ? "NOT our session" : "NULL"));
    return;
  }

  ASSERT(state < CaptureSessionState::MAX_STATE, "Wrong state %d", state);
  captureSessionState_ = state;
}

void OnSessionActive(void* ctx, ACameraCaptureSession* ses) {
  LOGW("session %p is active", ses);
  reinterpret_cast<NDKCamera*>(ctx)
      ->OnSessionState(ses, CaptureSessionState::ACTIVE);
  PRINTD("session %p is active", ses);
}

ACameraCaptureSession_stateCallbacks* NDKCamera::GetSessionListener() {
  static ACameraCaptureSession_stateCallbacks sessionListener = {
      .context = this,
      .onClosed = nullptr,
      .onReady = nullptr,
      .onActive = ::OnSessionActive,
  };
  return &sessionListener;
}

void NDKCamera::OnDeviceError(ACameraDevice* dev, int err) {
  std::string id(ACameraDevice_getId(dev));
  LOGI("CameraDevice %s is in error %#x", id.c_str(), err);
  /*
  PrintCameraDeviceError(err);

  CameraId& cam = cameras_[id];

  switch (err) {
    case ERROR_CAMERA_IN_USE:
      cam.available_ = false;
      cam.owner_ = false;
      break;
    case ERROR_CAMERA_SERVICE:
    case ERROR_CAMERA_DEVICE:
    case ERROR_CAMERA_DISABLED:
    case ERROR_MAX_CAMERAS_IN_USE:
      cam.available_ = false;
      cam.owner_ = false;
      break;
    default:
      LOGI("Unknown Camera Device Error: %#x", err);
  }
   */
}

void OnDeviceErrorChanges(void* ctx, ACameraDevice* dev, int err) {
  reinterpret_cast<NDKCamera*>(ctx)->OnDeviceError(dev, err);
}

ACameraDevice_stateCallbacks* NDKCamera::GetDeviceListener() {
  static ACameraDevice_stateCallbacks cameraDeviceListener = {
      .context = this,
      .onDisconnected = nullptr,
      .onError = ::OnDeviceErrorChanges,
  };
  return &cameraDeviceListener;
}

/*
 * Camera Manager Listener object
 */
void NDKCamera::OnCameraStatusChanged(const char* id, bool available) {
    //cameras_[std::string(id)].available_ = available ? true : false;
  PRINTD("Camera available: %s", available? "True" : "False");
}
void OnCameraAvailable(void* ctx, const char* id) {
  reinterpret_cast<NDKCamera*>(ctx)->OnCameraStatusChanged(id, true);
}
void OnCameraUnavailable(void* ctx, const char* id) {
  reinterpret_cast<NDKCamera*>(ctx)->OnCameraStatusChanged(id, false);
}

ACameraManager_AvailabilityCallbacks* NDKCamera::GetManagerListener() {
  static ACameraManager_AvailabilityCallbacks cameraMgrListener = {
      .context = this,
      .onCameraAvailable = ::OnCameraAvailable,
      .onCameraUnavailable = ::OnCameraUnavailable,
  };
  return &cameraMgrListener;
}

NDKCamera::NDKCamera()
: cameraMgr_(nullptr),
  outputContainer_(nullptr),
  captureSession_(nullptr),
  captureSessionState_(CaptureSessionState::MAX_STATE),
  pDevice_(nullptr){}

void NDKCamera::Init(bool rear) {
  requests_.resize(CAPTURE_REQUEST_COUNT);
  memset(requests_.data(), 0, requests_.size() * sizeof(requests_[0]));
  cameraMgr_ = ACameraManager_create();
  ASSERT(cameraMgr_, "Failed to create cameraManager");

  // Pick up the front or rear camera id.
  EnumerateCamera(rear);
  ASSERT(activeCameraId_.size(), "Unknown ActiveCameraIdx");
  PRINTD("Found camera ID %s", activeCameraId_.c_str());
  // Create back facing camera device
  CALL_MGR(openCamera(cameraMgr_, activeCameraId_.c_str(), GetDeviceListener(), &pDevice_));
  CALL_MGR(registerAvailabilityCallback(cameraMgr_, GetManagerListener()));
}

void NDKCamera::EnumerateCamera(bool rear) {
  bool found_camera = false;
  ACameraIdList* cameraIds = nullptr;
  CALL_MGR(getCameraIdList(cameraMgr_, &cameraIds));

  for (int i = 0; i < cameraIds->numCameras; ++i) {
    const char* id = cameraIds->cameraIds[i];
    ACameraMetadata* metadataObj;
    CALL_MGR(getCameraCharacteristics(cameraMgr_, id, &metadataObj));

    int32_t count = 0;
    const uint32_t* tags = nullptr;
    ACameraMetadata_getAllTags(metadataObj, &count, &tags);
    for (int tagIdx = 0; tagIdx < count; ++tagIdx) {
      if (ACAMERA_LENS_FACING == tags[tagIdx]) {
        ACameraMetadata_const_entry lensInfo = {
            0,
        };
        CALL_METADATA(getConstEntry(metadataObj, tags[tagIdx], &lensInfo));
        int facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(
            lensInfo.data.u8[0]);
        if (rear && facing == ACAMERA_LENS_FACING_BACK) {
          activeCameraId_ = id;
          found_camera = true;
        } else if (!rear && facing == ACAMERA_LENS_FACING_FRONT) {
          activeCameraId_ = id;
          found_camera = true;
        }
        break;
      }
    }
    ACameraMetadata_free(metadataObj);
  }

  ASSERT(found_camera, "No Camera Available on the device");
  ACameraManager_deleteCameraIdList(cameraIds);
}

NDKCamera::~NDKCamera() {
  // stop session if it is on:
  if (captureSessionState_ == CaptureSessionState::ACTIVE) {
    ACameraCaptureSession_stopRepeating(captureSession_);
  }
  ACameraCaptureSession_close(captureSession_);

  for (auto& req : requests_) {
    CALL_REQUEST(removeTarget(req.request_, req.target_));
    ACaptureRequest_free(req.request_);
    ACameraOutputTarget_free(req.target_);

    CALL_CONTAINER(remove(outputContainer_, req.sessionOutput_));
    ACaptureSessionOutput_free(req.sessionOutput_);

    ANativeWindow_release(req.outputNativeWindow_);
  }

  requests_.resize(0);
  ACaptureSessionOutputContainer_free(outputContainer_);

  if (pDevice_) {
    CALL_DEV(close(pDevice_));
  }
  
  if (cameraMgr_) {
    CALL_MGR(unregisterAvailabilityCallback(cameraMgr_, GetManagerListener()));
    ACameraManager_delete(cameraMgr_);
    cameraMgr_ = nullptr;
  }
}

void NDKCamera::StartPreview(bool start) {
  if (start) {
    CALL_SESSION(setRepeatingRequest(captureSession_, nullptr, 1,
                                     &requests_[PREVIEW_REQUEST_IDX].request_,
                                     nullptr));
  } else if (!start && captureSessionState_ == CaptureSessionState::ACTIVE) {
    ACameraCaptureSession_stopRepeating(captureSession_);
  } else {
    ASSERT(false, "Conflict states(%s, %d)", (start ? "true" : "false"),
           captureSessionState_);
  }
}

void NDKCamera::CreateSession(int capture_width, int capture_height) {
  surfaceBundle_.session = new android::SurfaceComposerClient();
  ASSERT(surfaceBundle_.session, "failed to create surface composer client");
  surfaceBundle_.dtoken = android::SurfaceComposerClient::getBuiltInDisplay(
                android::ISurfaceComposer::eDisplayIdMain);
  ASSERT(surfaceBundle_.dtoken, "Failed to get built in display");

  android::status_t status = android::SurfaceComposerClient::getDisplayInfo(
      surfaceBundle_.dtoken, &surfaceBundle_.dinfo);
  ASSERT(status == 0, "status not zero");
  PRINTD("Dinfo %dx%d", surfaceBundle_.dinfo.w, surfaceBundle_.dinfo.h);
  surfaceBundle_.surfaceControl = surfaceBundle_.session->createSurface(
      android::String8("preview"), capture_width, capture_height,
      android::PIXEL_FORMAT_RGBX_8888, 0);
  ASSERT(surfaceBundle_.surfaceControl, "Failed to create surface control!");

  surfaceBundle_.surface = surfaceBundle_.surfaceControl->getSurface();
  ASSERT(surfaceBundle_.surface, "Failed to get surface!");
  PRINTD("Surface created!");

  CreateSession(surfaceBundle_.surface.get());
}

android::GraphicBuffer* NDKCamera::GetLatestFrame() {
  surfaceBundle_.outBuffer = nullptr;

  android::status_t status = surfaceBundle_.surface->getLastQueuedBuffer(
      &surfaceBundle_.outBuffer, &surfaceBundle_.outFence,
      surfaceBundle_.outTransformMatrix);

  PRINTD("getLastQueuedBuffer status %d", status);
  if (surfaceBundle_.outBuffer) {
    PRINTD("GraphicBuffer w=%d, h=%d, fmt=%d",
	   surfaceBundle_.outBuffer->getWidth(),
	   surfaceBundle_.outBuffer->getHeight(),
	   surfaceBundle_.outBuffer->getPixelFormat());
    return surfaceBundle_.outBuffer.get();
  } else {
    return nullptr;
  }
}

void NDKCamera::CreateSession(ANativeWindow* previewWindow) {
  // Create output from this app's ANativeWindow, and add into output container
  requests_[PREVIEW_REQUEST_IDX].outputNativeWindow_ = previewWindow;
  requests_[PREVIEW_REQUEST_IDX].template_ = TEMPLATE_PREVIEW;

  CALL_CONTAINER(create(&outputContainer_));
  auto& req = requests_[PREVIEW_REQUEST_IDX];
  {
    ANativeWindow_acquire(req.outputNativeWindow_);
    CALL_OUTPUT(create(req.outputNativeWindow_, &req.sessionOutput_));
    CALL_CONTAINER(add(outputContainer_, req.sessionOutput_));
    CALL_TARGET(create(req.outputNativeWindow_, &req.target_));
    CALL_DEV(createCaptureRequest(pDevice_, req.template_, &req.request_));
    CALL_REQUEST(addTarget(req.request_, req.target_));
  }

  // Create a capture session for the given preview request
  captureSessionState_ = CaptureSessionState::READY;
  CALL_DEV(createCaptureSession(pDevice_,
                                outputContainer_, GetSessionListener(),
                                &captureSession_));
}

void NDKCamera::CreateSession(ANativeWindow* previewWindow,
                              ANativeWindow* jpgWindow) {
  // Create output from this app's ANativeWindow, and add into output container
  requests_[PREVIEW_REQUEST_IDX].outputNativeWindow_ = previewWindow;
  requests_[PREVIEW_REQUEST_IDX].template_ = TEMPLATE_PREVIEW;
  requests_[JPG_CAPTURE_REQUEST_IDX].outputNativeWindow_ = jpgWindow;
  requests_[JPG_CAPTURE_REQUEST_IDX].template_ = TEMPLATE_STILL_CAPTURE;

  CALL_CONTAINER(create(&outputContainer_));
  for (auto& req : requests_) {
    ANativeWindow_acquire(req.outputNativeWindow_);
    CALL_OUTPUT(create(req.outputNativeWindow_, &req.sessionOutput_));
    CALL_CONTAINER(add(outputContainer_, req.sessionOutput_));
    CALL_TARGET(create(req.outputNativeWindow_, &req.target_));
    CALL_DEV(createCaptureRequest(pDevice_, req.template_, &req.request_));
    CALL_REQUEST(addTarget(req.request_, req.target_));
  }

  // Create a capture session for the given preview request
  captureSessionState_ = CaptureSessionState::READY;
  CALL_DEV(createCaptureSession(pDevice_,
                                outputContainer_, GetSessionListener(),
                                &captureSession_));
}

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
