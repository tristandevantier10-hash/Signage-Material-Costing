#pragma once
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "NestingCore.h"


class NestingRenderer
{

public:

    void drawSheet(
        SDL_Renderer* renderer,
        const Sheet& sheet,
        int screenX,
        int screenY,
        int screenW,
        int screenH
    );


private:

    float getScale(
        const Sheet& sheet,
        int w,
        int h
    );

};
