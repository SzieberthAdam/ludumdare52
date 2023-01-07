// VSCode Folding
// Ctrl+K Ctrl+0
// cursor to: int main(void) block
// Ctrl+K Ctrl+ú
// Ctrl+K Ctrl+4 (or Ctrl+K Ctrl+5 to show regions inside)
// cursor to: switch (header->phse)
// Ctrl+K Ctrl+ő
// Preamble Show/Hide: Ctrl+K Ctrl+9 and Ctrl+K Ctrl+8
// Fold All Regions: Ctrl+k Ctrl+8

#include <stdint.h>
#include <stdio.h>

// TSC clock
// https://stackoverflow.com/questions/13772567/how-to-get-the-cpu-cycle-count-in-x86-64-from-c
#ifdef _MSC_VER
# include <intrin.h>
#else
# include <x86intrin.h>
#endif

#include "raylib.h"

#define TARGET_FPS     1440

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define COLOR_BACKGROUND BLACK
#define COLOR_FOREGROUND WHITE

#define SCENE_NEWGAME 0
#define SCENE_DRAWBOARD 1
#define SCENE_GAME 2


int main(void)
    {
    SetTraceLogLevel(LOG_DEBUG); // TODO: DEBUG
    int debugvar = 0;
    char debugstr[256][256] = {0};

    Vector2 cursor;
    char str[1024];

    uint16_t windowedScreenWidth = 512;
    uint16_t windowedScreenHeight = 512;
    uint16_t screenWidth = windowedScreenWidth;
    uint16_t screenHeight = windowedScreenHeight;

    uint8_t scene = SCENE_NEWGAME;

    uint8_t board[8][8];
    uint8_t vcount[8];

    uint16_t uintvar[16] = {0};

    Vector2 mouse;
    Vector2 mousedelta;
    int currentGesture = GESTURE_NONE;
    int lastGesture = GESTURE_NONE;

    InitWindow(windowedScreenWidth, windowedScreenHeight, "oFLATn");
    Vector2 windowPos = GetWindowPosition();

    SetTargetFPS(TARGET_FPS);

    int display = GetCurrentMonitor(); // see what display we are on right now

    while (!WindowShouldClose())
    {
        // check for alt + enter
        if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
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

        // DEBUG
        // sprintf(str, "lastGesture: %i; currentGesture: %i", lastGesture, currentGesture);
        // TraceLog(LOG_DEBUG, str);

        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawRectangle(0, 0, screenWidth, screenHeight, COLOR_BACKGROUND);

        //DrawFPS(windowedScreenWidth-100, 10); // for debug

        switch (scene)
        {

            case SCENE_NEWGAME:
            {
                for (uint8_t row=0; row<8; row++)
                {
                    for (uint8_t col=0; col<8; col++)
                    {
                        board[row][col] = 128;
                    }
                }
                scene = SCENE_DRAWBOARD;
            }

            case SCENE_DRAWBOARD:  // draw
            {
                if (currentGesture == GESTURE_DRAG) mousedelta = GetGestureDragVector();
                else mousedelta = GetMouseDelta();
                bool running_mouse = mousedelta.x != (float)(0) || mousedelta.y != (float)(0);
                if (running_mouse)
                {
                    uint8_t randval = __rdtsc() & 0x07;
                    board[uintvar[0] / 8][uintvar[0] % 8] = randval;
                    if (uintvar[0] == 8*8)
                    {
                        uintvar[0] = 0;
                        for (uint8_t v=0; v<8; v++) vcount[v] = 0;
                        for (uint8_t row=0; row<8; row++)
                        {
                            for (uint8_t col=0; col<8; col++)
                            {
                                uint8_t v = board[row][col];
                                if (v==128) continue;
                                vcount[v]++;
                            }
                        }
                        scene = SCENE_GAME;
                    }
                    else uintvar[0]++;
                }
                if (running_mouse)
                {
                    for (uint8_t row=0; row<8; row++)
                    {
                        for (uint8_t col=0; col<8; col++)
                        {
                            uint8_t v = board[row][col];
                            if (v==128) continue;
                            sprintf(str, "%d", v);
                            DrawText(str, 22 + 64 * col, 14 + 64 * row, 40, COLOR_FOREGROUND);
                        }
                    }
                }
                if (!running_mouse)
                {
                    DrawText("MOVE\nYOUR\nMOUSE\nFOR\nBOARD\nGENERATION", 22, 14, 40, COLOR_FOREGROUND);
                }
            } break;

            case SCENE_GAME:
            {
                for (uint8_t row=0; row<8; row++)
                {
                    for (uint8_t col=0; col<8; col++)
                    {
                        uint8_t v = board[row][col];
                        if (v==128) continue;
                        sprintf(str, "%d", v);
                        Rectangle r = {1 + 64 * col, 1 + 64 * row, 62, 62};
                        //DrawRectangleRec(r, ((Color){((8 < vcount[v]) ? 31 + 4 * vcount[v] : 0x00), ((vcount[v] == 8) ? 0xFF : 0x00), ((vcount[v] < 8) ? 3 + 36 * vcount[v] : 0x00), 0xFF}));
                        DrawRectangleRec(r, ((Color){
                                ((8 < vcount[v]) ? 255 - 36 * min(7, vcount[v]-8) - 3 : 0x00),
                                ((vcount[v] == 8) ? 0xFF : 0x00),
                                ((vcount[v] < 8) ? 3 + 36 * vcount[v] : 0x00),
                                0xFF
                            }
                        ));
                        DrawText(str, 22 + 64 * col, 14 + 64 * row, 40, COLOR_FOREGROUND);
                        if (CheckCollisionPointRec(mouse, r))
                        {
                            if (currentGesture != lastGesture && currentGesture == GESTURE_TAP)
                            {
                                for (uint8_t row1=((0 < row) ? row-1 : 0); row1<=((row < 7) ? row+1 : 7); row1++)
                                {
                                    for (uint8_t col1=((0 < col) ? col-1 : 0); col1<=((col < 7) ? col+1 : 7); col1++)
                                    {
                                        if ((row1==row) && (col1==col)) continue;
                                        uint8_t v0 = board[row1][col1];
                                        uint8_t v1 = (v0 + v) % 8;
                                        board[row1][col1] = v1;
                                        vcount[v0]--;
                                        vcount[v1]++;
                                    }
                                }
                            }
                        }
                    }
                }
            } break;

        }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}