#include "app.h"

#include "entity.h"
#include "raylib.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "box3d/box3d.h"

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

//----------------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------------
static constexpr int SCREEN_WIDTH = 1600;
static constexpr int SCREEN_HEIGHT = SCREEN_WIDTH * 9 / 16;

static RenderTexture2D target = {0};

static constexpr f32 TICK_HZ = 60.0f;
static constexpr f32 TICK_DT = 1.0f / TICK_HZ;

static Entity entity = {0};
static double accumulator = 0.0;

// Simple cube position (no physics yet)
static Vector3 cube_pos = {0, 0, 0};

// Camera
static Camera3D camera = {
        .position = {15, 8, 15}, .target = {0, 0, 0}, .up = {0, 1, 0}, .fovy = 45.0f, .projection = CAMERA_PERSPECTIVE};

// Box3D physics
static b3WorldId physics_world = {0};
static b3BodyId ground_body = {0};
static b3BodyId cube_body = {0};

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void App_UpdateDrawFrame(void);

//----------------------------------------------------------------------------------
// Main Loop
//----------------------------------------------------------------------------------
int App_Run(void) {
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);
#endif

    // Initialization
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "cgame");

    // Render texture for screen scaling
    target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    // Initialize game state
    Entity_Init(&entity);

    // Initialize Box3D physics world
    b3WorldDef world_def = b3DefaultWorldDef();
    world_def.gravity = (b3Vec3) {0, -9.81f, 0};
    physics_world = b3CreateWorld(&world_def);

    // Create ground body (static)
    b3BodyDef ground_def = b3DefaultBodyDef();
    ground_def.position = (b3Vec3) {0, -2, 0};
    ground_body = b3CreateBody(physics_world, &ground_def);

    // Add ground shape (thin sphere as platform)
    b3ShapeDef ground_shape = b3DefaultShapeDef();
    b3Sphere ground_sphere = {{0, 0, 0}, 0.5f};
    b3CreateSphereShape(ground_body, &ground_shape, &ground_sphere);

    // Create falling cube (dynamic)
    b3BodyDef cube_def = b3DefaultBodyDef();
    cube_def.type = b3_dynamicBody;
    cube_def.position = (b3Vec3) {0, 2, 0};
    cube_body = b3CreateBody(physics_world, &cube_def);

    // Add cube shape
    b3ShapeDef cube_shape = b3DefaultShapeDef();
    cube_shape.density = 1.0f;
    b3Sphere cube_sphere = {{0, 0, 0}, 0.5f};
    b3CreateSphereShape(cube_body, &cube_shape, &cube_sphere);

    // Apply mass from shapes
    b3Body_ApplyMassFromShapes(cube_body);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(App_UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose()) {
        App_UpdateDrawFrame();
    }
#endif

    // De-Initialization
    b3DestroyWorld(physics_world);
    UnloadRenderTexture(target);
    CloseWindow();

    return 0;
}

//----------------------------------------------------------------------------------
// Update and Draw Frame
//----------------------------------------------------------------------------------
static void App_UpdateDrawFrame(void) {
    // Update
    float frameDt = GetFrameTime();
    if (frameDt > 0.1f) {
        frameDt = 0.1f;
    }
    accumulator += frameDt;

    // Fixed-timestep updates
    while (accumulator >= TICK_DT) {
        // Step physics simulation
        b3World_Step(physics_world, TICK_DT, 4);

        // TODO: Update game logic here
        accumulator -= TICK_DT;
    }

    // Get cube position from physics
    const b3Transform cube_transform = b3Body_GetTransform(cube_body);
    cube_pos = (Vector3) {cube_transform.p.x, cube_transform.p.y, cube_transform.p.z};

    // Draw
    BeginTextureMode(target);
    ClearBackground(RAYWHITE);

    // Set up 3D camera
    BeginMode3D(camera);
    // TODO: Draw game screen here
    DrawCube(cube_pos, 1, 1, 1, RED);
    DrawCubeWires(cube_pos, 1, 1, 1, BLACK);
    // Draw ground as thin platform
    DrawCube((Vector3) {0, -2.5f, 0}, 20, 0.5f, 20, BLUE);
    DrawCubeWires((Vector3) {0, -2.5f, 0}, 20, 0.5f, 20, BLACK);
    DrawGrid(10, 1);
    EndMode3D();

    DrawRectangleLinesEx((Rectangle) {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, 4, BLACK);

    EndTextureMode();

    // Render to screen
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw render texture to screen
    DrawTexturePro(target.texture, (Rectangle) {0, 0, (float) target.texture.width, -(float) target.texture.height},
                   (Rectangle) {0, 0, (float) target.texture.width, (float) target.texture.height}, (Vector2) {0, 0},
                   0.0f, WHITE);

    // TODO: Draw UI here

    EndDrawing();
}
