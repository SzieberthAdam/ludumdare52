// VSCode Folding
// Ctrl+K Ctrl+0
// cursor to: int main(void) block
// Ctrl+K Ctrl+ú
// Ctrl+K Ctrl+4 (or Ctrl+K Ctrl+5 to show regions inside)
// cursor to: switch (header->phse)
// Ctrl+K Ctrl+ő
// Preamble Show/Hide: Ctrl+K Ctrl+9 and Ctrl+K Ctrl+8
// Fold All Regions: Ctrl+k Ctrl+8

#pragma region preamb

#include <stdint.h>

// TSC clock
// https://stackoverflow.com/questions/13772567/how-to-get-the-cpu-cycle-count-in-x86-64-from-c
#ifdef _MSC_VER
# include <intrin.h>
#else
# include <x86intrin.h>
#endif

#include "raylib.h"

#define TARGET_FPS     60

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define COLOR_BACKGROUND BLACK
#define COLOR_FOREGROUND WHITE

#pragma endregion preamb

void prep_board(uint8_t board[8][8])
{
    for (uint8_t row=0; row<8; row++)
    {
        for (uint8_t col=0; col<8; col++)
        {
            board[row][col] = 128;
        }
    }
}

void draw_board(uint8_t board[8][8])
{
    char s[16];
    for (uint8_t row=0; row<8; row++)
    {
        for (uint8_t col=0; col<8; col++)
        {
            uint8_t v = board[row][col];
            if (v==128) continue;
            sprintf(s, "%d", v);
            DrawText(s, 12 + 64 * col, 12 + 64 * row, 40, COLOR_FOREGROUND);
        }
    }
}

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

    Font font1, font2, font3;

    uint8_t scene = 0;
    uint8_t board[8][8];
    prep_board(board);
    uint16_t uintvar[16] = {0};

    Vector2 mouse;
    Vector2 mousedelta;
    int currentGesture = GESTURE_NONE;
    int lastGesture = GESTURE_NONE;

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
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

        DrawFPS(windowedScreenWidth-100, 10); // for debug

        switch (scene)
        {
            case 0:  // draw
            {
                if (currentGesture == GESTURE_DRAG) mousedelta = GetGestureDragVector();
                else mousedelta = GetMouseDelta();
                if (mousedelta.x != (float)(0) || mousedelta.y != (float)(0))
                {
                    uint8_t randval = __rdtsc() & 0x07;
                    board[uintvar[0] / 8][uintvar[0] % 8] = randval;
                    if (uintvar[0] == 8*8)
                    {
                        uintvar[0] = 0;
                        uintvar[1] = 0;
                        uintvar[2] = 0;
                        scene = 1;
                    }
                    else uintvar[0]++;
                }
                draw_board(board);
            } break;

            case 1:
            {
                draw_board(board);
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