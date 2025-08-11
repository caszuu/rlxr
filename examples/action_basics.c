#include "raylib.h"
#include "raymath.h"

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

int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "rlxr sample - xr action basics");

    // Initialize the XR runtime, exit if no XR runtime found
    bool success = InitXr();
    if (!success) {
        return -1;
    }

    // Position the XR play space and the player in the scene
    SetXrPosition((Vector3){ 0.0f, 1.5f, 0.0f });

    // Define a camera to mirror the XR view for the flatscreen window
    Camera camera = { 0 };
    camera.position = (Vector3){ 0.0f, 1.5f, 0.0f };   // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };     // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };         // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                               // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;            // Camera projection type

    // Setup Actions
    //--------------------------------------------------------------------------------------

    // In OpenXR, an input is made from two parts: an _action_ and its _bindings_. First an app creates an
    // _action_ which describes an input controlling a specific part of the app, an example might be
    // a "aim" pose action, a "walk" Vector2 action and a "fire" boolean action together controlling a fps player.

    // create a boolean action valid only for the left controller
    int menu = rlLoadAction("menu-example", RLXR_TYPE_BOOLEAN, RLXR_HAND_LEFT);

    // then to bind the _action_ to a specific hardware component, a _binding_ must be suggested. This _binding_
    // *may* be ignored / remapped by the Xr runtime. (eg. remapped in the SteamVR Controller Bindings UI)
    // Each _action_ must have at least one suggested _binding_ but multiple _bindings_ can also be suggested.

    // bind the color action with the menu component (maps to the menu or home button on most controllers)
    rlSuggestBinding(menu, RLXR_COMPONENT_MENU);

    // create another boolean action and bind it to the select component (for both controllers)
    int select = rlLoadAction("select-example", RLXR_TYPE_BOOLEAN, RLXR_HAND_BOTH);
    rlSuggestBinding(select, RLXR_COMPONENT_SELECT); // (maps to the trigger on most controllers)

    // create a pose action, this action will be used for fetching the position and rotation of both controllers
    int pose = rlLoadAction("controller-pose", RLXR_TYPE_POSE, RLXR_HAND_BOTH);
    rlSuggestBinding(pose, RLXR_COMPONENT_GRIP_POSE);

    // note: we used the grip pose, but the aim pose is also available (see https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#semantic-paths-standard-pose-identifiers)
    // WARNING: all actions and bindings must be created and suggested *before* the first UpdateXr() call

    WorldState world = {};

    // Setup text panel resources
    RenderTexture2D panelTarget = LoadRenderTexture(800, 450);

    world.textPanel.mesh = GenMeshPlane(2.f, -1.125f, 1, 1);
    world.textPanel.mat = LoadMaterialDefault();
    SetMaterialTexture(&world.textPanel.mat, MATERIAL_MAP_DIFFUSE, panelTarget.texture);

    while (!WindowShouldClose()) {
        // Update
        //----------------------------------------------------------------------------------
      
        // Update all action states and internal XR event loop, this needs to be done every frame
        UpdateXr();

        // Update the flatscreen camera from the Hmd view pose

        rlPose viewPose = GetXrViewPose();

        if (viewPose.isPositionValid) {
            camera.position = viewPose.position;
        }

        if (viewPose.isOrientationValid) {
            // camera conversion snippet from https://github.com/FireFlyForLife/rlOpenXR/blob/2fd2433eec8a096dd67c26c94671c4976f9b7dd8/src/rlOpenXR.cpp#L869

            camera.target = Vector3Add(Vector3RotateByQuaternion((Vector3){0.0f, 0.0f, -1.0f}, viewPose.orientation), viewPose.position);
            camera.up = Vector3RotateByQuaternion((Vector3){0.0f, 1.0f, 0.0f}, viewPose.orientation);
        }

        // fetch action values from a source device, if the source device is inactive, a zero-like value is returned
        // note: a rlGet* call can only fetch from a single device at once so a source device must be passed in
        bool leftSelectPressed = rlGetBool(select, RLXR_HAND_LEFT);
        bool rightSelectPressed = rlGetBool(select, RLXR_HAND_RIGHT);
        
        // fetch a full action state from a source device, this is identical to the simpler version above but it adds a few bits about the source device
        rlBoolState menuState = rlGetBoolState(menu, RLXR_HAND_LEFT);
        
        // menuState.value - the current input value
        // menuState.active - is there a component / binding that is awake and active for this action (eg. false if the source controller is sleeping)
        // menuState.changed - did .value change since the last UpdateXr() call

        // fetch controller poses for rendering in-app models
        world.hands[0] = rlGetPose(pose, RLXR_HAND_LEFT);
        world.hands[1] = rlGetPose(pose, RLXR_HAND_RIGHT);

        // Draw 2D text panel into a texture

        BeginTextureMode(panelTarget);

            ClearBackground(BLANK); // (fully transparent)
        
            DrawText(TextFormat("Select - left: %b right: %b", leftSelectPressed, rightSelectPressed), 32, 128, 26, BLACK);
            DrawText(TextFormat("Menu state - value: %b active: %b changed: %b", menuState.value, menuState.active, menuState.changed), 32, 154, 26, BLACK);

            DrawText("Left Hand:", 32, 200, 26, BLACK);
            DrawText(TextFormat("    valid: %b; position: %.04f %.04f %.04f", world.hands[0].isPositionValid, world.hands[0].position.x, world.hands[0].position.y, world.hands[0].position.z), 32, 226, 26, BLACK);
            DrawText(TextFormat("    valid: %b; orientation: %.04f %.04f %.04f %.04f", world.hands[0].isOrientationValid, world.hands[0].orientation.x, world.hands[0].orientation.y, world.hands[0].orientation.z, world.hands[0].orientation.w), 32, 252, 26, BLACK);

            DrawText("Right Hand:", 32, 300, 26, BLACK);
            DrawText(TextFormat("    valid: %b; position: %.04f %.04f %.04f", world.hands[1].isPositionValid, world.hands[1].position.x, world.hands[1].position.y, world.hands[1].position.z), 32, 326, 26, BLACK);
            DrawText(TextFormat("    valid: %b; orientation: %.04f %.04f %.04f %.04f", world.hands[1].isOrientationValid, world.hands[1].orientation.x, world.hands[1].orientation.y, world.hands[1].orientation.z, world.hands[1].orientation.w), 32, 352, 26, BLACK);

            DrawRectangleLines(16, 112, 784, 282, BLACK);

        EndTextureMode();

        // Draw to XR
        //----------------------------------------------------------------------------------

        // Begin new XR frame, the number of views (cameras) to render requested by the XR runtime is returned.
        int views = BeginXrMode();

        for (int i = 0; i < views; i++) {
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

static void drawScene(WorldState *world) {
    // draw controller cubes

    for (int i = 0; i < 2; i++) {
        rlPose hand = world->hands[i];

        if (hand.isPositionValid && hand.isOrientationValid) {
            rlPushMatrix();
                rlTranslatef(hand.position.x, hand.position.y, hand.position.z);
                rlMultMatrixf(MatrixToFloat(QuaternionToMatrix(hand.orientation)));

                DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, .08f, .1f, .12f, i == 1 ? ORANGE : BLUE);
                DrawCubeWires((Vector3){ 0.0f, 0.0f, 0.0f }, .08f, .1f, .12f, i == 1 ? RED : DARKBLUE);
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
