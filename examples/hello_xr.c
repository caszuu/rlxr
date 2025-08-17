#include "raylib.h"
#include "raymath.h"

#define RLXR_IMPLEMENTATION
#include "rlxr.h"

static void drawScene();

int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "rlxr sample - hello xr");

    // Initialize the XR runtime and rlxr resources, exit if no XR runtime found
    bool success = InitXr();
    if (!success) {
        return -1;
    }

    // Position the XR play space and the player in the scene
    SetXrPosition((Vector3){ 0.0f, 1.5f, 1.5f });

    // Define a camera to mirror the XR view for the flatscreen window
    Camera camera = { 0 };
    camera.position = (Vector3){ 0.0f, 1.5f, 1.5f };   // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };     // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };         // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                               // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;            // Camera projection type

    // let the XR runtime pace the frame loop on its own (blocks in UpdateXr)
    SetTargetFPS(-1);

    while (!WindowShouldClose()) {
        // Update
        //----------------------------------------------------------------------------------
      
        // Update internal XR event loop, this needs to be done every frame before BeginXrMode
        UpdateXr();

        // Get the pose (position and rotation) of the XR hmd (usually the centroid between XR views above)
        rlPose viewPose = GetXrViewPose();

        // Update flatscreen camera to mirror the XR hmd (if the hmd is being tracked)
        if (viewPose.isPositionValid) {
            camera.position = viewPose.position;
        }

        if (viewPose.isOrientationValid) {
            // camera conversion snippet from https://github.com/FireFlyForLife/rlOpenXR/blob/2fd2433eec8a096dd67c26c94671c4976f9b7dd8/src/rlOpenXR.cpp#L869

            camera.target = Vector3Add(Vector3RotateByQuaternion((Vector3){0.0f, 0.0f, -1.0f}, viewPose.orientation), viewPose.position);
            camera.up = Vector3RotateByQuaternion((Vector3){0.0f, 1.0f, 0.0f}, viewPose.orientation);
        }

        // Draw to XR
        //----------------------------------------------------------------------------------

        // Begin new XR frame, the number of views (cameras) to render requested by the XR runtime is returned.
        int views = BeginXrMode();

        // note: currently the number of views can be either 2 (stereoscopic rendering) or 0 (no rendering required, eg. the app is not visible in headset). View index 0 will
        //       always be the left eye and index 1 will always be the right eye.

        for (int i = 0; i < views; i++) {
            // Begin a XR view, this will setup the framebuffer and 3D rendering from the perspective of the view
            BeginView(i);
            
                ClearBackground(RAYWHITE);
                drawScene();

            // Release the XR view to the runtime and disable 3D rendering
            EndView();
        }

        // Release the frame to the runtime and show it on device
        EndXrMode();

        // Draw to screen
        //----------------------------------------------------------------------------------

        BeginDrawing();

            ClearBackground(RAYWHITE);
        
            BeginMode3D(camera);
                drawScene();
            EndMode3D();

            DrawFPS(10, 10);
        
        EndDrawing();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseXr(); // clean up and close connection with XR runtime
    CloseWindow();
}

static void drawScene() {
    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };
  
    DrawCube(cubePosition, 0.5f, 0.5f, 0.5f, RED);
    DrawCubeWires(cubePosition, 0.5f, 0.5f, 0.5f, MAROON);

    DrawGrid(10, 0.25f);
}

