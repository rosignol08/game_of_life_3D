/*******************************************************************************************
*
*
*   3 regles simples :
*   Separation (si un oiseau est trop proche d'un de ses voisin il s'en ecarte),
*   Alignement (ils s'alignent dans la direction des oiseau qui l'entourent),
*   Cohésion (cohésion pour aller vers la position moyenne des oiseau qui l'entourent).
*
*   
*
********************************************************************************************/
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#define RAYGUI_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#define RLIGHTS_IMPLEMENTATION
#if defined(_WIN32) || defined(_WIN64)
#include "include/shaders/rlights.h"
#define PLATFORME_LINUX false
#elif defined(__linux__)
#include "include/shaders/rlights.h"
#include "raygui.h"
#define PLATFORME_LINUX true
#endif

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            120//120//si c'est 100 ça ouvre pas les autres shaders
#endif
#define SHADOWMAP_RESOLUTION 1024
/*******************************************************************************************
*
*   raylib [shaders] example - Shadowmap
*
*   Example complexity rating: [★★★★] 4/4
*
*   Example originally created with raylib 5.0, last time updated with raylib 5.0
*
*   Example contributed by TheManTheMythTheGameDev (@TheManTheMythTheGameDev) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2023-2025 TheManTheMythTheGameDev (@TheManTheMythTheGameDev)
*
********************************************************************************************/


RenderTexture2D LoadShadowmapRenderTexture(int width, int height);
void UnloadShadowmapRenderTexture(RenderTexture2D target);

#include <ctime>

#define GRID_SIZE 30
#define CELL_SIZE 1.0f

struct Cell {
    bool alive;
    bool nextState;
};

void InitializeGrid(Cell grid[GRID_SIZE][GRID_SIZE][GRID_SIZE]) {
    srand(time(0)); // Initialize random seed with current time
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                grid[x][y][z].alive = (rand() % 2 == 0);
            }
        }
    }
}


int CountNeighbors(int x, int y, int z, Cell grid[GRID_SIZE][GRID_SIZE][GRID_SIZE]) {
    int count = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                if (dx == 0 && dy == 0 && dz == 0) continue; // Ne pas compter la cellule elle-même
                
                int nx = x + dx;
                int ny = y + dy;
                int nz = z + dz;
                
                // Vérification des limites de la grille
                if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE && nz >= 0 && nz < GRID_SIZE) {
                    if (grid[nx][ny][nz].alive) count++;
                }
            }
        }
    }
    return count;
}


void Update_grille(Cell grid[GRID_SIZE][GRID_SIZE][GRID_SIZE]) {
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                int neighbors = CountNeighbors(x, y, z, grid);
                grid[x][y][z].nextState = (grid[x][y][z].alive && (neighbors == 2 || neighbors == 3)) || (!grid[x][y][z].alive && neighbors == 3);
            }
        }
    }
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                grid[x][y][z].alive = grid[x][y][z].nextState;
            }
        }
    }
}


void Dessine_grille(Cell grid[GRID_SIZE][GRID_SIZE][GRID_SIZE]) {
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                if (grid[x][y][z].alive) {
                    DrawCube((Vector3){ (x - GRID_SIZE / 2) * CELL_SIZE, (y - GRID_SIZE / 2) * CELL_SIZE, (z - GRID_SIZE / 2) * CELL_SIZE }, CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE);
                    DrawCubeWires((Vector3){ (x - GRID_SIZE / 2) * CELL_SIZE, (y - GRID_SIZE / 2) * CELL_SIZE, (z - GRID_SIZE / 2) * CELL_SIZE }, CELL_SIZE, CELL_SIZE, CELL_SIZE, BLACK);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    // Shadows are a HUGE topic, and this example shows an extremely simple implementation of the shadowmapping algorithm,
    // which is the industry standard for shadows. This algorithm can be extended in a ridiculous number of ways to improve
    // realism and also adapt it for different scenes. This is pretty much the simplest possible implementation.
    InitWindow(screenWidth, screenHeight, "Game of Life 3D");

    Camera3D cam = (Camera3D){ 0 };
    cam.position = (Vector3){ 20.0f, 20.0f, 20.0f };
    cam.target = Vector3Zero();
    cam.projection = CAMERA_PERSPECTIVE;
    cam.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    cam.fovy = 90.0f;

    Shader shadowShader = LoadShader(TextFormat("include/shaders/resources/shaders/glsl%i/shadowmap.vs", GLSL_VERSION),
                                     TextFormat("include/shaders/resources/shaders/glsl%i/shadowmap.fs", GLSL_VERSION));
    shadowShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shadowShader, "viewPos");
    Vector3 lightDir = Vector3Normalize((Vector3){ 0.35f, -1.0f, -0.35f });
    Color lightColor = WHITE;
    Vector4 lightColorNormalized = ColorNormalize(lightColor);
    int lightDirLoc = GetShaderLocation(shadowShader, "lightDir");
    int lightColLoc = GetShaderLocation(shadowShader, "lightColor");
    SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shadowShader, lightColLoc, &lightColorNormalized, SHADER_UNIFORM_VEC4);
    int ambientLoc = GetShaderLocation(shadowShader, "ambient");
    float ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    SetShaderValue(shadowShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);
    int lightVPLoc = GetShaderLocation(shadowShader, "lightVP");
    int shadowMapLoc = GetShaderLocation(shadowShader, "shadowMap");
    int shadowMapResolution = SHADOWMAP_RESOLUTION;
    SetShaderValue(shadowShader, GetShaderLocation(shadowShader, "shadowMapResolution"), &shadowMapResolution, SHADER_UNIFORM_INT);
    Cell grille[GRID_SIZE][GRID_SIZE][GRID_SIZE];
    InitializeGrid(grille);
    printf("fini");
    Model cube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    cube.materials[0].shader = shadowShader;
    RenderTexture2D shadowMap = LoadShadowmapRenderTexture(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
    // For the shadowmapping algorithm, we will be rendering everything from the light's point of view
    Camera3D lightCam = (Camera3D){ 0 };
    lightCam.position = Vector3Scale(lightDir, -15.0f);
    lightCam.target = Vector3Zero();
    // Use an orthographic projection for directional lights
    lightCam.projection = CAMERA_ORTHOGRAPHIC;
    lightCam.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    lightCam.fovy = 20.0f;
    DisableCursor();
    SetTargetFPS(60);
    
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        float dt = GetFrameTime();

        Vector3 cameraPos = cam.position;
        SetShaderValue(shadowShader, shadowShader.locs[SHADER_LOC_VECTOR_VIEW], &cameraPos, SHADER_UNIFORM_VEC3);
        UpdateCamera(&cam, CAMERA_FIRST_PERSON);

        const float cameraSpeed = 0.05f;
        if (IsKeyDown(KEY_LEFT))
        {
            if (lightDir.x < 0.6f)
                lightDir.x += cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_RIGHT))
        {
            if (lightDir.x > -0.6f)
                lightDir.x -= cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_UP))
        {
            if (lightDir.z < 0.6f)
                lightDir.z += cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_DOWN))
        {
            if (lightDir.z > -0.6f)
                lightDir.z -= cameraSpeed * 60.0f * dt;
        }
        lightDir = Vector3Normalize(lightDir);
        lightCam.position = Vector3Scale(lightDir, -15.0f);
        //SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
        static float updateTime = 0.0f;
        static float timeAccumulator = 0.0f;
        const float desiredUpdateTime = 0.5f; // Adjust this value to control the simulation speed

        updateTime = GetFrameTime();
        timeAccumulator += updateTime;

        if (timeAccumulator >= desiredUpdateTime) {
            Update_grille(grille);
            timeAccumulator = 0.0f;
        }
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        // First, render all objects into the shadowmap
        // The idea is, we record all the objects' depths (as rendered from the light source's point of view) in a buffer
        // Anything that is "visible" to the light is in light, anything that isn't is in shadow
        // We can later use the depth buffer when rendering everything from the player's point of view
        // to determine whether a given point is "visible" to the light

        // Record the light matrices for future use!
        //Matrix lightView;
        //Matrix lightProj;
        //BeginTextureMode(shadowMap);
        //ClearBackground(WHITE);
        //BeginMode3D(lightCam);
        //    lightView = rlGetMatrixModelview();
        //    lightProj = rlGetMatrixProjection();
        //    DrawGrid();
        //EndMode3D();
        //EndTextureMode();
        //Matrix lightViewProj = MatrixMultiply(lightView, lightProj);
//
        //
        //SetShaderValueMatrix(shadowShader, lightVPLoc, lightViewProj);
        //
        //
        //rlEnableShader(shadowShader.id);
        //int slot = 10; // Can be anything 0 to 15, but 0 will probably be taken up
        //rlActiveTextureSlot(10);
        //rlEnableTexture(shadowMap.depth.id);
        //rlSetUniform(shadowMapLoc, &slot, SHADER_UNIFORM_INT, 1);
        ClearBackground(SKYBLUE);
        BeginMode3D(cam);
            Dessine_grille(grille);

        EndMode3D();

        DrawText("Game of Life 3D", 100, 10, 20, BLACK);
        DrawText("utiliser z q s d pour se deplacer", screenWidth - MeasureText("utiliser z q s d pour se deplacer", 30) - 10, 30, 30, DARKGRAY);
        DrawFPS(10, 10);
        EndDrawing();

        if (IsKeyPressed(KEY_F))
        {
            TakeScreenshot("shaders_shadowmap.png");
        }
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------

    UnloadShader(shadowShader);
    UnloadModel(cube);
    UnloadShadowmapRenderTexture(shadowMap);

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

RenderTexture2D LoadShadowmapRenderTexture(int width, int height)
{
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer
    target.texture.width = width;
    target.texture.height = height;

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create depth texture
        // We don't need a color texture for the shadowmap
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach depth texture to FBO
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

// Unload shadowmap render texture from GPU memory (VRAM)
void UnloadShadowmapRenderTexture(RenderTexture2D target)
{
    if (target.id > 0)
    {
        // NOTE: Depth texture/renderbuffer is automatically
        // queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}
