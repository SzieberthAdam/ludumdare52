#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// TSC clock
// https://stackoverflow.com/questions/13772567/how-to-get-the-cpu-cycle-count-in-x86-tileSize-from-c
#ifdef _MSC_VER
# include <intrin.h>
#else
# include <x86intrin.h>
#endif

#include "raylib.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define FIELDTYPECOUNT 5
#define BOARDROWS 9
#define BOARDCOLUMNS 19

#define COLOR_BACKGROUND BLACK
#define COLOR_FOREGROUND WHITE
#define COLOR_TITLE YELLOW

#define MAXLEVELNAMESIZE 32
#define MAXLEVELCOMAMNDCOUNT 255
#define LEVELCOMAMNDPARAMAREASIZE 65535

typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    uint8_t x;
    uint8_t y;
} Coord;

typedef struct {
    uint16_t id;
    uint8_t op;
    char *attrs;
} LevelCommand;


enum HortirataFieldType {
    LF = 0x0A,
    CR = 0x0D,
    Cursor = 0x11,
    Grass = '0',
    Grain = '1',
    Lettuce = '2',
    Berry = '3',
    Seed = '4',
    FarmStead = 'F',
    Arable = '_',
    Water = '~',
    Sand = ':',
    Oak = 'O',
};

enum HortirataScene {
    NoScene = 0,
    Draw = 1,
    Playing = 11,
    Win = 21,
    Thanks = 22,
};

enum eqpicksSpecialValue {
    eqpicksWin = 0,
    eqpicksMaxCalculate = 3, // IMPORTANT
    eqpicksTooHighToCalculate = 254,
    eqpicksUnchecked = 255,
};

enum LevelCommandOp {
    NOOP = 0x00,
    CLR = 0x01,
    MSG = 0x02,
    SETPLAYINGSCENE = 0x55,
    SETWINSCENE = 0x5F,
    CEND = 0x80,
    CGOTO = 0x81,
    DRAWCURSOR = 0xDC,
    DONE = 0xFD,
    END = 0xFF,
};


/*
=== HELPER FUNCTIONS ===========================================================================================
*/

// Greatest power of 2 less than or equal to x. Hacker's Delight, Figure 3-1.
unsigned flp2(unsigned x)
{
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x - (x >> 1);
}

// Least power of 2 greater than or equal to x. Hacker's Delight, Figure 3-3.
unsigned clp2(unsigned x)
{
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x + 1;
}

// Find first set bit in an integer.
unsigned ffs(int n)
{
    return log2(n & -n) + 1;
}


/*
=== GLOBAL VARIABLES ===========================================================================================
*/

char levelname[MAXLEVELNAMESIZE];
char str[1024];
char levelcommandparamarea[LEVELCOMAMNDPARAMAREASIZE];
int strwidth;
uint32_t picks = 0;
uint8_t board[BOARDROWS][BOARDCOLUMNS];
uint8_t eqpicks = 0;
uint8_t fieldtypecounts[FIELDTYPECOUNT];
uint8_t fieldtypecounttarget = 0;
uint8_t gamefields = 0;
uint8_t level = 0;
uint8_t randomfields = 0;
uint8_t scene = NoScene;
LevelCommand levelcommands[MAXLEVELCOMAMNDCOUNT];
int levelcommandidx = -1;
uint16_t levelcommandid = END;

Coord tileMap[255];
int currentGesture = GESTURE_NONE;
int display = 0;
int fps = 30;
int lastGesture = GESTURE_NONE;
Rectangle gameScreenDest;
Rectangle textboxLevel = {112, 688, 216, 24};
Rectangle textboxPicks = {1144, 688, 104, 24};
Rectangle tileWinDest = {624, 684, 32, 32};
Rectangle tileWinSource = {976, 16, 32, 32};
Texture2D backgroundTexture;
Texture2D tilesTexture;
uint16_t tileOriginX = 32;
uint16_t tileOriginY = 72;
uint16_t windowHeight = 0;
uint16_t windowWidth = 0;
uint32_t gameScreenHeight = 0;
uint32_t gameScreenWidth = 0;
uint32_t screenHeight = 0;
uint32_t screenWidth = 0;
uint8_t gameScreenScale;
uint8_t tileActiveSize = 50;
uint8_t tileDeficitAvailable = 9;
uint8_t tileSize = 64;
uint8_t tileSurplusAvailable = 9;
Vector2 mouse;
Vector2 mouseDelta;
Vector2 windowPos;


/*
=== FUNCTIONS ==================================================================================================
*/


Coord CoordFromText(const char *string)
{
    Coord c = {0, 0};
    char ch = string[0];
    if ('A' <= ch && ch <= 'S') c.y = ch - 'A';
    ch = string[1];
    if ('1' <= ch && ch <= '9') c.x = ch - '1';
    return c;
}

Rectangle GameScreenRectFromBoardCoord(Coord c)
{
    return (Rectangle){tileOriginX + c.y * tileSize, tileOriginY + c.x * tileSize, tileSize, tileSize};
}

void ResetLevelCommands()
{
    while (0 <= levelcommandidx)
    {
        switch (levelcommands[levelcommandidx].op)
        {
            case CEND: break;
            default: MemFree(levelcommands[levelcommandidx].attrs);
        }
        levelcommandidx--;
    }
}

int TextFindNewlineIndex(const char *string, unsigned int startidx)
{
    uint8_t c;
    unsigned int i = startidx;
    do
    {
        c = string[i];
        switch (c)
        {
            case LF:
            case CR:
            {
                return i;
            }
        }
        i++;
    }
    while (0 < c);
    return -1;
}

int TextFindNonNewlineIndex(const char *string, unsigned int startidx)
{
    uint8_t c = 0;
    unsigned int i = startidx;
    while (true)
    {
        c = string[i];
        switch (c)
        {
            case 0: return -1;
            case LF:
            case CR:
            {
                i++;
            } break;
            default: return i;
        }
    }
}


bool load_levelcommand(const char *string, LevelCommand *levelcommand)
{
    char opstr[16];
    uint16_t id = END;
    uint8_t op = 0;
    int i = TextFindIndex(string, " ");
    if (i==-1) return false;
    id = atoi(string);
    i++;
    int j = TextFindIndex(&string[i], " ");
    if (j==-1) strcpy(opstr, &string[i]);
    else memcpy(opstr, &string[i], j);
    opstr[j] = '\0';
    if (strcmp(opstr, "DONE") == 0) op = DONE;
    else if (strcmp(opstr, "MSG") == 0) op = MSG;
    else if (strcmp(opstr, "CLR") == 0) op = CLR;
    else if (strcmp(opstr, "CGOTO") == 0) op = CGOTO;
    else if (strcmp(opstr, "CEND") == 0) op = CEND;
    else if (strcmp(opstr, "DRAWCURSOR") == 0) op = DRAWCURSOR;
    else if (strcmp(opstr, "PLAYING") == 0) op = SETPLAYINGSCENE;
    else if (strcmp(opstr, "WIN") == 0) op = SETWINSCENE;
    else op = NOOP;
    levelcommand->id = id;
    levelcommand->op = op;
    if (j!=-1)
    {
        i += j + 1;
        int length = strlen(&string[i]);
        char *attrs = MemAlloc(length);
        strcpy(attrs, &string[i]);
        levelcommand->attrs = attrs;
    }
    return true;
}


bool load(const char *fileName)
{
    if (!FileExists(fileName)) return false;
    unsigned int filelength = GetFileLength(fileName);
    unsigned char* filedata = LoadFileData(fileName, &filelength);
    int i = min(TextFindNewlineIndex((char *)filedata, 0), MAXLEVELNAMESIZE - 1);
    if (i == -1) return false;
    memcpy(levelname, filedata, i);
    levelname[i] = '\0';
    // i++;
    // if (TextFindNewlineIndex(filedata, i) == i + 1) i++;
    i = TextFindNonNewlineIndex((char *)filedata, i);
    picks = 0;
    eqpicks = eqpicksUnchecked;
    uint8_t row = 0;
    uint8_t col = 0;
    for (uint8_t v=0; v<FIELDTYPECOUNT; v++) fieldtypecounts[v] = 0;
    gamefields = 0;
    randomfields = 0;
    while (i<filelength)
    {
        uint8_t c = filedata[i];
        switch (c)
        {
            case LF:
            case CR:
            {
                if (0<col) row++;
                col = 0;
            } break;
            case Grass:
            case Grain:
            case Lettuce:
            case Berry:
            case Seed:
            {
                board[row][col] = c;
                col++;
                if (BOARDCOLUMNS <= col)
                {
                    row++;
                    col = 0;
                }
                fieldtypecounts[c-Grass]++;
                gamefields++;
            } break;
            case Arable:
            {
                board[row][col] = c;
                col++;
                if (BOARDCOLUMNS <= col)
                {
                    row++;
                    col = 0;
                }
                randomfields++;
            } break;
            default:
            {
                board[row][col] = c;
                col++;
                if (BOARDCOLUMNS <= col)
                {
                    row++;
                    col = 0;
                }
            } break;
        }
        i++;
        if (BOARDROWS <= row) break;
    }
    if (0 == randomfields) scene = Playing;
    else scene = Draw;
    i = TextFindNonNewlineIndex((char *)filedata, i);
    ResetLevelCommands();
    while (i<filelength)
    {
        int j = TextFindNewlineIndex((char *)filedata, i);
        if (j==-1) j = filelength;
        if (i < j)
        {
            memcpy(str, &filedata[i], j-i);
            str[j-i] = '\0';
            bool success = load_levelcommand(str, &levelcommands[levelcommandidx+1]);
            if (success) levelcommandidx++;
            sprintf(str, "id=%d op=%d", levelcommands[levelcommandidx].id, levelcommands[levelcommandidx].op);
        }
        i = j+1;
    }
    if (0 <= levelcommandidx) levelcommandid = levelcommands[0].id;
    else levelcommandid = END;
    fieldtypecounttarget = (gamefields + randomfields) / FIELDTYPECOUNT;
    UnloadFileData(filedata);

    return true;
}





bool load_level(uint8_t levelval)
{
    sprintf(str, "%s\\level%03d.hortirata", GetApplicationDirectory(), levelval);
    bool success = load(str);
    if (success) level = levelval;
    return success;
}


bool save(const char *fileName)
{
    unsigned int bytesToWrite = BOARDROWS * (BOARDCOLUMNS+2);
    char *data = MemAlloc(bytesToWrite);
    uint8_t i = 0;
    for (uint8_t row=0; row<BOARDROWS; ++row)
    {
        for (uint8_t col=0; col<BOARDCOLUMNS; ++col)
        {
            data[i] = board[row][col];
            ++i;
            if (col == BOARDCOLUMNS - 1)
            {
                data[i] = CR;
                ++i;
                data[i] = LF;
                ++i;
            }
        }
    }
    bool success = SaveFileData(fileName, data, bytesToWrite);
    return success;
}


void transform(uint8_t board[BOARDROWS][BOARDCOLUMNS], uint8_t fieldtypecounts[FIELDTYPECOUNT], uint8_t row, uint8_t col)
{
    uint8_t c0 = board[row][col];
    for (uint8_t row1=((0 < row) ? row-1 : 0); row1<=((row < BOARDROWS-1) ? row+1 : BOARDROWS-1); row1++)
    {
        for (uint8_t col1=((0 < col) ? col-1 : 0); col1<=((col < BOARDCOLUMNS-1) ? col+1 : BOARDCOLUMNS-1); col1++)
        {
            if ((row1==row) && (col1==col)) continue;
            uint8_t c1 = board[row1][col1];
            switch (c1)
            {
                case Grass:
                case Grain:
                case Lettuce:
                case Berry:
                case Seed:
                {
                    uint8_t c2 = ((c1-Grass + c0-Grass) % FIELDTYPECOUNT)+Grass;
                    board[row1][col1] = c2;
                    fieldtypecounts[c1-Grass]--;
                    fieldtypecounts[c2-Grass]++;
                } break;
            }
        }
    }
}


bool vcount_in_equilibrium(uint8_t fieldtypecounts[FIELDTYPECOUNT], uint8_t fieldtypecounttarget)
{
    for (uint8_t v=0; v<FIELDTYPECOUNT; v++) if (fieldtypecounts[v] != fieldtypecounttarget) return false;
    return true;
}


bool simulate(uint8_t board[BOARDROWS][BOARDCOLUMNS], uint8_t fieldtypecounts[FIELDTYPECOUNT], uint8_t fieldtypecounttarget, uint8_t picks)
{
    uint8_t simboard[BOARDROWS][BOARDCOLUMNS];
    uint8_t simfieldtypecounts[FIELDTYPECOUNT];
    if (picks == 0) return false;
    for (uint8_t row=0; row<BOARDROWS; row++)
    {
        for (uint8_t col=0; col<BOARDCOLUMNS; col++)
        {
            uint8_t c = board[row][col];
            switch (c)
            {
                case Grain:  // Grass intentionally left out
                case Lettuce:
                case Berry:
                case Seed:
                {
                    memcpy(&simboard, board, BOARDROWS*BOARDCOLUMNS);
                    memcpy(&simfieldtypecounts, fieldtypecounts, FIELDTYPECOUNT);
                    transform(simboard, simfieldtypecounts, row, col);
                    if (vcount_in_equilibrium(simfieldtypecounts, fieldtypecounttarget)) return true;
                    if ((1 < picks) && simulate(simboard, simfieldtypecounts, fieldtypecounttarget, picks-1)) return true;
                } break;
            }
        }
    }
    return false;
}


void draw_cursor(Coord c, int cursornr)
{
    Rectangle source = (Rectangle){(tileMap[Cursor].y - 1 + cursornr) * tileSize, tileMap[Cursor].x * tileSize, tileSize, tileSize};
    Rectangle dest = GameScreenRectFromBoardCoord(c);
    DrawTexturePro(tilesTexture, source, dest, ((Vector2){0, 0}), 0, WHITE);
}


void draw_board()
{
    DrawTexture(backgroundTexture, 0, 0, WHITE);
    for (uint8_t row=0; row<BOARDROWS; row++)
    {
        for (uint8_t col=0; col<BOARDCOLUMNS; col++)
        {
            uint8_t c = board[row][col];
            Rectangle source;
            Rectangle dest = {tileOriginX + col * tileSize, tileOriginY + row * tileSize, tileSize, tileSize};
            switch (c)
            {
                case Grass:
                case Grain:
                case Lettuce:
                case Berry:
                case Seed:
                    {
                        int8_t m = max(min(tileSurplusAvailable, (fieldtypecounts[c-Grass] - fieldtypecounttarget)), -tileDeficitAvailable);
                        source = (Rectangle){(tileMap[c].y + m) * tileSize, tileMap[c].x * tileSize, tileSize, tileSize}; break;
                    } break;
                default:
                    source = (Rectangle){tileMap[c].y * tileSize, tileMap[c].x * tileSize, tileSize, tileSize};
            }
            DrawTexturePro(tilesTexture, source, dest, ((Vector2){0, 0}), 0, WHITE);
        }
    }
}


void draw_info()
{
    sprintf(str, "%d", level);
    strwidth = MeasureText(levelname, 20);
    DrawText(levelname, textboxLevel.x + (textboxLevel.width - strwidth)/2, textboxLevel.y + ((textboxLevel.height - 14)/2) - 2, 20, COLOR_BACKGROUND);
    sprintf(str, "%d", picks);
    strwidth = MeasureText(str, 20);
    DrawText(str, textboxPicks.x + (textboxPicks.width - strwidth)/2, textboxPicks.y + ((textboxPicks.height - 14)/2) - 2, 20, COLOR_BACKGROUND);
    switch (eqpicks)
    {
        case eqpicksWin:
        {
            DrawTexturePro(tilesTexture, tileWinSource, ((Rectangle){tileWinDest.x, tileWinDest.y , tileWinDest.width, tileWinDest.height}), ((Vector2){0, 0}), 0, WHITE);
        }; // fallthrough!
        case 1:
        {
            DrawRectangle(600, 688, 16, 24, COLOR_FOREGROUND);
            DrawRectangle(664, 688, 16, 24, COLOR_FOREGROUND);
        }; // fallthrough!
        case 2:
        {
            DrawRectangle(576, 692, 16, 16, COLOR_FOREGROUND);
            DrawRectangle(688, 692, 16, 16, COLOR_FOREGROUND);
        }; // fallthrough!
        case 3:
        {
            DrawRectangle(552, 696, 16, 8, COLOR_FOREGROUND);
            DrawRectangle(712, 696, 16, 8, COLOR_FOREGROUND);
        } break;
    }
}


void handle_levelcommands()
{
    uint16_t id;
    if (levelcommandidx == -1) return;
    if (levelcommandid == END) return;
    int i = 0;
    for (i = 0; i <= levelcommandidx; ++i)
    {
        if (levelcommands[i].id == levelcommandid) break;
    }
    while (i <= levelcommandidx)
    {
        switch (levelcommands[i].op)
        {
            case DONE: return;
            case CLR:
            {
                Coord c0 = CoordFromText(levelcommands[i].attrs);
                int j = TextFindIndex(levelcommands[i].attrs, " ") + 1;
                Coord c1 = CoordFromText(&levelcommands[i].attrs[j]);
                Rectangle r0 = GameScreenRectFromBoardCoord(c0);
                Rectangle r1 = GameScreenRectFromBoardCoord(c1);
                Rectangle r = (Rectangle){r0.x, r0.y, r1.x-r0.x+r1.width, r1.y-r0.y+r1.height};
                DrawRectangleRec(r, BLACK);
                i++;
            } break;
            case MSG:
            {
                Coord c0 = CoordFromText(levelcommands[i].attrs);
                int j = TextFindIndex(levelcommands[i].attrs, " ") + 1;
                Coord c1 = CoordFromText(&levelcommands[i].attrs[j]);
                Rectangle r0 = GameScreenRectFromBoardCoord(c0);
                Rectangle r1 = GameScreenRectFromBoardCoord(c1);
                Rectangle r = (Rectangle){r0.x, r0.y, r1.x-r0.x+r1.width, r1.y-r0.y+r1.height};
                DrawRectangleRec(r, BLACK);
                j += TextFindIndex(&levelcommands[i].attrs[j], " ") + 1;
                strwidth = MeasureText(&levelcommands[i].attrs[j], 20);
                DrawText(&levelcommands[i].attrs[j], r.x + (r.width - strwidth)/2, r.y + ((r.height - 14)/2) - 2, 20, WHITE);
                i++;
            } break;
            case CEND:
            {
                if (currentGesture != lastGesture && currentGesture == GESTURE_TAP) levelcommandid = END;
                return;
            } break;
            case CGOTO:
            {
                if (currentGesture != lastGesture && currentGesture == GESTURE_TAP)
                {
                    levelcommandid = atoi(levelcommands[i].attrs);
                    return;
                }
                i++;
            } break;
            case DRAWCURSOR:
            {
                Coord c = CoordFromText(levelcommands[i].attrs);
                int j = TextFindIndex(levelcommands[i].attrs, " ") + 1;
                int cursornr = max(1, atoi(&levelcommands[i].attrs[j]));
                draw_cursor(c, cursornr);
                i++;
            } break;
            case SETPLAYINGSCENE:
            {
                scene = Playing;
                i++;
            } break;
            case SETWINSCENE:
            {
                //scene = Win;
                levelcommandid = END;
                bool success = load_level(level+1);
                if (!success) scene = Thanks;
                return;
            } break;
        }
    }
}


int main(void)
    {
    SetTraceLogLevel(LOG_DEBUG);

    tileMap[Arable] = (Coord){0, 0};
    tileMap[Water] = (Coord){0, 1};
    tileMap[FarmStead] = (Coord){0, 2};
    tileMap[Cursor] = (Coord){0, 13};
    tileMap[Grass] = (Coord){1, 9};
    tileMap[Grain] = (Coord){2, 9};
    tileMap[Lettuce] = (Coord){3, 9};
    tileMap[Berry] = (Coord){4, 9};
    tileMap[Seed] = (Coord){5, 9};
    tileMap[Sand] = (Coord){0, 3};
    tileMap[Oak] = (Coord){0, 4};

    load_level(1);

    sprintf(str, "%s\\%s", GetApplicationDirectory(), "tiles.png");
    Image tiles_image = LoadImage(str);
    sprintf(str, "%s\\%s", GetApplicationDirectory(), "bg.png");
    Image bg_image = LoadImage(str);

    gameScreenWidth = bg_image.width;
    gameScreenHeight = bg_image.height;
    screenWidth = gameScreenWidth;
    screenHeight = gameScreenHeight;

    //SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Hortirata");
    SetWindowMinSize(gameScreenWidth, gameScreenHeight);
    windowPos = GetWindowPosition();
    windowWidth = screenWidth;
    windowHeight = screenHeight;

    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    RenderTexture2D screenTarget = LoadRenderTexture(gameScreenWidth, gameScreenHeight);
    SetTextureFilter(screenTarget.texture, TEXTURE_FILTER_POINT);

    SetTargetFPS(fps);

    // call LoadTextureFromImage(); STRICTLY AFTER InitWindow();!
    tilesTexture = LoadTextureFromImage(tiles_image);
    UnloadImage(tiles_image);
    backgroundTexture = LoadTextureFromImage(bg_image);
    UnloadImage(bg_image);

    display = GetCurrentMonitor(); // see what display we are on right now

    while (!WindowShouldClose())
    {
        // check for alt + enter
        if (IsKeyPressed(KEY_F10) || ((IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))))
        {
            // TODO: swithc to fake fullscreen once the screen refresh problem got resolved
            // Note: enabled temporary
            // instead of the true fullscreen which would be the following...
            if (IsWindowFullscreen()) {ToggleFullscreen(); SetWindowSize(windowWidth, windowHeight);}
            else {SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display)); ToggleFullscreen();}

            // TODO: force Windows to refresh whole display (RestoreWindow(); fails here)
            // Note: I go for classic fullscreen until resolved
            //// ... I go for fake fullscreen, similar to SDL_WINDOW_FULLSCREEN_DESKTOP.
            //if (IsWindowState(FLAG_WINDOW_UNDECORATED))
            //{
            //    ClearWindowState(FLAG_WINDOW_UNDECORATED);
            //    screenWidth = windowWidth;
            //    screenHeight = windowHeight;
            //    SetWindowSize(screenWidth, screenHeight);
            //    SetWindowPosition(windowPos.x, windowPos.y);
            //}
            //else
            //{
            //    windowPos = GetWindowPosition();
            //    windowWidth = GetScreenWidth();
            //    windowHeight = GetScreenHeight();
            //    screenWidth = GetMonitorWidth(display);
            //    screenHeight = GetMonitorHeight(display);
            //    SetWindowState(FLAG_WINDOW_UNDECORATED);
            //    SetWindowSize(screenWidth, screenHeight);
            //    SetWindowPosition(0, 0);
            //}
        }

        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        gameScreenScale = min(screenWidth/gameScreenWidth, screenHeight/gameScreenHeight);
        gameScreenDest = (Rectangle){((screenWidth - gameScreenWidth*gameScreenScale) / 2), ((screenHeight - gameScreenHeight*gameScreenScale) / 2), (float)screenTarget.texture.width*gameScreenScale, (float)screenTarget.texture.height*gameScreenScale};

        mouse = GetTouchPosition(0);
        lastGesture = currentGesture;
        currentGesture = GetGestureDetected();

        BeginTextureMode(screenTarget);

        ClearBackground(BLACK);

        switch (scene)
        {
            case Draw:
            {
                bool entropy = false;
                if (IsKeyPressed(KEY_SPACE)) entropy = true;
                if (!entropy)
                {
                    if (currentGesture == GESTURE_DRAG) mouseDelta = GetGestureDragVector();
                    else mouseDelta = GetMouseDelta();
                    entropy = mouseDelta.x != (float)(0) || mouseDelta.y != (float)(0);
                }
                if (0 < randomfields && entropy)
                {
                    uint64_t randomvalue = __rdtsc();
                    uint8_t row, col, c;
                    for (row=0; row<BOARDROWS; ++row)
                    {
                        for (col=0; col<BOARDCOLUMNS; ++col)
                        {
                            c = board[row][col];
                            if (c == Arable) break;
                        }
                        if (c == Arable) break;
                    }
                    uint8_t remgamefields = fieldtypecounttarget * FIELDTYPECOUNT - gamefields;
                    uint8_t remdummyfields = randomfields - remgamefields;
                    if (0 < remgamefields && 0 < remdummyfields)
                    {
                        uint8_t populationsize = randomfields;
                        uint8_t randsize = clp2(populationsize);
                        uint8_t randmask = (randsize * 2 - 1) << 3;
                        uint8_t v1 = (randomvalue & randmask) >> 3;
                        uint8_t v = randomvalue & 0x07;
                        if (v < FIELDTYPECOUNT)
                        {
                            if (v1 < remdummyfields)
                            {
                                board[row][col] = Water;
                                randomfields--;
                            }
                            else if (v < populationsize)
                            {
                                board[row][col] = v+Grass;
                                fieldtypecounts[v]++;
                                gamefields++;
                                randomfields--;
                            }
                        }
                    }
                    if (0 < remgamefields && 0 == remdummyfields)
                    {
                        uint8_t v = randomvalue & 0x07;
                        if (v < FIELDTYPECOUNT)
                        {
                            board[row][col] = v+Grass;
                            fieldtypecounts[v]++;
                            gamefields++;
                            randomfields--;
                        }
                    }
                    else if (0 == remgamefields && 0 < remdummyfields)
                    {
                        board[row][col] = Water;
                        randomfields--;
                    }
                    if (0 == randomfields) scene = Playing;
                }
                draw_board();
                draw_info();
            } break;

            case Playing:
            {
                uint8_t row = (mouse.y - gameScreenDest.y - tileOriginY * gameScreenScale) / (tileSize * gameScreenScale);
                uint8_t col = (mouse.x - gameScreenDest.x - tileOriginX * gameScreenScale) / (tileSize * gameScreenScale);
                uint8_t rowmod = ((uint32_t)(mouse.y - gameScreenDest.y - tileOriginY * gameScreenScale) % (tileSize * gameScreenScale)) / gameScreenScale;
                uint8_t colmod = ((uint32_t)(mouse.x - gameScreenDest.x - tileOriginX * gameScreenScale) % (tileSize * gameScreenScale)) / gameScreenScale;
                uint8_t lbound = (tileSize-tileActiveSize)/2;
                uint8_t ubound = tileActiveSize + lbound - 1;
                uint8_t c = board[row][col];
                bool validloc = \
                (
                        (row < BOARDROWS && col < BOARDCOLUMNS)
                        &&
                        (0 < c-Grass && c-Grass < FIELDTYPECOUNT)
                        &&
                        (lbound <= rowmod && rowmod <= ubound && lbound <= colmod && colmod <= ubound)
                );
                if (validloc && (currentGesture != lastGesture && currentGesture == GESTURE_TAP))
                {
                    transform(board, fieldtypecounts, row, col);
                    picks++;
                    eqpicks = eqpicksUnchecked;
                }
                bool equilibrium = vcount_in_equilibrium(fieldtypecounts, fieldtypecounttarget);
                if (equilibrium)
                {
                    eqpicks = eqpicksWin;
                    scene = Win;
                }
                else if (!equilibrium && (eqpicks == eqpicksUnchecked))
                {
                    for (eqpicks=1; eqpicks <= eqpicksMaxCalculate; eqpicks++)
                    {
                        equilibrium = simulate(board, fieldtypecounts, fieldtypecounttarget, eqpicks);
                        if (equilibrium) break;
                    }
                    if (!equilibrium) eqpicks = eqpicksTooHighToCalculate;
                }
                draw_board();
                draw_info();
                handle_levelcommands();
                if (validloc && (currentGesture == GESTURE_NONE || currentGesture == GESTURE_DRAG)) draw_cursor((Coord){row, col}, 1);
            } break;

            case Win:
            {
                draw_board();
                draw_info();
                handle_levelcommands();
                if (currentGesture != lastGesture && currentGesture == GESTURE_TAP)
                {
                    if (levelcommandid == END)
                    {
                        bool success = load_level(level+1);
                        if (!success) scene = Thanks;
                    }
                }
            } break;

            case Thanks:
            {
                sprintf(str, "THANKS FOR PLAYING!");
                strwidth = MeasureText(str, 20);
                DrawText(str, (gameScreenWidth - strwidth) / 2, 100, 20, COLOR_FOREGROUND);
            }
        }

        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(screenTarget.texture, (Rectangle){0, 0, (float)screenTarget.texture.width, -(float)screenTarget.texture.height}, gameScreenDest, (Vector2){ 0, 0 }, 0.0f, WHITE);
            //DrawFPS(screenWidth-100, 10); // for debug
        EndDrawing();
        //----------------------------------------------------------------------------------

        // Load by drop
        if (IsFileDropped())
        {
            FilePathList droppedfiles = LoadDroppedFiles();
            if (droppedfiles.count == 1)
            {
                bool success = load(droppedfiles.paths[0]);
                if (success) level = 0;
            }
            UnloadDroppedFiles(droppedfiles);
        }
        // Quickload
        if (IsKeyPressed(KEY_L))
        {
            sprintf(str, "%s\\%s", GetApplicationDirectory(), "puzzle.hortirata");
            bool success = load(str);
            if (success) level = 0;
        }
        // Quicksave
        if (IsKeyPressed(KEY_S))
        {
            sprintf(str, "%s\\%s", GetApplicationDirectory(), "puzzle.hortirata");
            save(str);
        }

    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    ResetLevelCommands();
    UnloadRenderTexture(screenTarget);
    UnloadTexture(backgroundTexture);
    UnloadTexture(tilesTexture);
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}