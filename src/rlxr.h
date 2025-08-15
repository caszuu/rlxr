/*
 *  rlxr - a minimalist openxr integration with raylib and rlgl
 */

/* clang-format off */

#ifndef RLXR_H
#define RLXR_H

#if defined(RLXR_STANDALONE)
    #include "raymath.h"
#else
    #include "raylib.h"
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------

// Function specifiers in case library is build/used as a shared library
// NOTE: Microsoft specifiers to tell compiler that symbols are imported/exported from a .dll
// NOTE: visibility(default) attribute makes symbols "visible" when compiled with -fvisibility=hidden
#if defined(_WIN32) && defined(BUILD_LIBTYPE_SHARED)
    #define RLAPI __declspec(dllexport)     // We are building the library as a Win32 shared library (.dll)
#elif defined(BUILD_LIBTYPE_SHARED)
    #define RLAPI __attribute__((visibility("default"))) // We are building the library as a Unix shared library (.so/.dylib)
#elif defined(_WIN32) && defined(USE_LIBTYPE_SHARED)
    #define RLAPI __declspec(dllimport)     // We are using the library as a Win32 shared library (.dll)
#endif

// Function specifiers definition
#ifndef RLAPI
    #define RLAPI       // Functions defined as 'extern' by default (implicit specifiers)
#endif

// Allow custom memory allocators
#ifndef RL_MALLOC
    #define RL_MALLOC(sz)     malloc(sz)
#endif
#ifndef RL_CALLOC
    #define RL_CALLOC(n,sz)   calloc(n,sz)
#endif
#ifndef RL_REALLOC
    #define RL_REALLOC(n,sz)  realloc(n,sz)
#endif
#ifndef RL_FREE
    #define RL_FREE(p)        free(p)
#endif

// Raylib TraceLog support
#ifndef TRACELOG
    #ifndef RLXR_SUPPORT_TRACELOG
        #define TRACELOG(level, ...) (void)0
        #define TRACELOGD(...) (void)0
    #else
        #define TRACELOG(level, ...) TraceLog(level, __VA_ARGS__)
        #define TRACELOGD(...) TraceLog(LOG_DEBUG, __VA_ARGS__)
    #endif
#endif

#ifndef RLXR_APP_NAME
    #define RLXR_APP_NAME "rlxr app"
#endif

#ifndef RLXR_ENGINE_NAME
    #define RLXR_ENGINE_NAME "raylib"
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
#if (defined(__STDC__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    #include <stdbool.h>
#elif !defined(__cplusplus) && !defined(bool) && !defined(RL_BOOL_TYPE)
    // Boolean type
typedef enum bool { false = 0, true = !false } bool;
#endif

typedef struct {
    Vector3 position;
    Quaternion orientation;

    bool isPositionValid;
    bool isOrientationValid;
} rlPose;

typedef enum {
    RLXR_STATE_UNKNOWN = 0,
    RLXR_STATE_IDLE,
    RLXR_STATE_SYNCHRONIZED,
    RLXR_STATE_VISIBLE,
    RLXR_STATE_FOCUSED,
} rlXrState;

typedef enum {
    RLXR_TYPE_BOOLEAN = 0,
    RLXR_TYPE_FLOAT,
    RLXR_TYPE_VECTOR2F,
    RLXR_TYPE_POSE,
    RLXR_TYPE_VIBRATION,
} rlActionType;

typedef enum {
    RLXR_HAND_LEFT = 1,
    RLXR_HAND_RIGHT = 2,

    RLXR_HAND_BOTH = 1 | 2,
} rlActionDevices;

typedef enum {
    RLXR_COMPONENT_SELECT = 0,
    RLXR_COMPONENT_MENU,
    RLXR_COMPONENT_GRIP_POSE,
    RLXR_COMPONENT_AIM_POSE,
    RLXR_COMPONENT_HAPTIC,
    RLXR_COMPONENT_MAX_ENUM,
} rlActionComponent;

typedef struct {
    bool value;
    bool active;
    bool changed; // (since last UpdateXr)
} rlBoolState;

typedef struct {
    float value;
    bool active;
    bool changed; // (since last UpdateXr)
} rlFloatState;

typedef struct {
    Vector2 value;
    bool active;
    bool changed; // (since last UpdateXr)
} rlVector2State;

typedef struct {
    rlPose value;
    bool active;
} rlPoseState;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif

RLAPI bool InitXr(); // returns true if successful, *must* be called after InitWindow or rlglInit
RLAPI void CloseXr();

// Session state
RLAPI void UpdateXr();        // updates internal XR state and actions, *must* be called every frame before BeginXrMode
RLAPI bool IsXrConnected();   // returns true after InitXr(), returns false after CloseXr() or a fatal XR error
RLAPI bool IsXrFocused();     // returns true if the XR device is awake and providing input to the app
RLAPI rlXrState GetXrState(); // returns the current XR session state

// Spaces and Poses
RLAPI rlPose GetXrViewPose();          // returns the pose of the users view (usually the centroid between XR views used in BeginView)
RLAPI void SetXrPosition(Vector3 pos); // sets the offset of the _reference_ frame, this offsets the entire play space (including the users cameras / views) by [pos] allowing you to move the player though-out the virtual space
RLAPI void SetXrOrientation(Quaternion quat);
RLAPI rlPose GetXrPose();              // fetches the current reference frame offsets

// View Rendering
RLAPI int BeginXrMode(); // returns the number of views that are requested by the xr runtime (returns 0 if rendering is not required by the runtime, eg. app is not visible to user)
RLAPI void EndXrMode();  // end and submit frame, must be called even when 0 views are requested
RLAPI void BeginView(unsigned int index); // begin view with index in range [0, request_count), this sets up 3D rendering with an internal camera matching the view
RLAPI void EndView();    // finish view and disable 3D rendering

// Action and Bindings
RLAPI unsigned int rlLoadAction(const char *name, rlActionType type, rlActionDevices devices); // registers a new action with the XR runtime; [mustn't be called after first UpdateXr() call]
RLAPI void rlSuggestBinding(unsigned int action, rlActionComponent component); // suggests a binding for a registered action, this can be ignored / remapped by the XR runtime; [mustn't be called after first UpdateXr() call]

RLAPI void rlSuggestProfile(const char *profilePath); // select the interaction profile used for following binding suggestions (by default /interaction_profiles/khr/simple_controller), the same profile mustn't be selected twice; [mustn't be called after UpdateXr]
RLAPI void rlSuggestBindingPro(unsigned int action, rlActionDevices devices, const char *componentPath); // suggests a binding with a direct openxr component path; [mustn't be called after UpdateXr]

// Action Fetchers - value only
RLAPI bool rlGetBool(unsigned int action, rlActionDevices device);
RLAPI float rlGetFloat(unsigned int action, rlActionDevices device);
RLAPI Vector2 rlGetVector2(unsigned int action, rlActionDevices device);
RLAPI rlPose rlGetPose(unsigned int action, rlActionDevices device);

// Action Fetchers - states
RLAPI rlBoolState rlGetBoolState(unsigned int action, rlActionDevices device);
RLAPI rlFloatState rlGetFloatState(unsigned int action, rlActionDevices device);
RLAPI rlVector2State rlGetVector2State(unsigned int action, rlActionDevices device);
RLAPI rlPoseState rlGetPoseState(unsigned int action, rlActionDevices device);

// Action Drivers
RLAPI void rlApplyHaptic(unsigned int action, rlActionDevices device, long duration, float amplitude); // duration in nanoseconds (-1 == min supported duration by runtime), aplitude in range [0.0, 1.0]

#if defined(__cplusplus)
}
#endif

#endif // RLXR_H

/***********************************************************************************
*
*   RLXR IMPLEMENTATION
*
************************************************************************************/

#if defined(RLXR_IMPLEMENTATION)

#define XR_USE_GRAPHICS_API_OPENGL

// Select xr platform and include headers for window handles (required for XrGraphicsBinding...)
#if defined(WIN32)
    #define XR_USE_PLATFORM_WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#else
    // workaround for raylib / Xlib name collision (https://github.com/raysan5/raylib/blob/59546eb54ad950eb882627670c759a595d338acf/src/platforms/rcore_desktop_glfw.c#L79)
    #define Font X11Font

    #define XR_USE_PLATFORM_XLIB
    #include <X11/Xlib.h>
    #include <GL/glx.h>

    #undef Font

    // #define XR_USE_PLATFORM_WAYLAND
    // #include <wayland-client.h>
#endif

#define RLXR_MAX_SPACES_PER_ACTION 2
#define RLXR_MAX_PATH_LENGTH 256

#define RLXR_NULL_ACTION (~(unsigned int)0)

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "raymath.h"
#include "rlgl.h"

#include <stdlib.h>
#include <stdint.h> // for openxr ints
#include <string.h>
#include <assert.h>

#include <math.h>

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

typedef struct {
    XrSwapchainImageOpenGLKHR *colorImages;
    XrSwapchainImageOpenGLKHR *depthImages;
    unsigned int colorImageCount;
    unsigned int depthImageCount;
    
    XrSwapchain colorSwapchain;
    XrSwapchain depthSwapchain;

    int64_t colorFormat;
    int64_t depthFormat;

    unsigned int framebuffer;
    unsigned int depthRenderBuffer; // used only as a fallback
} rlxrViewBuffers;

typedef struct {
    XrAction action;
    XrSpace actionSpaces[RLXR_MAX_SPACES_PER_ACTION]; // used only for pose actions

    rlActionDevices subpaths;
} rlxrAction;

typedef struct {
    // session state //
    
    XrInstance instance;
    XrSession session;

    XrSessionState state;

    XrSystemId system;
    XrSystemProperties systemProps;

    XrViewConfigurationType viewConfig;
    unsigned int viewCount;
    XrViewConfigurationView *viewProps;

    rlxrViewBuffers *viewBufs;
    XrView *views;
    XrCompositionLayerProjectionView *projectionViews;
    XrCompositionLayerDepthInfoKHR *depthInfoViews;

    bool depthSupported;

    // spaces //

    XrSpace referenceSpace;

    Vector3 refPosition;
    Quaternion refOrientation;

    XrSpace viewSpace;

    // actions //

    XrActionSet actionSet;
    XrPath userPaths[2];

    unsigned int actionCount, actionCap;
    rlxrAction *actions;

    unsigned int bindingCount, bindingCap;
    XrActionSuggestedBinding *bindings;
    XrPath currentSuggestProfile;

    bool actionSetAttached;

    // frame state //

    XrFrameState frameState;

    bool sessionRunning; 
    bool frameActive;
    unsigned int viewActiveIndex;

    // extended functions //

    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR;
} rlxrState;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static rlxrState rlxr = { XR_NULL_HANDLE };

//----------------------------------------------------------------------------------
// Module Functions Definition - OpenXR state managment
//----------------------------------------------------------------------------------

static bool rlxrInitInstance() {
    // app info
    
    XrApplicationInfo appInfo;
    strncpy(appInfo.applicationName, RLXR_APP_NAME, XR_MAX_APPLICATION_NAME_SIZE);
    appInfo.applicationVersion = 1;
    strncpy(appInfo.engineName, RLXR_ENGINE_NAME, XR_MAX_APPLICATION_NAME_SIZE);
    appInfo.engineVersion = 1;
    appInfo.apiVersion = XR_CURRENT_API_VERSION;

    // instance extensions

    int instanceExtCount = 1;
    const char *instanceExts[1] = { XR_KHR_OPENGL_ENABLE_EXTENSION_NAME };

    // create instance

    XrInstanceCreateInfo instanceInfo = {XR_TYPE_INSTANCE_CREATE_INFO, NULL};
    instanceInfo.createFlags = 0;
    instanceInfo.applicationInfo = appInfo;
    instanceInfo.enabledApiLayerCount = 0;
    instanceInfo.enabledApiLayerNames = NULL;
    instanceInfo.enabledExtensionCount = instanceExtCount;
    instanceInfo.enabledExtensionNames = instanceExts;

    XrResult res = xrCreateInstance(&instanceInfo, &rlxr.instance);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to init XrInstance (%d)", res);
        return false;
    }

    // FIXME: XR_EXT_debug_utils setup
    res = xrGetInstanceProcAddr(rlxr.instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&rlxr.pfnGetOpenGLGraphicsRequirementsKHR);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to init OpenGL bindings (%d)", res);
        return false;
    }

    // get system

    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    res = xrGetSystem(rlxr.instance, &systemInfo, &rlxr.system);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to get XrSystemId (%d)", res);
        return false;
    }

    rlxr.systemProps.type = XR_TYPE_SYSTEM_PROPERTIES;
    rlxr.systemProps.next = 0;
    res = xrGetSystemProperties(rlxr.instance, rlxr.system, &rlxr.systemProps);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to get XrSystemProperties (%d)", res);
        return false;
    }

    // get view configuration(s)

    rlxr.viewConfig = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    res = xrEnumerateViewConfigurationViews(rlxr.instance, rlxr.system, rlxr.viewConfig, 0, &rlxr.viewCount, NULL);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to enumerate views (%d)", res);
        return false;
    }

    rlxr.viewProps = (XrViewConfigurationView *)RL_MALLOC(rlxr.viewCount * sizeof(rlxr.viewProps[0]));
    for (int i = 0; i < rlxr.viewCount; i++) {
        rlxr.viewProps[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        rlxr.viewProps[i].next = 0;
    }

    res = xrEnumerateViewConfigurationViews(rlxr.instance, rlxr.system, rlxr.viewConfig, rlxr.viewCount, &rlxr.viewCount, rlxr.viewProps);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to enumerate views (%d)", res);
        return false;
    }

    // create action set

    XrActionSetCreateInfo setInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
    strncpy(setInfo.actionSetName, "rlxr-primary-set", XR_MAX_ACTION_SET_NAME_SIZE);
    strncpy(setInfo.localizedActionSetName, RLXR_APP_NAME" Primary Input", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
    setInfo.priority = 0;

    res = xrCreateActionSet(rlxr.instance, &setInfo, &rlxr.actionSet);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to create action set (%d)", res);
        return false;
    }

    // init user paths

    xrStringToPath(rlxr.instance, "/user/hand/left", &rlxr.userPaths[0]);
    xrStringToPath(rlxr.instance, "/user/hand/right", &rlxr.userPaths[1]);

    xrStringToPath(rlxr.instance, "/interaction_profiles/khr/simple_controller", &rlxr.currentSuggestProfile);

    return true;
}

static int64_t rlxrChooseSwapchainFormat(int64_t preferred, bool fallback) {
    unsigned int formatCount;
    int64_t *formats;
    
    XrResult res = xrEnumerateSwapchainFormats(rlxr.session, 0, &formatCount, NULL);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to enumerate swapchain formats (%d)", res);
        return -1;
    }

    formats = (int64_t *)RL_MALLOC(formatCount * sizeof(formats[0]));
    res = xrEnumerateSwapchainFormats(rlxr.session, formatCount, &formatCount, formats);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to enumerate swapchain formats (%d)", res);

        RL_FREE(formats);
        return -1;
    }

    int64_t format = fallback ? -1 : formats[0];
    for (int i = 0; i < formatCount; i++) {
        if (formats[i] == preferred)
            format = preferred;
    }

    RL_FREE(formats);
    return format;
}

typedef union {

#ifdef XR_USE_PLATFORM_WIN32
    XrGraphicsBindingOpenGLWin32KHR win32;
#endif
#ifdef XR_USE_PLATFORM_XLIB
    XrGraphicsBindingOpenGLXlibKHR xlib;
#endif
#ifdef XR_USE_PLATFORM_WAYLAND
    XrGraphicsBindingOpenGLWaylandKHR wayland;
#endif

} rlxrGraphicsBindingOpenGL;

static bool rlxrInitSession() {
    // rlgl graphics binding

    XrGraphicsRequirementsOpenGLKHR openglReqs = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};

    XrResult res = rlxr.pfnGetOpenGLGraphicsRequirementsKHR(rlxr.instance, rlxr.system, &openglReqs);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to fetch OpenGL requirements (%d)", res);
        return false;
    }

    // FIXME: gl version check

    // note: this is a hacky solution taken from the Monado openxr-simple-example repo and only works for select platforms as openxr requires very low level handles.
    //       currently supported platforms are:
    //         - GL/Win32 (fetching current WGL context)
    //         - GL/Xlib (fetching current GLX context)
    //
    // - note on wayland: The most widely used way to enable wayland support is currently the XR_MDNX_egl_enable extension, but
    //                    GraphicsBindingsOpenGLWaylandKHR *might* also be supported in the future, see https://gitlab.freedesktop.org/monado/monado/-/merge_requests/2527
    //
    //                    For now GraphicsBindingsOpenGLWaylandKHR is implemented but disabled as it would fail on most if not all runtimes.

    rlxrGraphicsBindingOpenGL rlglInfo;

#ifdef XR_USE_PLATFORM_WIN32
    if (HGLRC ctx = wglGetCurrentContext()) {
        rlglInfo.win32 = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
        rlglInfo.win32.hDC = wglGetCurrentDC();
        rlglInfo.win32.HGLRC = ctx;

        TRACELOG(LOG_INFO, "XR: Detected graphics binding: Win32");
    } else
#endif
#ifdef XR_USE_PLATFORM_XLIB
    if (GLXContext ctx = glXGetCurrentContext()) {
        rlglInfo.xlib = (XrGraphicsBindingOpenGLXlibKHR){XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};
        rlglInfo.xlib.xDisplay = XOpenDisplay(NULL);
        rlglInfo.xlib.glxContext = ctx;
        rlglInfo.xlib.glxDrawable = glXGetCurrentDrawable();

        TRACELOG(LOG_INFO, "XR: Detected graphics binding: Xlib");
    } else
#endif
#ifdef XR_USE_PLATFORM_WAYLAND
    if (struct wl_display *disp = wl_display_connect(NULL)) {
        rlglInfo.wayland = (XrGraphicsBindingOpenGLWaylandKHR){XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR};
        rlglInfo.wayland.display = disp;

        TRACELOG(LOG_INFO, "XR: Detected graphics binding: Wayland");
    } else
#endif
    {
        TRACELOG(LOG_ERROR, "XR: No supported graphics platform detected");
        return false;
    }

    // create session
    
    XrSessionCreateInfo sessionInfo = {XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &rlglInfo;
    sessionInfo.createFlags = 0;
    sessionInfo.systemId = rlxr.system;

    res = xrCreateSession(rlxr.instance, &sessionInfo, &rlxr.session);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to create XrSession (%d)", res);
        return false;
    }

    rlxr.state = XR_SESSION_STATE_UNKNOWN;

    // init reference spaces

    XrReferenceSpaceCreateInfo refInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    refInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; // TODO: reference space type api
    refInfo.poseInReferenceSpace = (XrPosef){{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};

    res = xrCreateReferenceSpace(rlxr.session, &refInfo, &rlxr.referenceSpace);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to create reference XrSpace (%d)", res);
        return false;
    }

    refInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    res = xrCreateReferenceSpace(rlxr.session, &refInfo, &rlxr.viewSpace);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to create view XrSpace (%d)", res);
        return false;
    }

    // init swapchains

    int64_t colorFormat = rlxrChooseSwapchainFormat(GL_RGBA16F, true);
    int64_t depthFormat = rlxrChooseSwapchainFormat(GL_DEPTH_COMPONENT16, false);

    rlxr.depthSupported = true;
    if (depthFormat < 0) {
        TRACELOG(LOG_WARNING, "XR: Preferred depth format not supported, falling back to internal render buffers");
        rlxr.depthSupported = false;
    }

    rlxr.viewBufs = (rlxrViewBuffers *)RL_MALLOC(rlxr.viewCount * sizeof(rlxrViewBuffers));

    // init color swapchains
    for (int i = 0; i < rlxr.viewCount; i++) {
        rlxrViewBuffers *view = &rlxr.viewBufs[i];
        
        XrSwapchainCreateInfo chainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        chainInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        chainInfo.createFlags = 0;
        chainInfo.format = colorFormat;
        chainInfo.sampleCount = rlxr.viewProps[i].recommendedSwapchainSampleCount;
        chainInfo.width = rlxr.viewProps[i].recommendedImageRectWidth;
        chainInfo.height = rlxr.viewProps[i].recommendedImageRectHeight;
        chainInfo.faceCount = 1;
        chainInfo.arraySize = 1;
        chainInfo.mipCount = 1;        

        res = xrCreateSwapchain(rlxr.session, &chainInfo, &view->colorSwapchain);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to create swapchain (%d)", res);
            return false;
        }

        // enumerate chain images

        res = xrEnumerateSwapchainImages(view->colorSwapchain, 0, &view->colorImageCount, NULL);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to enumerate swapchain images (%d)", res);
            return false;
        }

        view->colorImages = (XrSwapchainImageOpenGLKHR *)RL_MALLOC(view->colorImageCount * sizeof(view->colorImages[0]));
        for (int i = 0; i < view->colorImageCount; i++) {
            view->colorImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
            view->colorImages[i].next = 0;
        }
        
        res = xrEnumerateSwapchainImages(view->colorSwapchain, view->colorImageCount, &view->colorImageCount, (XrSwapchainImageBaseHeader *)view->colorImages);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to enumerate swapchain images (%d)", res);
            return false;
        }
    }

    // init depth swapchains
    if (rlxr.depthSupported) {
        for (int i = 0; i < rlxr.viewCount; i++) {
            rlxrViewBuffers *view = &rlxr.viewBufs[i];
        
            XrSwapchainCreateInfo chainInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
            chainInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            chainInfo.createFlags = 0;
            chainInfo.format = depthFormat;
            chainInfo.sampleCount = rlxr.viewProps[i].recommendedSwapchainSampleCount;
            chainInfo.width = rlxr.viewProps[i].recommendedImageRectWidth;
            chainInfo.height = rlxr.viewProps[i].recommendedImageRectHeight;
            chainInfo.faceCount = 1;
            chainInfo.arraySize = 1;
            chainInfo.mipCount = 1;

            res = xrCreateSwapchain(rlxr.session, &chainInfo, &view->depthSwapchain);
            if (XR_FAILED(res)) {
                TRACELOG(LOG_ERROR, "XR: Failed to create depth swapchain (%d)", res);
                return false;
            }

            // enumerate chain images

            res = xrEnumerateSwapchainImages(view->depthSwapchain, 0, &view->depthImageCount, NULL);
            if (XR_FAILED(res)) {
                TRACELOG(LOG_ERROR, "XR: Failed to enumerate swapchain images (%d)", res);
                return false;
            }

            view->depthImages = (XrSwapchainImageOpenGLKHR *)RL_MALLOC(view->depthImageCount * sizeof(view->depthImages[0]));
            for (int i = 0; i < view->depthImageCount; i++) {
                view->depthImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
                view->depthImages[i].next = 0;
            }
        
            res = xrEnumerateSwapchainImages(view->depthSwapchain, view->depthImageCount, &view->depthImageCount, (XrSwapchainImageBaseHeader *)view->depthImages);
            if (XR_FAILED(res)) {
                TRACELOG(LOG_ERROR, "XR: Failed to enumerate swapchain images (%d)", res);
                return false;
            }
        }
    }

    for (int i = 0; i < rlxr.viewCount; i++) {
        rlxr.viewBufs[i].framebuffer = rlLoadFramebuffer();

        if (!rlxr.depthSupported) {
            rlxr.viewBufs[i].depthRenderBuffer = rlLoadTextureDepth(rlxr.viewProps[i].recommendedImageRectWidth, rlxr.viewProps[i].recommendedImageRectHeight, true);
            rlFramebufferAttach(rlxr.viewBufs[i].framebuffer, rlxr.viewBufs[i].depthRenderBuffer, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
        }
    }

    // pre-allocate view storage

    rlxr.views = (XrView *)RL_MALLOC(rlxr.viewCount * sizeof(XrView));
    for (int i = 0; i < rlxr.viewCount; i++) {
        rlxr.views[i].type = XR_TYPE_VIEW;
        rlxr.views[i].next = NULL;
    }
    
    rlxr.projectionViews = (XrCompositionLayerProjectionView *)RL_MALLOC(rlxr.viewCount * sizeof(XrCompositionLayerProjectionView));
    for (int i = 0; i < rlxr.viewCount; i++) {
        rlxr.projectionViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        rlxr.projectionViews[i].next = NULL;

        rlxr.projectionViews[i].subImage.swapchain = rlxr.viewBufs[i].colorSwapchain;
        rlxr.projectionViews[i].subImage.imageArrayIndex = 0;
        rlxr.projectionViews[i].subImage.imageRect.offset.x = 0;
        rlxr.projectionViews[i].subImage.imageRect.offset.y = 0;
        rlxr.projectionViews[i].subImage.imageRect.extent.width = rlxr.viewProps[i].recommendedImageRectWidth;
        rlxr.projectionViews[i].subImage.imageRect.extent.height = rlxr.viewProps[i].recommendedImageRectHeight;

        // .pose and .fov must be updated every frame
    }

    if (rlxr.depthSupported) {
        rlxr.depthInfoViews = (XrCompositionLayerDepthInfoKHR *)RL_MALLOC(rlxr.viewCount * sizeof(XrCompositionLayerDepthInfoKHR));
        for (int i = 0; i < rlxr.viewCount; i++) {
            rlxr.depthInfoViews[i].type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;
            rlxr.depthInfoViews[i].next = NULL;
            rlxr.depthInfoViews[i].minDepth = 0.f; // TODO: sync with rlgl clip mode (?)
            rlxr.depthInfoViews[i].maxDepth = 1.f;

            rlxr.depthInfoViews[i].subImage.swapchain = rlxr.viewBufs[i].depthSwapchain;
            rlxr.depthInfoViews[i].subImage.imageArrayIndex = 0;
            rlxr.depthInfoViews[i].subImage.imageRect.offset.x = 0;
            rlxr.depthInfoViews[i].subImage.imageRect.offset.y = 0;
            rlxr.depthInfoViews[i].subImage.imageRect.extent.width = rlxr.viewProps[i].recommendedImageRectWidth;
            rlxr.depthInfoViews[i].subImage.imageRect.extent.height = rlxr.viewProps[i].recommendedImageRectHeight;

            // .nearZ and .farZ must be updated every frame from rlgl

            // depth info is chained to projection, not submitted as separate layer
            rlxr.projectionViews[i].next = &rlxr.depthInfoViews[i];
        }
    }

    // log success and device info

    TRACELOG(LOG_INFO, "XR: OpenXR session initialized sucessufully");
    TRACELOG(LOG_INFO, "XR: System information:");
    TRACELOG(LOG_INFO, "    > Name: %s", rlxr.systemProps.systemName);
    TRACELOG(LOG_INFO, "    > View size: %d x %d", rlxr.viewProps[0].recommendedImageRectWidth, rlxr.viewProps[0].recommendedImageRectHeight);
    TRACELOG(LOG_INFO, "    > View count: %d", rlxr.viewCount);

    return true;
}

bool InitXr() {
    if (rlxr.instance) return true;

    memset(&rlxr, 0, sizeof(rlxr));

    if (!rlxrInitInstance()) return false;
    if (!rlxrInitSession()) return false;

    rlxr.refPosition = (Vector3){0.f, 0.f, 0.f};
    rlxr.refOrientation = (Quaternion){0.f, 0.f, 0.f, 1.f};

    rlxr.sessionRunning = false;
    rlxr.frameActive = false;
    rlxr.viewActiveIndex = ~0;
    
    return true;
}

void CloseXr() {
    if (!rlxr.instance) return;

    for (int i = 0; i < rlxr.actionCount; i++) {
        for (int j = 0; j < RLXR_MAX_SPACES_PER_ACTION; j++)
            if (rlxr.actions[i].actionSpaces[j] != XR_NULL_HANDLE)
                xrDestroySpace(rlxr.actions[i].actionSpaces[j]);

        xrDestroyAction(rlxr.actions[i].action);
    }
    RL_FREE(rlxr.actions);
    RL_FREE(rlxr.bindings);

    for (int i = 0; i < rlxr.viewCount; i++) {
        RL_FREE(rlxr.viewBufs[i].colorImages);
        if (rlxr.depthSupported) RL_FREE(rlxr.viewBufs[i].depthImages);
        
        rlUnloadFramebuffer(rlxr.viewBufs[i].framebuffer);
        xrDestroySwapchain(rlxr.viewBufs[i].colorSwapchain);

        if (rlxr.depthSupported) xrDestroySwapchain(rlxr.viewBufs[i].depthSwapchain);
        else rlUnloadTexture(rlxr.viewBufs[i].depthRenderBuffer);
    }

    RL_FREE(rlxr.projectionViews);
    RL_FREE(rlxr.views);
    RL_FREE(rlxr.viewBufs);

    xrDestroySpace(rlxr.viewSpace);
    xrDestroySpace(rlxr.referenceSpace);

    xrDestroyActionSet(rlxr.actionSet);

    xrDestroySession(rlxr.session);
    xrDestroyInstance(rlxr.instance);

    rlxr.instance = XR_NULL_HANDLE;
    TRACELOG(LOG_INFO, "XR: Session closed suceessfully");
}

static void submitSuggestedBindings() {
    assert(!rlxr.actionSetAttached);

    if (rlxr.bindingCount != 0) {
        XrInteractionProfileSuggestedBinding profileInfo = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        profileInfo.interactionProfile = rlxr.currentSuggestProfile;
        profileInfo.countSuggestedBindings = rlxr.bindingCount;
        profileInfo.suggestedBindings = rlxr.bindings;

        XrResult res = xrSuggestInteractionProfileBindings(rlxr.instance, &profileInfo);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to suggest bindings, input will probably not work (%d)", res);
        }
    }

    rlxr.bindingCount = 0;
}

void UpdateXr() {
    // attach action set (first update call)

    if (!rlxr.actionSetAttached) {
        // suggest bindings

        submitSuggestedBindings();

        // attach

        XrSessionActionSetsAttachInfo attachInfo = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        attachInfo.countActionSets = 1;
        attachInfo.actionSets = &rlxr.actionSet;

        XrResult res = xrAttachSessionActionSets(rlxr.session, &attachInfo);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to attach action set, input will probably not work (%d)", res);
        } else if (rlxr.actionCount != 0) {
            TRACELOG(LOG_INFO, "XR: %d actions attached successfully", rlxr.actionCount);
        }

        rlxr.actionSetAttached = true;
    }

    // poll events

    XrEventDataBuffer ev = {XR_TYPE_EVENT_DATA_BUFFER};
    for (; xrPollEvent(rlxr.instance, &ev) == XR_SUCCESS; ev = (XrEventDataBuffer){XR_TYPE_EVENT_DATA_BUFFER}) {
        switch (ev.type) {
            case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                TRACELOG(LOG_WARNING, "XR: Event buffer overflow, %d events lost; UpdateXr might be getting called too little", ((XrEventDataEventsLost *)&ev)->lostEventCount);
                break;

            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                TRACELOG(LOG_ERROR, "XR: Instance loss pending; rlxr disconnected.");

                CloseXr();
                return;

            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                // TODO: ref space
                break;

            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                XrEventDataSessionStateChanged *state = (XrEventDataSessionStateChanged *)&ev;

                if (state->state == XR_SESSION_STATE_READY) {
                    // session ready, begin session with view configuration
                    XrSessionBeginInfo beginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
                    beginInfo.primaryViewConfigurationType = rlxr.viewConfig;

                    XrResult res = xrBeginSession(rlxr.session, &beginInfo);
                    if (XR_FAILED(res)) {
                        TRACELOG(LOG_ERROR, "XR: Failed to begin session (%d)", res);
                    }

                    rlxr.sessionRunning = true;
                }
                if (state->state == XR_SESSION_STATE_STOPPING) {
                    XrResult res = xrEndSession(rlxr.session);
                    if (XR_FAILED(res)) {
                        TRACELOG(LOG_ERROR, "XR: Failed to end session (%d)", res);
                    }

                    rlxr.sessionRunning = false;
                }
                if (state->state == XR_SESSION_STATE_EXITING) {
                    TRACELOG(LOG_ERROR, "XR: Session exiting; rlxr disconnected.");

                    CloseXr();
                    return;
                }
                if (state->state == XR_SESSION_STATE_LOSS_PENDING) {
                    TRACELOG(LOG_ERROR, "XR: Session loss pending; rlxr disconnected.");

                    CloseXr();
                    return;
                }

                rlxr.state = state->state;
                break;
            }

            default:
                break;
        }
    }

    if (rlxr.sessionRunning) {
        // sync with xr runtime

        rlxr.frameState.type = XR_TYPE_FRAME_STATE;
        XrFrameWaitInfo waitInfo = {XR_TYPE_FRAME_WAIT_INFO};

        XrResult res = xrWaitFrame(rlxr.session, &waitInfo, &rlxr.frameState);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to wait for a frame (%d)", res);
        }

        // sync action set

        XrActiveActionSet activeSet = {};
        activeSet.actionSet = rlxr.actionSet;
        activeSet.subactionPath = XR_NULL_PATH;

        XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeSet;

        res = xrSyncActions(rlxr.session, &syncInfo);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_WARNING, "XR: Failed to sync actions");
        }
    }
}

bool IsXrConnected() {
    return rlxr.instance != XR_NULL_HANDLE;
}

bool IsXrFocused() {
    if (!rlxr.instance) return false;
    
    return rlxr.state == XR_SESSION_STATE_FOCUSED;
}

rlXrState GetXrState() {
    if (!rlxr.instance) return RLXR_STATE_UNKNOWN;
    
    switch (rlxr.state) {
    case XR_SESSION_STATE_IDLE:
    case XR_SESSION_STATE_READY:
    case XR_SESSION_STATE_STOPPING:
        return RLXR_STATE_IDLE;

    case XR_SESSION_STATE_SYNCHRONIZED:
        return RLXR_STATE_SYNCHRONIZED;

    case XR_SESSION_STATE_VISIBLE:
        return RLXR_STATE_VISIBLE;

    case XR_SESSION_STATE_FOCUSED:
        return RLXR_STATE_FOCUSED;

    default:
        return RLXR_STATE_UNKNOWN;
    }
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Space and Poses
//----------------------------------------------------------------------------------

static rlPose xrPoseToRlPose(XrPosef xrPose, bool position, bool orientation) {
    rlPose pose;
    pose.position = (Vector3){0.f, 0.f, 0.f};
    pose.orientation = (Quaternion){0.f, 0.f, 0.f, 1.f};
    pose.isPositionValid = false;
    pose.isOrientationValid = false;

    if (position) {
        pose.position = (Vector3){xrPose.position.x, xrPose.position.y, xrPose.position.z};
        pose.position = Vector3Add(rlxr.refPosition, pose.position);
        pose.isPositionValid = true;
    }

    if (orientation) {
        pose.orientation = (Quaternion){xrPose.orientation.x, xrPose.orientation.y, xrPose.orientation.z, xrPose.orientation.w};
        pose.orientation = QuaternionMultiply(rlxr.refOrientation, pose.orientation);
        pose.isOrientationValid = true;
    }

    return pose;
}

rlPose GetXrViewPose() {
    XrSpaceLocation location = {XR_TYPE_SPACE_LOCATION};

    XrResult res = xrLocateSpace(rlxr.viewSpace, rlxr.referenceSpace, rlxr.frameState.predictedDisplayTime, &location);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: failed to locate view space (%d)", res);
    }

    return xrPoseToRlPose(location.pose, location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT, location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT);
}

void SetXrPosition(Vector3 pos) {
    rlxr.refPosition = pos;
}

void SetXrOrientation(Quaternion quat) {
    rlxr.refOrientation = quat;
}

rlPose GetXrPose() {
    return (rlPose){rlxr.refPosition, rlxr.refOrientation};
}

//----------------------------------------------------------------------------------
// Module Functions Definition - View Rendering
//----------------------------------------------------------------------------------

// =============================================================================
// math code adapted from
// https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/common/xr_linear.h
// Copyright (c) 2017 The Khronos Group Inc.
// Copyright (c) 2016 Oculus VR, LLC.
// SPDX-License-Identifier: Apache-2.0
// =============================================================================

typedef enum {
    GRAPHICS_VULKAN,
    GRAPHICS_OPENGL,
    GRAPHICS_OPENGL_ES
} GraphicsAPI;

inline static void
XrMatrix4x4f_CreateProjectionFov(Matrix* result,
                                 GraphicsAPI graphicsApi,
                                 const XrFovf fov,
                                 const float nearZ,
                                 const float farZ)
{
    const float tanAngleLeft = tanf(fov.angleLeft);
    const float tanAngleRight = tanf(fov.angleRight);

    const float tanAngleDown = tanf(fov.angleDown);
    const float tanAngleUp = tanf(fov.angleUp);

    const float tanAngleWidth = tanAngleRight - tanAngleLeft;

    // Set to tanAngleDown - tanAngleUp for a clip space with positive Y
    // down (Vulkan). Set to tanAngleUp - tanAngleDown for a clip space with
    // positive Y up (OpenGL / D3D / Metal).
    const float tanAngleHeight =
     graphicsApi == GRAPHICS_VULKAN ? (tanAngleDown - tanAngleUp) : (tanAngleUp - tanAngleDown);

    // Set to nearZ for a [-1,1] Z clip space (OpenGL / OpenGL ES).
    // Set to zero for a [0,1] Z clip space (Vulkan / D3D / Metal).
    const float offsetZ =
     (graphicsApi == GRAPHICS_OPENGL || graphicsApi == GRAPHICS_OPENGL_ES) ? nearZ : 0;

    if (farZ <= nearZ) {
        // place the far plane at infinity
        result->m0 = 2 / tanAngleWidth;
        result->m4 = 0;
        result->m8 = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
        result->m12 = 0;

        result->m1 = 0;
        result->m5 = 2 / tanAngleHeight;
        result->m9 = (tanAngleUp + tanAngleDown) / tanAngleHeight;
        result->m13 = 0;

        result->m2 = 0;
        result->m6 = 0;
        result->m10 = -1;
        result->m14 = -(nearZ + offsetZ);

        result->m3 = 0;
        result->m7 = 0;
        result->m11 = -1;
        result->m15 = 0;
    } else {
        // normal projection
        result->m0 = 2 / tanAngleWidth;
        result->m4 = 0;
        result->m8 = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
        result->m12 = 0;

        result->m1 = 0;
        result->m5 = 2 / tanAngleHeight;
        result->m9 = (tanAngleUp + tanAngleDown) / tanAngleHeight;
        result->m13 = 0;

        result->m2 = 0;
        result->m6 = 0;
        result->m10 = -(farZ + offsetZ) / (farZ - nearZ);
        result->m14 = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

        result->m3 = 0;
        result->m7 = 0;
        result->m11 = -1;
        result->m15 = 0;
    }
}

int BeginXrMode() {
    assert(!rlxr.frameActive);

    if (!rlxr.sessionRunning) {
        return 0; // session not yet synchronized, skip this frame
    }

    // locate view poses

    XrViewState viewState = {XR_TYPE_VIEW_STATE};
    XrViewLocateInfo locateInfo = {XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = rlxr.viewConfig;
    locateInfo.displayTime = rlxr.frameState.predictedDisplayTime;
    locateInfo.space = rlxr.referenceSpace;

    XrResult res = xrLocateViews(rlxr.session, &locateInfo, &viewState, rlxr.viewCount, &rlxr.viewCount, rlxr.views);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to locate views (%d)", res);
    }

    // begin frame

    XrFrameBeginInfo beginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
    
    res = xrBeginFrame(rlxr.session, &beginInfo);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to begin a frame (%d)", res);
    }

    rlxr.frameActive = true;
    rlxr.viewActiveIndex = ~0;

    return rlxr.viewCount;
}

void EndXrMode() {
    if (!rlxr.frameActive) return;
    assert(rlxr.viewActiveIndex == ~0);

    // end frame and submit layer(s)

    XrCompositionLayerProjection layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    layer.layerFlags = 0;
    layer.space = rlxr.referenceSpace;
    layer.viewCount = rlxr.viewCount;
    layer.views = rlxr.projectionViews;

    const XrCompositionLayerBaseHeader *submit_layers[1] = { (XrCompositionLayerBaseHeader *)&layer };

    XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = rlxr.frameState.predictedDisplayTime;
    endInfo.layerCount = rlxr.frameState.shouldRender ? 1 : 0;
    endInfo.layers = submit_layers;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE; // TODO: add support for env blend modes

    XrResult res = xrEndFrame(rlxr.session, &endInfo);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to end a frame (%d)", res);
    }

    rlxr.frameActive = false;
}

void BeginView(unsigned int index) {
    assert(rlxr.frameActive && rlxr.viewActiveIndex == ~0);
    assert(index < rlxr.viewCount);

    // acquire swapchain images

    rlxrViewBuffers *view = &rlxr.viewBufs[index];

    XrSwapchainImageAcquireInfo acqInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t colorAcquiredIndex, depthAcquiredIndex;

    XrResult res = xrAcquireSwapchainImage(view->colorSwapchain, &acqInfo, &colorAcquiredIndex);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to acquire an image from swapchain (%d)", res);
    }

    if (rlxr.depthSupported) {
        res = xrAcquireSwapchainImage(view->depthSwapchain, &acqInfo, &depthAcquiredIndex);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to acquire an image from swapchain (%d)", res);
        }
    }

    XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = 1000;

    res = xrWaitSwapchainImage(view->colorSwapchain, &waitInfo);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to wait for an image from swapchain (%d)", res);
    }

    if (rlxr.depthSupported) {
        res = xrWaitSwapchainImage(view->depthSwapchain, &waitInfo);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to wait for an image from swapchain (%d)", res);
        }
    }

    // setup viewport and rlgl (very similar setup to BeginMode3D)

    rlDrawRenderBatchActive();

    rlxr.projectionViews[index].pose = rlxr.views[index].pose;
    rlxr.projectionViews[index].fov = rlxr.views[index].fov;

    if (rlxr.depthSupported) {
        rlxr.depthInfoViews[index].nearZ = rlGetCullDistanceNear();
        rlxr.depthInfoViews[index].farZ = rlGetCullDistanceFar();
    }

    int w = rlxr.viewProps[index].recommendedImageRectWidth;
    int h = rlxr.viewProps[index].recommendedImageRectHeight;

    rlViewport(0, 0, w, h);
    rlScissor(0, 0, w, h);

    rlFramebufferAttach(view->framebuffer, view->colorImages[colorAcquiredIndex].image, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    if (rlxr.depthSupported) {
        // attach XrSwapchain depth if supprted, if not a render buffer is already attached from swapchain setup
        rlFramebufferAttach(view->framebuffer, view->depthImages[depthAcquiredIndex].image, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    }

    rlEnableFramebuffer(view->framebuffer);
    rlSetFramebufferWidth(w);
    rlSetFramebufferHeight(h);
   
    rlEnableDepthTest();

    // setup view camera
    
    Matrix xr_proj;
    XrMatrix4x4f_CreateProjectionFov(&xr_proj, GRAPHICS_OPENGL, rlxr.views[index].fov, rlGetCullDistanceNear(), rlGetCullDistanceFar());
    rlSetMatrixProjection(xr_proj);

    Vector3 pos = Vector3Add(rlxr.refPosition, (Vector3){rlxr.views[index].pose.position.x, rlxr.views[index].pose.position.y, rlxr.views[index].pose.position.z});
    Quaternion quat = QuaternionMultiply(rlxr.refOrientation, (Quaternion){rlxr.views[index].pose.orientation.x, rlxr.views[index].pose.orientation.y, rlxr.views[index].pose.orientation.z, rlxr.views[index].pose.orientation.w});

    Matrix xr_view = MatrixMultiply(QuaternionToMatrix(quat), MatrixTranslate(pos.x, pos.y, pos.z));
    xr_view = MatrixInvert(xr_view);
    rlSetMatrixModelview(xr_view);
    
    rlxr.viewActiveIndex = index;
}

void EndView() {
    assert(rlxr.frameActive && rlxr.viewActiveIndex != ~0);

    rlDrawRenderBatchActive();

    // release swapchains

    XrSwapchainImageReleaseInfo relInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    
    XrResult res = xrReleaseSwapchainImage(rlxr.viewBufs[rlxr.viewActiveIndex].colorSwapchain, &relInfo);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to release a swapchain image (%d)", res);
    }

    if (rlxr.depthSupported) {
        res = xrReleaseSwapchainImage(rlxr.viewBufs[rlxr.viewActiveIndex].depthSwapchain, &relInfo);
        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to release a swapchain image (%d)", res);
        }
    }

    // return rlgl to a default state

    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();

    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();

    rlDisableFramebuffer();
    rlDisableDepthTest();

#ifdef RAYLIB_H
    // a hacky way to tell raylib to restore its default window viewport
    EndTextureMode();
#endif

    rlxr.viewActiveIndex = ~0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Actions
//----------------------------------------------------------------------------------

static void rlxrResizeArray(void **ptr, unsigned int *size, unsigned int *cap, unsigned int new_size, unsigned int elem_size) {
    *size = new_size;
    if (new_size <= *cap) return;

    unsigned int new_cap = *cap * 2;
    *cap = new_cap > new_size ? new_cap : new_size;

    *ptr = RL_REALLOC(*ptr, *cap * elem_size);
}

unsigned int rlLoadAction(const char *name, rlActionType type, rlActionDevices devices) {
    assert(!rlxr.actionSetAttached);

    // create action

    XrActionCreateInfo actionInfo = {XR_TYPE_ACTION_CREATE_INFO};
    strncpy(actionInfo.actionName, name, XR_MAX_ACTION_NAME_SIZE);
    strncpy(actionInfo.localizedActionName, name, XR_MAX_LOCALIZED_ACTION_NAME_SIZE);

    switch (type) {
    case RLXR_TYPE_BOOLEAN:
        actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
        break;

    case RLXR_TYPE_FLOAT:
        actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
        break;

    case RLXR_TYPE_VECTOR2F:
        actionInfo.actionType = XR_ACTION_TYPE_VECTOR2F_INPUT;
        break;

    case RLXR_TYPE_POSE:
        actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
        break;

    case RLXR_TYPE_VIBRATION:
        actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
        break;
    }

    switch (devices) {
    case RLXR_HAND_LEFT:
        actionInfo.countSubactionPaths = 1;
        actionInfo.subactionPaths = &rlxr.userPaths[0];
        break;

    case RLXR_HAND_RIGHT:
        actionInfo.countSubactionPaths = 1;
        actionInfo.subactionPaths = &rlxr.userPaths[1];
        break;

    case RLXR_HAND_BOTH:
        actionInfo.countSubactionPaths = 2;
        actionInfo.subactionPaths = rlxr.userPaths;
        break;

    default:
        TRACELOG(LOG_ERROR, "XR: invalid action devices for action %s", name);
        return RLXR_NULL_ACTION;
    }

    XrAction xrAction;
    XrResult res = xrCreateAction(rlxr.actionSet, &actionInfo, &xrAction);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to create action %s", name);
        return RLXR_NULL_ACTION;
    }

    // create action space (if required)

    XrSpace xrSpaces[RLXR_MAX_SPACES_PER_ACTION] = { XR_NULL_HANDLE, XR_NULL_HANDLE };

    if (type == RLXR_TYPE_POSE) {
        XrActionSpaceCreateInfo spaceInfo = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
        spaceInfo.action = xrAction;
        spaceInfo.poseInActionSpace = (XrPosef){{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};

        switch (devices) {
        case RLXR_HAND_LEFT:
            spaceInfo.subactionPath = rlxr.userPaths[0];
            res = xrCreateActionSpace(rlxr.session, &spaceInfo, &xrSpaces[0]);
            break;

        case RLXR_HAND_RIGHT:
            spaceInfo.subactionPath = rlxr.userPaths[1];
            res = xrCreateActionSpace(rlxr.session, &spaceInfo, &xrSpaces[1]);
            break;

        case RLXR_HAND_BOTH:
            for (int i = 0; i < 2; i++) {
                spaceInfo.subactionPath = rlxr.userPaths[i];
                res = xrCreateActionSpace(rlxr.session, &spaceInfo, &xrSpaces[i]);

                if (XR_FAILED(res)) break;
            }
            break;
        }

        if (XR_FAILED(res)) {
            TRACELOG(LOG_ERROR, "XR: Failed to create action spaces for action %s", name);
            return RLXR_NULL_ACTION;
        }
    }

    // allocate a new rlxrAction

    unsigned int actionIdx = rlxr.actionCount;
    rlxrResizeArray((void **)&rlxr.actions, &rlxr.actionCount, &rlxr.actionCap, rlxr.actionCount + 1, sizeof(rlxrAction));

    rlxrAction *action = &rlxr.actions[actionIdx];
    action->action = xrAction;
    memcpy(action->actionSpaces, xrSpaces, sizeof(action->actionSpaces));
    action->subpaths = devices;

    return actionIdx;
}

static void appendBinding(rlxrAction *action, const char *path) {
    assert(!rlxr.actionSetAttached);

    // convert path

    XrPath xrPath;
    XrResult res = xrStringToPath(rlxr.instance, path, &xrPath);

    if (XR_FAILED(res)) {
        switch (res) {
        case XR_ERROR_PATH_FORMAT_INVALID:
            TRACELOG(LOG_ERROR, "XR: Failed to suggest bidning, path format invalid");
            return;

        case XR_ERROR_PATH_COUNT_EXCEEDED:
            TRACELOG(LOG_ERROR, "XR: Failed to suggest bidning, path count exceeded");
            return;

        default:
            TRACELOG(LOG_ERROR, "XR: Failed to suggest bidning, path error (%d)", res);
            return;
        }
    }

    // insert new bidning

    unsigned int bindingIdx = rlxr.bindingCount;
    rlxrResizeArray((void **)&rlxr.bindings, &rlxr.bindingCount, &rlxr.bindingCap, rlxr.bindingCount + 1, sizeof(XrActionSuggestedBinding));

    XrActionSuggestedBinding *binding = &rlxr.bindings[bindingIdx];
    binding->action = action->action;
    binding->binding = xrPath;
}

// LUTs of all enum component paths (based on the khr/simple_controller profile)
static const char *khrHandPaths[] = {
    "/input/select/click",
    "/input/menu/click",
    "/input/grip/pose",
    "/input/aim/pose",
    "/output/haptic",
};

void rlSuggestBinding(unsigned int action, rlActionComponent component) {
    rlxrAction *ac = &rlxr.actions[action];

    rlSuggestBindingPro(action, ac->subpaths, khrHandPaths[component]);
}

void rlSuggestProfile(const char *profilePath) {
    // submit buffered bindings
    submitSuggestedBindings();

    // load next interaction profile
    xrStringToPath(rlxr.instance, profilePath, &rlxr.currentSuggestProfile);
}

void rlSuggestBindingPro(unsigned int action, rlActionDevices devices, const char *component) {
    assert(!rlxr.actionSetAttached);
    if (action == RLXR_NULL_ACTION) return;

    rlxrAction *ac = &rlxr.actions[action];
    static char pathBuf[RLXR_MAX_PATH_LENGTH];

    if (devices == RLXR_HAND_BOTH) {
        strncpy(pathBuf, "/user/hand/left", RLXR_MAX_PATH_LENGTH);
        strncat(pathBuf, component, RLXR_MAX_PATH_LENGTH - strlen(pathBuf));
        appendBinding(ac, pathBuf);

        strncpy(pathBuf, "/user/hand/right", RLXR_MAX_PATH_LENGTH);
        strncat(pathBuf, component, RLXR_MAX_PATH_LENGTH - strlen(pathBuf));
        appendBinding(ac, pathBuf);
    } else {
        const char *userPath = devices == RLXR_HAND_LEFT ? "/user/hand/left" : "/user/hand/right";
        strncpy(pathBuf, userPath, RLXR_MAX_PATH_LENGTH);
        strncat(pathBuf, component, RLXR_MAX_PATH_LENGTH - strlen(pathBuf));

        appendBinding(ac, pathBuf);
    }
}

rlBoolState rlGetBoolState(unsigned int action, rlActionDevices device) {
    assert(rlxr.actionSetAttached);
    if (action == RLXR_NULL_ACTION) return (rlBoolState){ 0, 0, 0 };

    rlxrAction *ac = &rlxr.actions[action];

    XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    getInfo.action = ac->action;

    switch (device) {
    case RLXR_HAND_LEFT:
        getInfo.subactionPath = rlxr.userPaths[0];
        break;

    case RLXR_HAND_RIGHT:
        getInfo.subactionPath = rlxr.userPaths[1];
        break;

    default:
        TRACELOG(LOG_WARNING, "XR: Unsupported device in getBool (action: %d)", action);
        return (rlBoolState){ 0, 0, 0 };
    }

    XrActionStateBoolean state = {XR_TYPE_ACTION_STATE_BOOLEAN};
    XrResult res = xrGetActionStateBoolean(rlxr.session, &getInfo, &state);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to get bool action (result: %d; action: %d)", res, action);
        return (rlBoolState){ 0, 0, 0 };
    }

    return (rlBoolState){ (bool)state.currentState, (bool)state.isActive, (bool)state.changedSinceLastSync };
}

bool rlGetBool(unsigned int action, rlActionDevices device) {
    rlBoolState state = rlGetBoolState(action, device);
    return state.value && state.active;
}

rlFloatState rlGetFloatState(unsigned int action, rlActionDevices device) {
    assert(rlxr.actionSetAttached);
    if (action == RLXR_NULL_ACTION) return (rlFloatState){ 0.f, 0, 0 };

    rlxrAction *ac = &rlxr.actions[action];

    XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    getInfo.action = ac->action;

    switch (device) {
    case RLXR_HAND_LEFT:
        getInfo.subactionPath = rlxr.userPaths[0];
        break;

    case RLXR_HAND_RIGHT:
        getInfo.subactionPath = rlxr.userPaths[1];
        break;

    default:
        TRACELOG(LOG_WARNING, "XR: Unsupported device in getFloat (action: %d)", action);
        return (rlFloatState){ 0.f, 0, 0 };
    }

    XrActionStateFloat state = {XR_TYPE_ACTION_STATE_FLOAT};
    XrResult res = xrGetActionStateFloat(rlxr.session, &getInfo, &state);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to get float action (result: %d; action: %d)", res, action);
        return (rlFloatState){ 0.f, 0, 0 };
    }

    return (rlFloatState){ state.currentState, (bool)state.isActive, (bool)state.changedSinceLastSync };
}

float rlGetFloat(unsigned int action, rlActionDevices device) {
    rlFloatState state = rlGetFloatState(action, device);
    return state.active ? state.value : 0.0f;
}

rlVector2State rlGetVector2State(unsigned int action, rlActionDevices device) {
    assert(rlxr.actionSetAttached);
    if (action == RLXR_NULL_ACTION) return (rlVector2State){ { 0.f, 0.f }, 0, 0 };

    rlxrAction *ac = &rlxr.actions[action];

    XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    getInfo.action = ac->action;

    switch (device) {
    case RLXR_HAND_LEFT:
        getInfo.subactionPath = rlxr.userPaths[0];
        break;

    case RLXR_HAND_RIGHT:
        getInfo.subactionPath = rlxr.userPaths[1];
        break;

    default:
        TRACELOG(LOG_WARNING, "XR: Unsupported device in getVector2 (action: %d)", action);
        return (rlVector2State){ { 0.f, 0.f }, 0, 0 };
    }

    XrActionStateVector2f state = {XR_TYPE_ACTION_STATE_VECTOR2F};
    XrResult res = xrGetActionStateVector2f(rlxr.session, &getInfo, &state);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to get vector2 action (result: %d; action: %d)", res, action);
        return (rlVector2State){ { 0.f, 0.f }, 0, 0 };
    }

    return (rlVector2State){ { state.currentState.x, state.currentState.y }, (bool)state.isActive, (bool)state.changedSinceLastSync };
}

Vector2 rlGetVector2(unsigned int action, rlActionDevices device) {
    rlVector2State state = rlGetVector2State(action, device);
    return state.active ? state.value : (Vector2){ 0.f, 0.f };
}

rlPoseState rlGetPoseState(unsigned int action, rlActionDevices device) {
    assert(rlxr.actionSetAttached);
    if (action == RLXR_NULL_ACTION) return (rlPoseState){ (rlPose){ {}, {}, 0, 0 }, 0 };

    rlxrAction *ac = &rlxr.actions[action];

    XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
    getInfo.action = ac->action;

    switch (device) {
    case RLXR_HAND_LEFT:
        getInfo.subactionPath = rlxr.userPaths[0];
        break;

    case RLXR_HAND_RIGHT:
        getInfo.subactionPath = rlxr.userPaths[1];
        break;

    default:
        TRACELOG(LOG_WARNING, "XR: Unsupported device in getPose (action: %d)", action);
        return (rlPoseState){ (rlPose){ {}, {}, 0, 0 }, 0 };
    }

    XrActionStatePose state = {XR_TYPE_ACTION_STATE_POSE};
    XrResult res = xrGetActionStatePose(rlxr.session, &getInfo, &state);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to get pose action (result: %d; action: %d)", res, action);
        return (rlPoseState){ (rlPose){ {}, {}, 0, 0 }, 0 };
    }

    if (!state.isActive) {
        // return null pose if device not active
        return (rlPoseState){ (rlPose){ {}, {}, 0, 0 }, false };
    }

    // locate pose space

    XrSpaceLocation location = {XR_TYPE_SPACE_LOCATION};
    res = xrLocateSpace(device == RLXR_HAND_LEFT ? ac->actionSpaces[0] : ac->actionSpaces[1], rlxr.referenceSpace, rlxr.frameState.predictedDisplayTime, &location);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to locate pose space (result: %d; action: %d)", res, action);
        return (rlPoseState){ (rlPose){ {}, {}, 0, 0 }, true };
    }

    return (rlPoseState){ xrPoseToRlPose(location.pose, location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT, location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT), (bool)state.isActive };
}

rlPose rlGetPose(unsigned int action, rlActionDevices device) {
    rlPoseState state = rlGetPoseState(action, device);
    return state.active ? state.value : (rlPose){ {}, {}, 0, 0 };
}

void rlApplyHaptic(unsigned int action, rlActionDevices device, long duration, float amplitude) {
    assert(rlxr.actionSetAttached);
    if (action == RLXR_NULL_ACTION) return;

    rlxrAction *ac = &rlxr.actions[action];

    XrHapticActionInfo hapticInfo = {XR_TYPE_HAPTIC_ACTION_INFO};
    hapticInfo.action = ac->action;

    switch (device) {
    case RLXR_HAND_LEFT:
        hapticInfo.subactionPath = rlxr.userPaths[0];
        break;

    case RLXR_HAND_RIGHT:
        hapticInfo.subactionPath = rlxr.userPaths[1];
        break;

    default:
        TRACELOG(LOG_WARNING, "XR: Unsupported device in applyHaptic (action: %d)", action);
        return;
    }

    XrHapticVibration vibration = {XR_TYPE_HAPTIC_VIBRATION};
    vibration.amplitude = amplitude;
    vibration.duration = duration;
    vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

    XrResult res = xrApplyHapticFeedback(rlxr.session, &hapticInfo, (XrHapticBaseHeader *)&vibration);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to apply haptic action (result: %d; action: %d)", res, action);
    }
}

#endif

/* clang-format on */
