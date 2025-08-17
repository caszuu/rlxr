# rlxr
`rlxr` is a smol single-header module for integrating OpenXR with Raylib and its ecosystem.

`rlxr` features a raylib-like subset of the OpenXR API that supports device-independent rendering and interaction with the binding based OpenXR input system.

Currently only Windows and Linux (`Xlib` only) platforms are supported. While OpenGL 3.3 might work, it's recommended to build your project against OpenGL 4.3 as some OpenXR runtimes might require it.

Feature support:
- [x] XR Fullscreen Session
- [x] XR Rendering API
- [x] View / Reference Pose API
- [x] Actions and Bindings
- [x] Interaction Profiles

- [ ] XR Overlay Session (`XR_EXTX_overlay`)
- [ ] AR Session (XR Environment Blend Mode API)
- [ ] Android / GLES support

## Usage and Building

As for other single-header modules, simply include `rlxr.h` with `RLXR_IMPLEMENTATION` being defined in a **single** translation unit.
```c
#include "raylib.h"

#define RLXR_IMPLEMENTATION
#include "rlxr.h"
```

To build a project with `rlxr` embedded, the OpenXR Loader together with platform specific dependencies will have to be linked. OpenXR can be installed using your distros `openxr` package or with the OpenXR-SDK for Windows. A helper CMake script is present at the root of the repository that will find and link all dependencies for you.

If you're using CMake for your project, simply clone the repo and link it to your project with:
```cmake
add_subdirectory(rlxr)
target_link_libraries(my_project PRIVATE rlxr)
```

## API Cheatsheet

Session API:
```c
RLAPI bool InitXr(); // returns true if successful, *must* be called after InitWindow or rlglInit
RLAPI void CloseXr();

// Session state
RLAPI void UpdateXr();        // updates internal XR state and actions, *must* be called every frame before BeginXrMode
RLAPI bool IsXrConnected();   // returns true after InitXr(), returns false after CloseXr() or a fatal XR error
RLAPI bool IsXrFocused();     // returns true if the XR device is awake and providing input to the app
RLAPI rlXrState GetXrState(); // returns the current XR session state

// Spaces and Poses
RLAPI rlPose GetXrViewPose();          // returns the pose of the users view (usually the centroid between XR views used in BeginView)
RLAPI void SetXrPosition(Vector3 pos); // sets the offset of the reference frame, this offsets the entire play space (including the users cameras / views) by [pos] allowing you to move the player throughout a virtual space
RLAPI void SetXrOrientation(Quaternion quat);
RLAPI rlPose GetXrPose();              // fetches the current reference frame offsets
```

Rendering API:
```c
RLAPI int BeginXrMode(); // returns the number of views that are requested by the xr runtime (returns 0 if rendering is not required by the runtime, eg. app is not visible to user)
RLAPI void EndXrMode();  // end and submit frame, *must* be called even when 0 views are requested
RLAPI void BeginView(unsigned int index); // begin view with index in range [0, request_count), this sets up 3D rendering with an internal camera matching the view
RLAPI void EndView();    // finish view and disable 3D rendering
```

Actions API:
```c
RLAPI unsigned int rlLoadAction(const char *name, rlActionType type, rlActionDevices devices); // register a new action with the XR runtime; [mustn't be called after first UpdateXr() call]
RLAPI void rlSuggestBinding(unsigned int action, rlActionComponent component); // suggest a binding for an action, this can be ignored / remapped by the XR runtime; [mustn't be called after first UpdateXr() call]

RLAPI void rlSuggestProfile(const char *profilePath); // select the interaction profile used for following binding suggestions (by default /interaction_profiles/khr/simple_controller), the same profile mustn't be selected twice; [mustn't be called after UpdateXr]
RLAPI void rlSuggestBindingPro(unsigned int action, rlActionDevices devices, const char *componentPath); // suggest a binding with a direct openxr component path; [mustn't be called after UpdateXr]

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
```
