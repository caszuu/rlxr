#include "raylib.h"
#include "raymath.h"

#define RLXR_APP_NAME "Rlxr example - interaction profiles"
#define RLXR_IMPLEMENTATION
#include "rlxr.h"

typedef struct {
    Mesh mesh;
    Material mat;
} Panel;

typedef struct {
    rlPose hands[2];
    Panel textPanel;
} WorldState;

static void drawScene(WorldState *state);

/* clang-format off */

int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Rlxr example - interaction profiles");

    // Initialize the XR runtime, exit if no XR runtime found
    bool success = InitXr();
    if (!success)
    {
        return -1;
    }

    // Position the XR play space and the player in the scene
    SetXrPosition((Vector3){0.0f, 0.0f, 0.0f});

    // Define a camera to mirror the XR view for the flatscreen window
    Camera camera = {0};
    camera.position = (Vector3){0.0f, 1.5f, 1.5f}; // Camera position
    camera.target = (Vector3){0.0f, 1.0f, 0.0f};   // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};       // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                           // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;        // Camera projection type

    // Setup Actions and Profiles
    //--------------------------------------------------------------------------------------

    // While binding with the simplified rlSuggestBinding may work for some simple cases,
    // if we need every input that the device controllers provide we need to make use of
    // _interaction profiles_.
    //
    // Interaction profiles are a way of _binding_ actions to hardware and vendor specific
    // components directly in a forward compatible manor. An app can provide bindings for _all_
    // profiles it supports and the XR runtime is then free to choose which profile to use and to
    // potentially remap them for newer hardware. (eg. old oculus profiles for newer meta headsets)
    //
    // This gives us full access to the controllers, but we also need to express them using
    // OpenXR paths specific to each profile. All supported profiles by OpenXR can be found in the
    // spec (section 6.4, here https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#semantic-paths-interaction-profiles)

    // == setup actions ==

    // create a boolean action valid only for the left controller
    int menu = rlLoadAction("menu", RLXR_TYPE_BOOLEAN, RLXR_HAND_BOTH);

    int walk = rlLoadAction("walk", RLXR_TYPE_VECTOR2F, RLXR_HAND_BOTH);
    int fire = rlLoadAction("fire-gun", RLXR_TYPE_BOOLEAN, RLXR_HAND_BOTH);
    int hold = rlLoadAction("hold-item", RLXR_TYPE_FLOAT, RLXR_HAND_BOTH);

    int haptic = rlLoadAction("haptic", RLXR_TYPE_VIBRATION, RLXR_HAND_BOTH);
    int grip = rlLoadAction("grip-pose", RLXR_TYPE_POSE, RLXR_HAND_BOTH);

    // == touch controllers ==

    rlSuggestProfile("/interaction_profiles/oculus/touch_controller");

    // have to use two bindings for menu due to it having hand-specific bidnings on each hand
    rlSuggestBindingPro(menu, RLXR_HAND_LEFT, "/input/x/click");
    rlSuggestBindingPro(menu, RLXR_HAND_RIGHT, "/input/a/click");

    rlSuggestBindingPro(walk, RLXR_HAND_LEFT, "/input/thumbstick");
    rlSuggestBindingPro(fire, RLXR_HAND_RIGHT, "/input/trigger/value");
    rlSuggestBindingPro(hold, RLXR_HAND_BOTH, "/input/squeeze/value");

    rlSuggestBindingPro(haptic, RLXR_HAND_BOTH, "/output/haptic");
    rlSuggestBindingPro(grip, RLXR_HAND_BOTH, "/input/grip/pose");

    // == index controllers ==

    rlSuggestProfile("/interaction_profiles/valve/index_controller");

    rlSuggestBindingPro(menu, RLXR_HAND_BOTH, "/input/a/click"); // both hands can use the same binding here
    rlSuggestBindingPro(walk, RLXR_HAND_BOTH, "/input/thumbstick");
    rlSuggestBindingPro(fire, RLXR_HAND_RIGHT, "/input/trigger/value");
    rlSuggestBindingPro(hold, RLXR_HAND_BOTH, "/input/squeeze/value");

    rlSuggestBindingPro(haptic, RLXR_HAND_BOTH, "/output/haptic");
    rlSuggestBindingPro(grip, RLXR_HAND_BOTH, "/input/grip/pose");

    // == vive controllers ==

    rlSuggestProfile("/interaction_profiles/htc/vive_controller");

    rlSuggestBindingPro(menu, RLXR_HAND_BOTH, "/input/menu/click");
    rlSuggestBindingPro(walk, RLXR_HAND_LEFT, "/input/trackpad"); // missing thumbsticks here, must use a trackpad.. for better or worse
    rlSuggestBindingPro(fire, RLXR_HAND_RIGHT, "/input/trigger/value");
    rlSuggestBindingPro(hold, RLXR_HAND_BOTH, "/input/squeeze/click"); // only /click available, no float support

    rlSuggestBindingPro(haptic, RLXR_HAND_BOTH, "/output/haptic");
    rlSuggestBindingPro(grip, RLXR_HAND_BOTH, "/input/grip/pose");

    // ... add profiles as needed, or better load them from a user-configurable bindings file ...

    // WARNING: rlSuggestProfile can be called only *once* per profile
    // WARNING: all actions and bindings must be created and suggested *before* the first UpdateXr() call

    // Setup text panel resources
    //--------------------------------------------------------------------------------------

    RenderTexture2D panelTarget = LoadRenderTexture(580, 120);
    WorldState world = {};

    world.textPanel.mesh = GenMeshPlane(1.0f, -0.25f, 1, 1);
    world.textPanel.mat = LoadMaterialDefault();
    SetMaterialTexture(&world.textPanel.mat, MATERIAL_MAP_DIFFUSE, panelTarget.texture);

    float lastTime = GetTime();

    while (!WindowShouldClose() && IsXrConnected())
    {
        // Update
        //----------------------------------------------------------------------------------

        // Update all action states and internal XR event loop, this needs to be done every frame
        UpdateXr();

        // Update the flatscreen camera from the Hmd view pose

        rlPose viewPose = GetXrViewPose();

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

        // update player position based on walk input

        rlVector2State walkState = rlGetVector2State(walk, RLXR_HAND_LEFT);

        if (viewPose.isOrientationValid && walkState.active)
        {
            Vector3 forward = Vector3RotateByQuaternion((Vector3){0.0f, 0.0f, -1.0f}, viewPose.orientation);
            forward = Vector3Multiply(forward, (Vector3){1.0f, 0.0f, 1.0f});
            forward = Vector3Normalize(forward);

            Vector3 right = {-forward.z, 0.0f, forward.x};
            Vector3 step = Vector3Add(Vector3Scale(right, walkState.value.x), Vector3Scale(forward, walkState.value.y));

            rlPose currentPose = GetXrPose();
            SetXrPosition(Vector3Add(currentPose.position, Vector3Scale(step, GetTime() - lastTime /*delta time*/)));
        }

        // fetch action states from a source devices
        bool menuPressed = rlGetBool(menu, RLXR_HAND_LEFT) | rlGetBool(menu, RLXR_HAND_RIGHT);

        // fetch controller poses for rendering in-app models
        world.hands[0] = rlGetPose(grip, RLXR_HAND_LEFT);
        world.hands[1] = rlGetPose(grip, RLXR_HAND_RIGHT);

        // feed to hold float into the haptic output
        float leftHold = rlGetFloat(hold, RLXR_HAND_LEFT);
        rlApplyHaptic(haptic, RLXR_HAND_LEFT, -1 /* == minimal supported by runtime*/, leftHold);

        float rightHold = rlGetFloat(hold, RLXR_HAND_RIGHT);
        rlApplyHaptic(haptic, RLXR_HAND_RIGHT, -1, rightHold);

        lastTime = GetTime();

        // Draw 2D text panel into a texture

        BeginTextureMode(panelTarget);

            ClearBackground(BLANK); // (fully transparent)

            DrawText("Walk around with your left thumbstick!\nOr squeeze to vibrate your controllers.", 32, 32, 24, BLACK);
            DrawRectangleLines(1, 1, 560 - 2, 120 - 2, BLACK);

        EndTextureMode();

        // Draw to XR
        //----------------------------------------------------------------------------------

        // Begin new XR frame, the number of views (cameras) to render requested by the XR runtime is returned.
        int views = BeginXrMode();

        for (int i = 0; i < views; i++)
        {
            // Begin a XR view, this will setup 3D rendering from the perspective of the view
            BeginView(i);

                ClearBackground(RAYWHITE);
                drawScene(&world);

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
                drawScene(&world);
            EndMode3D();

            DrawFPS(10, 10);

        EndDrawing();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseXr();
    CloseWindow();
}

/* clang-format on */

static void drawScene(WorldState *world) {
    // draw controller cubes

    for (int i = 0; i < 2; i++)
    {
        rlPose hand = world->hands[i];

        if (hand.isPositionValid && hand.isOrientationValid)
        {
            rlPushMatrix();
            rlTranslatef(hand.position.x, hand.position.y, hand.position.z);
            rlMultMatrixf(MatrixToFloat(QuaternionToMatrix(hand.orientation)));

            DrawCube((Vector3){0.0f, 0.0f, 0.0f}, .08f, .1f, .12f, i == 1 ? ORANGE : BLUE);
            DrawCubeWires((Vector3){0.0f, 0.0f, 0.0f}, .08f, .1f, .12f, i == 1 ? RED : DARKBLUE);
            rlPopMatrix();
        }
    }

    // draw 3D scene

    rlPushMatrix();

    rlTranslatef(.0f, 1.2f, -1.5f);
    rlRotatef(90.f, 1.f, .0f, .0f);

    rlSetCullFace(RL_CULL_FACE_FRONT);
    DrawMesh(world->textPanel.mesh, world->textPanel.mat, MatrixIdentity());
    rlSetCullFace(RL_CULL_FACE_BACK);

    rlPopMatrix();

    DrawGrid(10, 0.25f);
}
