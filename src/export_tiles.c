#include "raylib.h"

int main(void)
{
    Image tiles_image = LoadImage("tiles.png");
    ExportImageAsCode(tiles_image,"tiles.h");
    return 0;
}