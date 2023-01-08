#include <stdint.h>
#include <stdio.h>
#include <string.h>

// TSC clock
// https://stackoverflow.com/questions/13772567/how-to-get-the-cpu-cycle-count-in-x86-TILESIZE-from-c
#ifdef _MSC_VER
# include <intrin.h>
#else
# include <x86intrin.h>
#endif

#include "raylib.h"

#define TARGET_FPS     30

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define N 5
#define M 2
#define A (N + 2*M)
#define TILESIZE 64
#define TILECENTERSIZE 50
#define TILEUNDERLEVEL 9
#define TILEOVERLEVEL 9
#define TILE_HOVER_COL 17
#define TILE_LOCK_COL 18

#define COLOR_BACKGROUND BLACK
#define COLOR_FOREGROUND WHITE

#define SCENE_NEWGAME 0
#define SCENE_DRAWBOARD 1
#define SCENE_GAME 2
#define SCENE_WIN 3

#define STATE_WIN 0
#define STATE_MANYHARVESTS 127
#define STATE_BOARDUNDERCONSTRUCTION 255

#define MAXSIMHARVESTS N

#define WX ((screenWidth - windowedScreenWidth) / 2)
#define WY ((screenHeight - windowedScreenHeight) / 2)

char str[1024];

typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    uint8_t board[A][A];
    uint8_t lastpick[2];
    uint32_t harvests;
} HortirataData;


void transform(uint8_t board[A][A], uint8_t vcount[N], uint8_t row, uint8_t col)
{
    uint8_t v = board[row][col];
    for (uint8_t row1=((0 < row) ? row-1 : 0); row1<=((row < A-1) ? row+1 : A-1); row1++)
    {
        for (uint8_t col1=((0 < col) ? col-1 : 0); col1<=((col < A-1) ? col+1 : A-1); col1++)
        {
            if ((row1==row) && (col1==col)) continue;
            uint8_t v0 = board[row1][col1];
            if (0x80 <= v0) continue;
            uint8_t v1 = (v0 + v) % N;
            board[row1][col1] = v1;
            vcount[v0]--;
            vcount[v1]++;
        }
    }
}

bool vcount_in_equilibrium(uint8_t vcount[N], uint8_t target)
{
    for (uint8_t v=0; v<N; v++)
    {
        if (vcount[v] != target) return false;
    }
    return true;
}


bool simulate(uint8_t board[A][A], uint8_t vcount[N], uint8_t harvests, uint8_t lastpick[2])
{
    uint8_t simboard[A][A];
    uint8_t simvcount[N];

    if (harvests == 0) return false;

    for (uint8_t row=0; row<A; row++)
    {
        for (uint8_t col=0; col<A; col++)
        {
            uint8_t v = board[row][col];
            if (0 < v && v < N && !(lastpick[0] == row && lastpick[1] == col))
            {
                memcpy(&simboard, board, A*A);
                memcpy(&simvcount, vcount, N);
                transform(simboard, simvcount, row, col);
                if (vcount_in_equilibrium(simvcount, N)) return true;
                if ((1 < harvests) && simulate(simboard, simvcount, harvests-1, ((uint8_t[2]){row, col}))) return true;
            }
        }
    }
    return false;
}


void draw_board(HortirataData data, uint8_t vcount[N], uint8_t targetvcount, uint8_t eqharvests, Rectangle viewport, Texture2D tiles)
{
    for (uint8_t row=0; row<A; row++)
    {
        for (uint8_t col=0; col<A; col++)
        {
            uint8_t v = data.board[row][col];
            uint8_t i = (0 < vcount[v]) ? min(TILEUNDERLEVEL + TILEOVERLEVEL, TILEUNDERLEVEL + vcount[v] - targetvcount) : TILEUNDERLEVEL;
            Rectangle dest = {viewport.x + col * TILESIZE, viewport.y + row * TILESIZE, TILESIZE, TILESIZE};
            if (0x80 <= v)
            {
                DrawTexturePro(tiles, ((Rectangle){(v - 0x80) * TILESIZE, 0, TILESIZE, TILESIZE}), dest, ((Vector2){0, 0}), 0, WHITE);
            }
            else if (data.lastpick[0] == row && data.lastpick[1] == col)
            {
                DrawTexturePro(tiles, ((Rectangle){i * TILESIZE, (1+v) * TILESIZE, TILESIZE, TILESIZE}), dest, ((Vector2){0, 0}), 0, WHITE);
                DrawTexturePro(tiles, ((Rectangle){TILE_LOCK_COL * TILESIZE, 0, TILESIZE, TILESIZE}), dest, ((Vector2){0, 0}), 0, WHITE);
            }
            else if (v == 0)
            {
                DrawTexturePro(tiles, ((Rectangle){i * TILESIZE, (1+v) * TILESIZE, TILESIZE, TILESIZE}), dest, ((Vector2){0, 0}), 0, WHITE);
            }
            else
            {
                DrawTexturePro(tiles, ((Rectangle){i * TILESIZE, (1+v) * TILESIZE, TILESIZE, TILESIZE}), dest, ((Vector2){0, 0}), 0, WHITE);
            }
        }
    }
    switch (eqharvests)
    {
        case STATE_WIN: sprintf(str, "CONGRATULATIONS!"); break;
        case STATE_MANYHARVESTS: sprintf(str, "EQUILIBRIUM OVER %d HARVESTS", MAXSIMHARVESTS); break;
        case STATE_BOARDUNDERCONSTRUCTION: sprintf(str, "MOVE YOUR MOUSE!"); break;
        default: sprintf(str, "EQUILIBRIUM IN %d HARVEST%s", eqharvests, ((eqharvests==1) ? "" : "S"));
    }
    DrawText(str, viewport.x + 10, viewport.y + 7 + TILESIZE * A, 20, COLOR_FOREGROUND);
    if (eqharvests != STATE_BOARDUNDERCONSTRUCTION)
    {
        if (data.harvests==0) sprintf(str, "NO HARVESTS YET");
        else sprintf(str, "%d HARVEST%s", data.harvests, ((data.harvests==1) ? "" : "S"));
        int strwidth = MeasureText(str, 20);
        DrawText(str, viewport.x + viewport.width - 10 - strwidth , viewport.y + 7 + TILESIZE * A, 20, COLOR_FOREGROUND);
    }
}


int main(void)
    {
    SetTraceLogLevel(LOG_DEBUG);

    HortirataData data;
    uint8_t vcount[N];

    uint16_t windowedScreenWidth = A * TILESIZE;
    uint16_t windowedScreenHeight = A * TILESIZE + 32;
    uint16_t screenWidth = windowedScreenWidth;
    uint16_t screenHeight = windowedScreenHeight;

    uint8_t scene = SCENE_NEWGAME;

    Vector2 mouse;
    Vector2 mousedelta;
    int currentGesture = GESTURE_NONE;
    int lastGesture = GESTURE_NONE;

    SetConfigFlags(FLAG_VSYNC_HINT);
    SetTargetFPS(TARGET_FPS);
    sprintf(str, "%s\\%s", GetApplicationDirectory(), "tiles.png");
    Image tiles_image = LoadImage(str);
    InitWindow(windowedScreenWidth, windowedScreenHeight, "Hortirata");
    Vector2 windowPos = GetWindowPosition();
    Texture2D tiles = LoadTextureFromImage(tiles_image); // STRICTLY AFTER InitWindow();!
    UnloadImage(tiles_image);
    int display = GetCurrentMonitor(); // see what display we are on right now

    while (!WindowShouldClose())
    {
        // check for alt + enter
        if (IsKeyPressed(KEY_F10) || ((IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))))
        {
            // instead of the true fullscreen which would be the following...
            // if (IsWindowFullscreen()) {ToggleFullscreen(); SetWindowSize(windowedScreenWidth, windowedScreenHeight);}
            // else {SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display)); ToggleFullscreen();}

            // ... I go for fake fullscreen, similar to SDL_WINDOW_FULLSCREEN_DESKTOP.
            if (IsWindowState(FLAG_WINDOW_UNDECORATED))
            {
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
                screenWidth = windowedScreenWidth;
                screenHeight = windowedScreenHeight;
                SetWindowSize(screenWidth, screenHeight);
                SetWindowPosition(windowPos.x, windowPos.y);
            }
            else
            {
                windowPos = GetWindowPosition();
                windowedScreenWidth = GetScreenWidth();
                windowedScreenHeight = GetScreenHeight();
                screenWidth = GetMonitorWidth(display);
                screenHeight = GetMonitorHeight(display);
                SetWindowState(FLAG_WINDOW_UNDECORATED);
                SetWindowSize(screenWidth, screenHeight);
                SetWindowPosition(0, 0);
            }
        }

        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        mouse = GetTouchPosition(0);
        lastGesture = currentGesture;
        currentGesture = GetGestureDetected();

        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawRectangle(0, 0, screenWidth, screenHeight, COLOR_BACKGROUND);

        switch (scene)
        {

            case SCENE_NEWGAME:
            {
                memset(data.lastpick, 255, 2);
                data.harvests = 0;
                for (uint8_t v=0; v<N; v++) vcount[v] = 0;
                for (uint8_t row=0; row<A; row++)
                {
                    for (uint8_t col=0; col<A; col++)
                    {
                        if (M <= row && row < A-M && M <= col && col < A-M) data.board[row][col] = 0x80 + 0;
                        else data.board[row][col] = 0x80 + 1;
                    }
                }
                scene = SCENE_DRAWBOARD;
            }

            case SCENE_DRAWBOARD:
            {
                uint8_t row, col, v;
                for (row=0; row<A; ++row)
                {
                    for (col=0; col<A; ++col)
                    {
                        v = data.board[row][col];
                        if (v == 0x80) break;
                    }
                    if (v == 0x80) break;
                }
                if (v == 0x80)
                {
                    if (currentGesture == GESTURE_DRAG) mousedelta = GetGestureDragVector();
                    else mousedelta = GetMouseDelta();
                    bool running_mouse = mousedelta.x != (float)(0) || mousedelta.y != (float)(0);
                    if (running_mouse)
                    {
                        uint8_t v = __rdtsc() & 0x07; // random value
                        if (v < N)
                        {
                            data.board[row][col] = v;
                            vcount[v]++;
                        }
                    }
                }
                else scene = SCENE_GAME;
                draw_board(data, vcount, 5, STATE_BOARDUNDERCONSTRUCTION, ((Rectangle){WX,WY,windowedScreenWidth,windowedScreenHeight}), tiles);
            } break;

            case SCENE_GAME:
            {
                uint8_t row = (mouse.y - WY) / TILESIZE;
                uint8_t col = (mouse.x - WX) / TILESIZE;
                uint8_t rowmod = (uint32_t)(mouse.y - WY) % TILESIZE;
                uint8_t colmod = (uint32_t)(mouse.x - WX) % TILESIZE;
                uint8_t lbound = (TILESIZE-TILECENTERSIZE)/2;
                uint8_t ubound = TILECENTERSIZE + lbound - 1;
                uint8_t v = data.board[row][col];
                bool validloc = \
                (
                        (0 < v && v < N)
                        &&
                        (lbound <= rowmod && rowmod <= ubound && lbound <= colmod && colmod <= ubound)
                        &&
                        !(data.lastpick[0] == row && data.lastpick[1] == col)
                );
                if (validloc && (currentGesture != lastGesture && currentGesture == GESTURE_TAP))
                {
                    transform(data.board, vcount, row, col);
                    data.lastpick[0] = row;
                    data.lastpick[1] = col;
                    data.harvests++;
                }
                uint8_t eqharvests = 0;
                bool equilibrium = vcount_in_equilibrium(vcount, N);
                if (equilibrium) scene = SCENE_WIN;
                else
                {
                    for (eqharvests=1; eqharvests <= MAXSIMHARVESTS; eqharvests++)
                    {
                        equilibrium = simulate(data.board, vcount, eqharvests, data.lastpick);
                        if (equilibrium) break;
                    }
                    if (!equilibrium) eqharvests = STATE_MANYHARVESTS;
                }
                Rectangle viewport = {WX,WY,windowedScreenWidth,windowedScreenHeight};
                draw_board(data, vcount, 5, eqharvests, viewport, tiles);
                if (validloc && (currentGesture == GESTURE_NONE || currentGesture == GESTURE_DRAG))
                {
                    Rectangle dest = {viewport.x + col * TILESIZE, viewport.y + row * TILESIZE, TILESIZE, TILESIZE};
                    DrawTexturePro(tiles, ((Rectangle){TILE_HOVER_COL * TILESIZE, 0, TILESIZE, TILESIZE}), dest, ((Vector2){0, 0}), 0, WHITE);
                }
            } break;

            case SCENE_WIN:
            {
                if (currentGesture != lastGesture && currentGesture == GESTURE_TAP) scene = SCENE_NEWGAME;
                draw_board(data, vcount, 5, STATE_WIN, ((Rectangle){WX,WY,windowedScreenWidth,windowedScreenHeight}), tiles);
            } break;

        }

        //DrawFPS(screenWidth-100, 10); // for debug

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    UnloadTexture(tiles);
    //--------------------------------------------------------------------------------------

    return 0;
}