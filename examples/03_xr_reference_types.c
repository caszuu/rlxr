#include "raylib.h"
#include "raymath.h"

#define RLXR_APP_NAME "Rlxr example - reference types"
#define RLXR_IMPLEMENTATION
#include "rlxr.h"

#include <stdio.h>

static void drawScene();

/* clang-format off */

int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Rlxr example - reference types");

    // Initialize the XR runtime, exit if no XR runtime found
    bool success = InitXr();
    if (!success)
    {
        return -1;
    }

    // Position the XR play space on the center on the grid
    SetXrPosition((Vector3){0.0f, 0.0f, 0.0f});

    // Define a camera to mirror the XR view for the flatscreen window
    Camera camera = {0};
    camera.position = (Vector3){0.0f, 1.5f, 1.5f}; // Camera position
    camera.target = (Vector3){0.0f, 1.0f, 0.0f};   // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};       // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                           // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;        // Camera projection type

    // Setup a Reference type
    //----------------------------------------------------------------------------------

    // When using positions tracked by a XR device, they must always be relative to a pre-defined point in real world space for them to have any meaning.
    // In OpenXR, this point is called a _reference space_ and it defines what the coordinate origin maps to in the real world play space, OpenXR offers
    // 3 types of reference space _types_ with varying guarantees and availability.

    // The LOCAL reference type is the most basic one and is *always* available. (will always return true)
    // It defines the coordinate origin (for all X, Y and Z axis) as the initial position of the headset
    // after app start or after the last recentering. This is useful if you do not care where the floor is.
    bool localSuccess = SetXrReference(RLXR_REFERENCE_LOCAL);

    // The LOCAL_FLOOR reference type is an extension of LOCAL. It defines the coordinate origin same as LOCAL
    // but with the exception that Y == 0 is the best estimate of where the users floor is. This reference type
    // should be preferred in most cases and is the default chosen on init if available.
    //
    // It is available on the *vast* majority of XR devices, but it might be missing on some older hardware.
    bool localFloorSuccess = SetXrReference(RLXR_REFERENCE_LOCAL_FLOOR);

    // The STAGE reference type is more specialized. It defines the coordinate origin as the center of the users
    // play space boundary for X and Z and the floor level for Y. This reference type is useful for full room-scale
    // experiences where the virtual world is centered around the users available play area. As a side effect though,
    // headset re-centering will *not* do anything as by definition the reference origin cannot be moved until the user
    // changes their play space boundaries.
    //
    // The availability of this reference type is always *optional* and can change through-out the lifetime of the app
    // as the user switches between room-scale and seated / standing tracking. Always check the return bool to check if
    // the switch was successful.
    bool stageSuccess = SetXrReference(RLXR_REFERENCE_STAGE);

    printf("Reference type support; local: %d local_floor: %d stage: %d\n", localSuccess, localFloorSuccess, stageSuccess);

    // You can always fetch what reference type you're currently using.
    rlReferenceType refType = GetXrReference();
    switch (refType)
    {
    case RLXR_REFERENCE_LOCAL:
        printf("Reference type after init: LOCAL\n");
        break;

    case RLXR_REFERENCE_LOCAL_FLOOR:
        printf("Reference type after init: LOCAL_FLOOR\n");
        break;

    case RLXR_REFERENCE_STAGE:
        printf("Reference type after init: STAGE\n");
        break;
    }

    int lastType = -1;

    while (!WindowShouldClose() && IsXrConnected())
    {
        // Update
        //----------------------------------------------------------------------------------

        // Update internal XR event loop, this needs to be done every frame before BeginXrMode()
        UpdateXr();

        // Get the pose (position and rotation) of the XR hmd (usually the centroid between XR views bellow)
        rlPose viewPose = GetXrViewPose();

        // Update flatscreen camera to mirror the XR hmd (if the hmd is being tracked)
        if (viewPose.isPositionValid)
        {
            camera.position = viewPose.position;
        }

        if (viewPose.isOrientationValid)
        {
            // camera conversion snippet from https://github.com/FireFlyForLife/rlOpenXR/blob/2fd2433eec8a096dd67c26c94671c4976f9b7dd8/src/rlOpenXR.cpp#L869

            camera.target = Vector3Add(Vector3RotateByQuaternion((Vector3){0.0f, 0.0f, -1.0f}, viewPose.orientation), viewPose.position);
            camera.up = Vector3RotateByQuaternion((Vector3){0.0f, 1.0f, 0.0f}, viewPose.orientation);
        }

        // Switch between available reference types every 10 seconds
        if ((int)GetTime() / 10 % 3 == 0 && lastType != 0)
        {
            bool ok = SetXrReference(RLXR_REFERENCE_LOCAL);
            if (ok) printf("Reference type in use: LOCAL\n");

            lastType = 0;
        } else if ((int)GetTime() / 10 % 3 == 1 && lastType != 1)
        {
            bool ok = SetXrReference(RLXR_REFERENCE_LOCAL_FLOOR);
            if (ok) printf("Reference type in use: LOCAL_FLOOR\n");

            lastType = 1;
        } else if ((int)GetTime() / 10 % 3 == 2 && lastType != 2)
        {
            bool ok = SetXrReference(RLXR_REFERENCE_STAGE);
            if (ok) printf("Reference type in use: STAGE\n");

            lastType = 2;
        }

        // Draw to XR
        //----------------------------------------------------------------------------------

        // Begin new XR frame, the number of views (cameras) to render requested by the XR runtime is returned.
        int views = BeginXrMode();

        for (int i = 0; i < views; i++)
        {
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

            rlReferenceType type = GetXrReference();
            if (type == RLXR_REFERENCE_LOCAL) DrawText(TextFormat("Ref Type: LOCAL"), 10, 35, 20, BLACK);
            if (type == RLXR_REFERENCE_LOCAL_FLOOR) DrawText(TextFormat("Ref Type: LOCAL_FLOOR"), 10, 35, 20, BLACK);
            if (type == RLXR_REFERENCE_STAGE) DrawText(TextFormat("Ref Type: STAGE"), 10, 35, 20, BLACK);

        EndDrawing();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseXr();
    CloseWindow();
}

/* clang-format on */

static void drawScene() {
    Vector3 cubePosition = {2.5f, 0.0f, 2.5f};

    DrawCube(cubePosition, 0.5f, 0.5f, 0.5f, RED);
    DrawCubeWires(cubePosition, 0.5f, 0.5f, 0.5f, MAROON);

    cubePosition = (Vector3){-2.5f, 0.0f, -2.5f};

    DrawCube(cubePosition, 0.5f, 0.5f, 0.5f, BLUE);
    DrawCubeWires(cubePosition, 0.5f, 0.5f, 0.5f, SKYBLUE);

    DrawGrid(20, 0.25f);
}
