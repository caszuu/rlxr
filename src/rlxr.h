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
    #include "utils.h"
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

#ifndef TRACELOG
    #define TRACELOG(level, ...) (void)0
    #define TRACELOGD(...) (void)0
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

typedef enum {
    RLXR_STATE_UNKNOWN = 0,
    RLXR_STATE_IDLE,
    RLXR_STATE_SYNCHRONIZED,
    RLXR_STATE_VISIBLE,
    RLXR_STATE_FOCUSED,
} rlXrState;

typedef struct {
    Vector3 position;
    Quaternion orientation;

    bool isPositionValid;
    bool isOrientationValid;
} rlPose;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif

RLAPI bool InitXr(); // returns true if successful, *must* be called after InitWindow or rlglInit
RLAPI void CloseXr();

// Session state
RLAPI bool IsXrConnected();   // returns true after InitXr(), returns false after CloseXr() or a fatal XR error
RLAPI bool IsXrFocused();     // returns true if the XR device is awake and displaying rendered frames
RLAPI rlXrState GetXrState(); // returns the current XR session state

// Spaces and Poses
RLAPI rlPose GetViewPose(); // returns the pose of the users view (eg the position and orientation of the hmd in the wider refrence frame)
RLAPI void SetXrPosition(Vector3 pos); // sets the offset of the _refrence_ frame, this offsets the entire play space (including the users camera / views) by [pos] allowing you to move the player though-out the virtual space
RLAPI void SetXrOrientation(Quaternion quat);
RLAPI rlPose GetXrPose(); // fetches the current refrence frame offsets

// View Rendering
RLAPI int BeginXrMode(); // returns the number of views that are requested by the xr runtime (returns 0 if rendering is not required by the runtime, eg. app is not visible to user)
RLAPI void EndXrMode();  // end and submit frame, must be called even when 0 views are requested
RLAPI void BeginView(unsigned int index); // begin view with index in range [0, request_count), a view is always rendered in 3D with an internal camera
RLAPI void EndView();    // finish and submit view

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
    // #define GLFW_EXPOSE_NATIVE_WAYLAND
    // #define GLFW_NATIVE_INCLUDE_NONE
    // #include <GLFW/glfw3native.h>
#endif

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
} rlxrViewBuffers;

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

    // spaces //

    XrSpace referenceSpace;

    Vector3 refPosition;
    Quaternion refOrientation;

    XrSpace viewSpace;   

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
    strncpy(appInfo.applicationName, "rlxr app", XR_MAX_APPLICATION_NAME_SIZE);
    appInfo.applicationVersion = 1;
    strncpy(appInfo.engineName, "raylib", XR_MAX_APPLICATION_NAME_SIZE);
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
    //         - GL/Wayland (independent context?)

#ifdef XR_USE_PLATFORM_WIN32
    XrGraphicsBindingOpenGLWin32KHR rlglInfo = {XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR};
    rlglInfo.hDC = wglGetCurrentDC();
    rlglInfo.HGLRC = wglGetCurrentContext();
#endif

#ifdef XR_USE_PLATFORM_XLIB
    XrGraphicsBindingOpenGLXlibKHR rlglInfo = {XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};
    rlglInfo.xDisplay = XOpenDisplay(NULL);
    rlglInfo.glxContext = glXGetCurrentContext();
    rlglInfo.glxDrawable = glXGetCurrentDrawable();
#endif

// #ifdef XR_USE_PLATFORM_WAYLAND
//     XrGraphicsBindingOpenGLWaylandKHR rlglInfo{XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR};
//     rlglInfo.display = glfwGetWaylandDisplay();
// #endif
    
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

    if (depthFormat < 0) {
        TRACELOG(LOG_WARNING, "XR: Preferred depth format not supported");
        return false; // TODO: support depth-less rendering
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

    for (int i = 0; i < rlxr.viewCount; i++) {
        rlxr.viewBufs[i].framebuffer = rlLoadFramebuffer();
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

    rlxr.depthInfoViews = (XrCompositionLayerDepthInfoKHR *)RL_MALLOC(rlxr.viewCount * sizeof(XrCompositionLayerDepthInfoKHR));
    for (int i = 0; i < rlxr.viewCount; i++) {
        rlxr.depthInfoViews[i].type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;
        rlxr.depthInfoViews[i].next = NULL;
        rlxr.depthInfoViews[i].minDepth = 0.f; // TODO: sync with rlgl clip mode
        rlxr.depthInfoViews[i].maxDepth = 1.f;

        rlxr.depthInfoViews[i].subImage.swapchain = rlxr.viewBufs[i].depthSwapchain;
        rlxr.depthInfoViews[i].subImage.imageArrayIndex = 0;
        rlxr.depthInfoViews[i].subImage.imageRect.offset.x = 0;
        rlxr.depthInfoViews[i].subImage.imageRect.offset.y = 0;
        rlxr.depthInfoViews[i].subImage.imageRect.extent.width = rlxr.viewProps[i].recommendedImageRectWidth;
        rlxr.depthInfoViews[i].subImage.imageRect.extent.height = rlxr.viewProps[i].recommendedImageRectHeight;

        // depth info is chained to projection, not submitted as separate layer
        rlxr.projectionViews[i].next = &rlxr.depthInfoViews[i];
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

    for (int i = 0; i < rlxr.viewCount; i++) {
        RL_FREE(rlxr.viewBufs[i].colorImages);
        RL_FREE(rlxr.viewBufs[i].depthImages);
        
        rlUnloadFramebuffer(rlxr.viewBufs[i].framebuffer);
        xrDestroySwapchain(rlxr.viewBufs[i].colorSwapchain);
        xrDestroySwapchain(rlxr.viewBufs[i].depthSwapchain);
    }

    RL_FREE(rlxr.projectionViews);
    RL_FREE(rlxr.views);
    RL_FREE(rlxr.viewBufs);

    xrDestroySpace(rlxr.viewSpace);
    xrDestroySpace(rlxr.referenceSpace);

    xrDestroySession(rlxr.session);
    xrDestroyInstance(rlxr.instance);

    rlxr.instance = XR_NULL_HANDLE;
}

void UpdateXr() {
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

rlPose GetViewPose() {
    XrSpaceLocation location = {XR_TYPE_SPACE_LOCATION};

    XrResult res = xrLocateSpace(rlxr.viewSpace, rlxr.referenceSpace, rlxr.frameState.predictedDisplayTime /*FIXME: get current time*/, &location);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: failed to locate view space (%d)", res);
    }

    rlPose pose;
    pose.position = (Vector3){0.f, 0.f, 0.f};
    pose.orientation = (Quaternion){0.f, 0.f, 0.f, 1.f};
    pose.isPositionValid = false;
    pose.isOrientationValid = false;

    if (location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
        pose.position = (Vector3){location.pose.position.x, location.pose.position.y, location.pose.position.z};
        pose.position = Vector3Add(rlxr.refPosition, pose.position);
        pose.isPositionValid = true;
    }

    if (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
        pose.orientation = (Quaternion){location.pose.orientation.x, location.pose.orientation.y, location.pose.orientation.z, location.pose.orientation.w};
        pose.orientation = QuaternionMultiply(rlxr.refOrientation, pose.orientation);
        pose.isOrientationValid = true;
    }

    return pose;
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

    // sync with xr runtime

    rlxr.frameState.type = XR_TYPE_FRAME_STATE;
    XrFrameWaitInfo waitInfo = {XR_TYPE_FRAME_WAIT_INFO};

    XrResult res = xrWaitFrame(rlxr.session, &waitInfo, &rlxr.frameState);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to wait for a frame (%d)", res);
    }

    // locate view poses

    XrViewState viewState = {XR_TYPE_VIEW_STATE};
    XrViewLocateInfo locateInfo = {XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = rlxr.viewConfig;
    locateInfo.displayTime = rlxr.frameState.predictedDisplayTime;
    locateInfo.space = rlxr.referenceSpace;

    res = xrLocateViews(rlxr.session, &locateInfo, &viewState, rlxr.viewCount, &rlxr.viewCount, rlxr.views);
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

    res = xrAcquireSwapchainImage(view->depthSwapchain, &acqInfo, &depthAcquiredIndex);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to acquire an image from swapchain (%d)", res);
    }

    XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = 1000;

    res = xrWaitSwapchainImage(view->colorSwapchain, &waitInfo);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to wait for an image from swapchain (%d)", res);
    }
    
    res = xrWaitSwapchainImage(view->depthSwapchain, &waitInfo);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to wait for an image from swapchain (%d)", res);
    }

    // setup viewport and rlgl (very similar setup to BeginMode3D)

    rlDrawRenderBatchActive();

    rlxr.projectionViews[index].pose = rlxr.views[index].pose;
    rlxr.projectionViews[index].fov = rlxr.views[index].fov;
    rlxr.depthInfoViews[index].nearZ = rlGetCullDistanceNear();
    rlxr.depthInfoViews[index].farZ = rlGetCullDistanceFar();

    int w = rlxr.viewProps[index].recommendedImageRectWidth;
    int h = rlxr.viewProps[index].recommendedImageRectHeight;

    rlViewport(0, 0, w, h);
    rlScissor(0, 0, w, h);

    rlFramebufferAttach(view->framebuffer, view->colorImages[colorAcquiredIndex].image, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlFramebufferAttach(view->framebuffer, view->depthImages[depthAcquiredIndex].image, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

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

    res = xrReleaseSwapchainImage(rlxr.viewBufs[rlxr.viewActiveIndex].depthSwapchain, &relInfo);
    if (XR_FAILED(res)) {
        TRACELOG(LOG_ERROR, "XR: Failed to release a swapchain image (%d)", res);
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

#endif

/* clang-format on */
