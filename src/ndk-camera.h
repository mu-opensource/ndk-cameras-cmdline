#ifndef NDK_CAMERA_H_
#define NDK_CAMERA_H_

#define LOG_TAG "NDK-CAMERA"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ASSERT(cond, fmt, ...)                                \
  if (!(cond)) {                                              \
    __android_log_assert(#cond, LOG_TAG, fmt, ##__VA_ARGS__); \
  }
#define PRINTD(...) printf("D %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n");

/*
 * A set of macros to call into Camera APIs. The API is grouped with a few
 * objects, with object name as the prefix of function names.
 */
#define CALL_CAMERA(func)                                             \
  {                                                                   \
    camera_status_t status = func;                                    \
    ASSERT(status == ACAMERA_OK, "%s call failed with code: %#x, %s", \
           __FUNCTION__, status, GetErrorStr(status));                \
  }
#define CALL_MGR(func) CALL_CAMERA(ACameraManager_##func)
#define CALL_DEV(func) CALL_CAMERA(ACameraDevice_##func)
#define CALL_METADATA(func) CALL_CAMERA(ACameraMetadata_##func)
#define CALL_CONTAINER(func) CALL_CAMERA(ACaptureSessionOutputContainer_##func)
#define CALL_OUTPUT(func) CALL_CAMERA(ACaptureSessionOutput_##func)
#define CALL_TARGET(func) CALL_CAMERA(ACameraOutputTarget_##func)
#define CALL_REQUEST(func) CALL_CAMERA(ACaptureRequest_##func)
#define CALL_SESSION(func) CALL_CAMERA(ACameraCaptureSession_##func)

#define UKNOWN_TAG "UNKNOW_TAG"
#define MAKE_PAIR(val) std::make_pair(val, #val)

template <typename T>
const char* GetPairStr(T key, std::vector<std::pair<T, const char*>>& store) {
  typedef typename std::vector<std::pair<T, const char*>>::iterator iterator;
  for (iterator it = store.begin(); it != store.end(); ++it) {
    if (it->first == key) {
      return it->second;
    }
  }
  LOGW("(%#08x) : UNKNOWN_TAG for %s", key, typeid(store[0].first).name());
  return UKNOWN_TAG;
}
/*
 * camera_status_t error translation
 */
using ERROR_PAIR = std::pair<camera_status_t, const char*>;
static std::vector<ERROR_PAIR> errorInfo{
    MAKE_PAIR(ACAMERA_OK),
    MAKE_PAIR(ACAMERA_ERROR_UNKNOWN),
    MAKE_PAIR(ACAMERA_ERROR_INVALID_PARAMETER),
    MAKE_PAIR(ACAMERA_ERROR_CAMERA_DISCONNECTED),
    MAKE_PAIR(ACAMERA_ERROR_NOT_ENOUGH_MEMORY),
    MAKE_PAIR(ACAMERA_ERROR_METADATA_NOT_FOUND),
    MAKE_PAIR(ACAMERA_ERROR_CAMERA_DEVICE),
    MAKE_PAIR(ACAMERA_ERROR_CAMERA_SERVICE),
    MAKE_PAIR(ACAMERA_ERROR_SESSION_CLOSED),
    MAKE_PAIR(ACAMERA_ERROR_INVALID_OPERATION),
    MAKE_PAIR(ACAMERA_ERROR_STREAM_CONFIGURE_FAIL),
    MAKE_PAIR(ACAMERA_ERROR_CAMERA_IN_USE),
    MAKE_PAIR(ACAMERA_ERROR_MAX_CAMERA_IN_USE),
    MAKE_PAIR(ACAMERA_ERROR_CAMERA_DISABLED),
    MAKE_PAIR(ACAMERA_ERROR_PERMISSION_DENIED),
};

struct ImageFormat {
  int32_t width;
  int32_t height;
  enum AIMAGE_FORMATS format;  // Through out this demo, the format is fixed to
                   // YUV_420 format
};

/**
 * MAX_BUF_COUNT:
 *   Max buffers in this ImageReader.
 */
#define MAX_BUF_COUNT 4

class ImageReader {
public:
  explicit ImageReader(ImageFormat* format);
  virtual ~ImageReader();

  ANativeWindow* GetNativeWindow(void);
  AImage* GetNextImage(void);
  void DeleteImage(AImage* image);
  
 private:
  AImageReader* reader_;
};

enum class CaptureSessionState : int32_t {
  READY = 0,  // session is ready
  ACTIVE,     // session is busy
  CLOSED,     // session is closed(by itself or a new session evicts)
  MAX_STATE
};

enum PREVIEW_INDICES {
  PREVIEW_REQUEST_IDX = 0,
  JPG_CAPTURE_REQUEST_IDX,
  CAPTURE_REQUEST_COUNT,
};

struct CaptureRequestInfo {
  ANativeWindow* outputNativeWindow_;
  ACaptureSessionOutput* sessionOutput_;
  ACameraOutputTarget* target_;
  ACaptureRequest* request_;
  ACameraDevice_request_template template_;
  int sessionSequenceId_;
};

class NDKCamera {
 public:
  NDKCamera();
  virtual ~NDKCamera();
  
  void Init(bool rear);
  void CreateSession(ANativeWindow* previewWindow, ANativeWindow* jpgWindow);
  void StartPreview(bool start);
  
  // Listeners
  ACameraManager_AvailabilityCallbacks* GetManagerListener();
  ACameraDevice_stateCallbacks* GetDeviceListener();
  ACameraCaptureSession_stateCallbacks* GetSessionListener();
  
  void OnCameraStatusChanged(const char* id, bool available);
  void OnDeviceState(ACameraDevice* dev);
  void OnDeviceError(ACameraDevice* dev, int err);
  void OnSessionState(ACameraCaptureSession* ses, CaptureSessionState state);

 private:
  void EnumerateCamera(bool rear);

  ACameraManager* cameraMgr_;
  std::string activeCameraId_;

  std::vector<CaptureRequestInfo> requests_;
  ACaptureSessionOutputContainer* outputContainer_;
  ACameraCaptureSession* captureSession_;
  CaptureSessionState captureSessionState_;
  ACameraDevice* pDevice_;
};

#endif  // NDK_CAMERA_H_
