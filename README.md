# A minimalist Raylib OpenXR library
`rlxr` is a smol single-header module for integrating OpenXR with raylib and its ecosystem.

Currently only session management and rendering APIs are implemented and only Windows and Linux (`Xlib` only) are supported with Android being a possibility in the future. While OpenGL 3.3 might work, it's definitely recommended to build `rlgl` against OpenGL 4.3 as some OpenXR runtimes might require it.

Feature support:
- [x] XR Fullscreen Session
- [x] XR Rendering API
- [x] View / Reference Pose API

- [ ] Actions and ActionSets
- [ ] XR Overlay Session (`XR_EXTX_overlay`)
- [ ] AR Session (XR Environment Blend Mode API)
- [ ] Andoird / GLES support

## Building

As for other single-header libs, simply include it with a **single** translation unit having a `RLXR_IMPLEMENTATION` defined
```c
#include "raylib.h"

...

#define RLXR_IMPLEMENTATION
#include "rlxr.h"
```

On Linux, install the distros `openxr` equivalent package and link `-lGL -lX11 -lopenxr_loader` to your executable as dependencies. On Windows... good luck.

## API Cheatsheet

Session API:
```c
RLAPI bool InitXr(); // returns true if successful, *must* be called after InitWindow or rlglInit
RLAPI void CloseXr();

// Session state
RLAPI bool IsXrConnected(); // returns true after InitXr(), returns false after CloseXr() or a fatal XR error
RLAPI bool IsXrFocused(); // returns true if the XR device is awake and displaying rendered frames
RLAPI rlXrState GetXrState(); // returns the current XR session state

// Spaces and Poses
RLAPI rlPose GetViewPose(); // returns the pose of the users view (eg the position and orientation of the hmd in the wider refrence frame)
RLAPI void SetXrPosition(Vector3 pos); // sets the offset of the _refrence_ frame, this offsets the entire play space (including the users camera / views) by [pos] allowing you to move the player though-out the virtual space
RLAPI void SetXrOrientation(Quaternion quat);
RLAPI rlPose GetXrPose(); // fetches the current refrence frame offsets

```

Rendering API:
```c
RLAPI int BeginXrMode(); // returns the number of views that are requested by the xr runtime (returns 0 if rendering is not required by the runtime, eg. app is not visible to user)
RLAPI void EndXrMode();  // end and submit frame, must be called even when 0 views are requested
RLAPI void BeginView(unsigned int index); // begin view with index in range [0, request_count), a view is always rendered in 3D with an internal camera
RLAPI void EndView(); // finish and submit view
```
