#include <stdint.h>
#include <stdio.h>
#include <string.h>

// TSC clock
// https://stackoverflow.com/questions/13772567/how-to-get-the-cpu-cycle-count-in-x86-64-from-c
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
#define M 3
#define A (N + 2*M)

#define COLOR_BACKGROUND BLACK
#define COLOR_FOREGROUND WHITE

#define SCENE_NEWGAME 0
#define SCENE_DRAWBOARD 1
#define SCENE_GAME 2
#define SCENE_WIN 3

#define MAXSIMSTEPS N

#define WX ((screenWidth - windowedScreenWidth) / 2)
#define WY ((screenHeight - windowedScreenHeight) / 2)


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


bool simulate(uint8_t board[A][A], uint8_t vcount[N], uint8_t steps, uint8_t lastpick[2])
{
    uint8_t simboard[A][A];
    uint8_t simvcount[N];

    if (steps == 0) return false;

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
                if (vcount_in_equilibrium(simvcount, N))
                {
                    return true;
                }
                if ((1 < steps) && simulate(simboard, simvcount, steps-1, ((uint8_t[2]){row, col})))
                {
                    return true;
                }
            }
        }
    }
    return false;
}


int main(void)
    {
    char str[1024];

    uint16_t windowedScreenWidth = A * 64;
    uint16_t windowedScreenHeight = A * 64 + 32;
    uint16_t screenWidth = windowedScreenWidth;
    uint16_t screenHeight = windowedScreenHeight;

    uint8_t scene = SCENE_NEWGAME;

    uint8_t board[A][A];
    uint8_t vcount[N];
    uint8_t lastpick[2] = {255, 255};
    uint32_t hcount;

    uint16_t uintvar[16] = {0};

    Vector2 mouse;
    Vector2 mousedelta;
    int currentGesture = GESTURE_NONE;
    int lastGesture = GESTURE_NONE;


    sprintf(str, "%s\\%s", GetApplicationDirectory(), "tiles.png");
    Image tiles_image = LoadImage(str);

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(windowedScreenWidth, windowedScreenHeight, "Hortirata");
    Vector2 windowPos = GetWindowPosition();

    Texture2D tiles = LoadTextureFromImage(tiles_image); // STRICTLY AFTER InitWindow();!

    UnloadImage(tiles_image);

    SetTargetFPS(TARGET_FPS);

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
                memset(lastpick, 255, 2);
                hcount = 0;
                for (uint8_t v=0; v<N; v++) vcount[v] = 0;
                for (uint8_t row=0; row<A; row++)
                {
                    for (uint8_t col=0; col<A; col++)
                    {
                        if (M <= row && row < A-M && M <= col && col < A-M)
                        {
                            board[row][col] = 0x80 + 0;
                        }
                        else
                        {
                            board[row][col] = 0x80 + 1;
                        }
                    }
                }
                scene = SCENE_DRAWBOARD;
            }

            case SCENE_DRAWBOARD:  // draw
            {
                uint8_t fieldcount = N;
                if (currentGesture == GESTURE_DRAG) mousedelta = GetGestureDragVector();
                else mousedelta = GetMouseDelta();
                bool running_mouse = mousedelta.x != (float)(0) || mousedelta.y != (float)(0);
                if (running_mouse)
                {
                    uint8_t v = __rdtsc() & 0x07; // random value
                    if (v < fieldcount)
                    {
                        board[M + uintvar[0] / N][M + uintvar[0] % N] = v;
                        vcount[v]++;
                        if (uintvar[0] == N*N - 1)
                        {
                            uintvar[0] = 0;
                            scene = SCENE_GAME;
                        }
                        else uintvar[0]++;
                    }
                }
                for (uint8_t row=0; row<A; row++)
                {
                    for (uint8_t col=0; col<A; col++)
                    {
                        uint8_t v = board[row][col];
                        if (0x80 <= v)
                        {
                            DrawTexturePro(tiles, ((Rectangle){(v - 0x80) * 64, 0, 64, 64}), ((Rectangle){WX + col * 64, WY + row * 64, 64, 64}), ((Vector2){0, 0}), 0, WHITE);
                        }
                        else
                        {
                            uint8_t i = (0 < vcount[v]) ? min(13, vcount[v] - 1) : 4;
                            DrawTexturePro(tiles, ((Rectangle){i * 64, (1+v) * 64, 64, 64}), ((Rectangle){WX + col * 64, WY + row * 64, 64, 64}), ((Vector2){0, 0}), 0, WHITE);
                        }
                    }
                }
                DrawText("MOVE YOUR MOUSE", WX + 10, WY + 7 + 64 * A, 20, COLOR_FOREGROUND);
            } break;

            case SCENE_GAME:
            {
                for (uint8_t row=0; row<A; row++)
                {
                    for (uint8_t col=0; col<A; col++)
                    {
                        uint8_t v = board[row][col];
                        uint8_t i = (0 < vcount[v]) ? min(13, vcount[v] - 1) : 4;
                        if (0x80 <= v)
                        {
                            DrawTexturePro(tiles, ((Rectangle){(v - 0x80) * 64, 0, 64, 64}), ((Rectangle){WX + col * 64, WY + row * 64, 64, 64}), ((Vector2){0, 0}), 0, WHITE);
                        }
                        else if (lastpick[0] == row && lastpick[1] == col)
                        {
                            DrawTexturePro(tiles, ((Rectangle){i * 64, (1+v) * 64, 64, 64}), ((Rectangle){WX + col * 64, WY + row * 64, 64, 64}), ((Vector2){0, 0}), 0, GRAY);
                        }
                        else if (v == 0)
                        {
                            DrawTexturePro(tiles, ((Rectangle){i * 64, (1+v) * 64, 64, 64}), ((Rectangle){WX + col * 64, WY + row * 64, 64, 64}), ((Vector2){0, 0}), 0, WHITE);
                        }
                        else
                        {
                            DrawTexturePro(tiles, ((Rectangle){i * 64, (1+v) * 64, 64, 64}), ((Rectangle){WX + col * 64, WY + row * 64, 64, 64}), ((Vector2){0, 0}), 0, WHITE);
                            if (CheckCollisionPointRec(mouse, ((Rectangle){WX + 1 + 64 * col, WY + 1 + 64 * row, 62, 62})))
                            {
                                if (currentGesture != lastGesture && currentGesture == GESTURE_TAP)
                                {
                                    transform(board, vcount, row, col);
                                    lastpick[0] = row;
                                    lastpick[1] = col;
                                    hcount++;
                                }
                            }
                        }

                    }
                }

                uint8_t steps;
                bool equilibrium = vcount_in_equilibrium(vcount, N);
                if (equilibrium) scene = SCENE_WIN;
                else
                {
                    for (steps=1; steps <= MAXSIMSTEPS; steps++)
                    {
                        equilibrium = simulate(board, vcount, steps, lastpick);
                        if (equilibrium)
                        {
                            sprintf(str, "EQUILIBRIUM IN %d HARVEST%s", steps, ((steps==1) ? "" : "S"));
                            DrawText(str, WX + 10, WY + 7 + 64 * A, 20, COLOR_FOREGROUND);
                            break;
                        };
                    }
                    if (!equilibrium)
                    {
                        sprintf(str, "EQUILIBRIUM OVER %d HARVESTS", MAXSIMSTEPS);
                        DrawText(str, WX + 10, WY + 7 + 64 * A, 20, COLOR_FOREGROUND);
                    }
                }

                if (hcount==0)
                {
                    sprintf(str, "NO HARVESTS YET");
                }
                else
                {
                    sprintf(str, "%d HARVEST%s", hcount, ((hcount==1) ? "" : "S"));
                }
                int strwidth = MeasureText(str, 20);
                DrawText(str, WX - 10 + windowedScreenWidth - strwidth , WY + 7 + 64 * A, 20, COLOR_FOREGROUND);

            } break;

            case SCENE_WIN:
            {
                for (uint8_t row=0; row<A; row++)
                {
                    for (uint8_t col=0; col<A; col++)
                    {
                        uint8_t v = board[row][col];
                        if (0x80 <= v)
                        {
                            DrawTexturePro(tiles, ((Rectangle){(v - 0x80) * 64, 0, 64, 64}), ((Rectangle){WX + col * 64, WY + row * 64, 64, 64}), ((Vector2){0, 0}), 0, WHITE);
                        }
                        else
                        {
                            uint8_t i = (0 < vcount[v]) ? min(13, vcount[v] - 1) : 4;
                            DrawTexturePro(tiles, ((Rectangle){i * 64, (1+v) * 64, 64, 64}), ((Rectangle){WX + col * 64, WY + row * 64, 64, 64}), ((Vector2){0, 0}), 0, WHITE);
                        }
                    }
                }
                DrawText("CONGRATULATIONS!", WX + 10, WY + 7 + 64 * A, 20, COLOR_FOREGROUND);
                if (currentGesture != lastGesture && currentGesture == GESTURE_TAP)
                {
                    scene = SCENE_NEWGAME;
                };
            } break;

        }

        //DrawFPS(windowedScreenWidth-100, 10); // for debug

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