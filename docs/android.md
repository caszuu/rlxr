# Building raymob with rlxr

Raymob is an excellent project template for android raylib applications. `rlxr` works with it almost out-of-the-box but some initial setup has to be done.

First, `git clone` a fresh raymob project using the standard guide in its README. Make sure you
can compile and build the basic example that comes with the fresh clone before proceeding.

Then, update to the latest `raylib` and pull-in `rlxr` to the project as a dependency
```shell
git submodule update --remote                                                           # update raylib from upstream
git submodule add -b main https://github.com/caszuu/rlxr.git app/src/main/cpp/deps/rlxr # add rlxr to your dependencies
```

Finally, patch the raymob project with a `rlxr` provided patch
```shell
git apply app/src/main/cpp/deps/rlxr/docs/raymob-xr.patch
```
This will configure raymob to link with OpenXR and `rlxr` and advertise your app to Android as an Immersive XR application. After building and installing to an OpenXR capable device, it should launch in immersive mode.

## Troubleshooting

### Application only opens a flatscreen window, OpenXR input still works
Make sure you have the proper `<intent-filter>` set in your `AndroidManifest.xml` as described in the manual configuration section. Android will not give you fullscreen / immersive access without proper intent filters.

### Application view is frozen when not focused / when an overlay is open
Make sure you have the latest `raylib` with `FLAG_WINDOW_ALWAYS_RUN` support under Android. You can pull-in the latest changes with
```shell
git submodules update --remote
```

## Manual raymob configuration

If you need to configure an existing raymob project or do not want to use the patch, this section describes all configuration that needs to be done for `rlxr` to work properly.

First, we need to add to OpenXR loader as a dependency and that is done in the `build.gradle` script in `app/src/`.
```diff
    buildFeatures {
        buildConfig true
        viewBinding true
+       prefab true
    }
}

/*

// Add your project's dependencies here.
// This section is used to include external libraries or modules that your project relies on.
// These dependencies will be automatically downloaded and integrated into the build process.
// To add a new dependency, use the 'implementation' or 'api' keyword followed by the library's coordinate or name.

+ */

dependencies {
+     implementation 'org.khronos.openxr:openxr_loader_for_android:1.1.54'
}

- */
```

Then we have to tell Android that this is a XR application, this is done in the manifest in `app/src/main/AndroidManifest.xml`. To allow for OpenXR to work we have to replace the built-in `<intent-filter>` with the following
```xml
<!-- Standard Khronos OpenXR launcher intent filter. -->
<intent-filter>
    <action android:name="android.intent.action.MAIN"/>
    <category android:name="android.intent.category.LAUNCHER"/>
    <category android:name="org.khronos.openxr.intent.category.IMMERSIVE_HMD"/>
</intent-filter>
            
<!-- Meta Quest-specific non-standard intent filter. -->
<intent-filter>
    <action android:name="android.intent.action.MAIN"/>
    <category android:name="android.intent.category.LAUNCHER"/>
    <category android:name="com.oculus.intent.category.VR"/>
</intent-filter>
```
Note this will also hide the flatscreen window on most devices, if you need both flatscreen and immersive XR, you will most likely need to use multiple Android activities.

Last, we also have to link `rlxr` itself with your project, this is done with cmake in `app/src/main/cpp/CMakeLists.txt`.
```diff
# Include raylib and raymob as a subdirectories
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/raylib)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/raymob)
+ add_subdirectory(${CMAKE_SOURCE_DIR}/deps/rlxr)

# Fetch all source files for your project (recursively), excluding 'deps' source files
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/*.c" "${CMAKE_SOURCE_DIR}/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/deps/.*")

# Add headers directory for android_native_app_glue.c
include_directories(${ANDROID_NDK}/sources/android/native_app_glue/)

# Add android_native_app_glue.c to the source files
list(APPEND SOURCES ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# Create a shared library with game source files
add_library(${APP_LIB_NAME} SHARED ${SOURCES})

# Define compiler macros for the library
target_compile_definitions(${APP_LIB_NAME} PRIVATE PLATFORM_ANDROID)

# Apply flags depending on the build type
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_definitions(${APP_LIB_NAME} PRIVATE _DEBUG DEBUG)
endif()

# Include raylib and raymob header files
target_include_directories(${APP_LIB_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/deps/raylib")
target_include_directories(${APP_LIB_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/deps/raymob")

# Link required libraries to the native application
- target_link_libraries(${APP_LIB_NAME} raylib raymoblib)
+ target_link_libraries(${APP_LIB_NAME} raylib raymoblib rlxr)
```