#include "NestingRender.h"
#include <string>
#include <iostream>


float NestingRenderer::getScale(
    const Sheet& sheet,
    int w,
    int h
)
{

    float sx =
        (float)w / sheet.width;


    float sy =
        (float)h / sheet.height;


    return std::min(sx, sy);

}



void NestingRenderer::drawSheet(
    SDL_Renderer* renderer,
    const Sheet& sheet,
    int screenX,
    int screenY,
    int screenW,
    int screenH
)
{

    static int debugCount = 0;
    debugCount++;

    if (debugCount % 60 == 0)
    {
        std::cout
            << "\n========== DRAW SHEET DEBUG ==========\n"
            << "Sheet size: "
            << sheet.width
            << " x "
            << sheet.height
            << "\n"
            << "Placed items: "
            << sheet.placed.size()
            << "\n"
            << "Screen position: "
            << screenX
            << ", "
            << screenY
            << "\n"
            << "Screen area: "
            << screenW
            << " x "
            << screenH
            << "\n";
    }


    float scale =
        getScale(
            sheet,
            screenW,
            screenH
        );


    if (debugCount % 60 == 0)
    {
        std::cout
            << "Scale: "
            << scale
            << "\n";
    }


    if (scale <= 0)
    {
        std::cout
            << "[ERROR] INVALID SCALE\n";

        return;
    }



    SDL_Rect sheetRect;


    sheetRect.x = screenX;
    sheetRect.y = screenY;

    sheetRect.w =
        sheet.width * scale;

    sheetRect.h =
        sheet.height * scale;



    if (debugCount % 60 == 0)
    {
        std::cout
            << "Sheet Rect: "
            << sheetRect.x
            << ", "
            << sheetRect.y
            << " "
            << sheetRect.w
            << " x "
            << sheetRect.h
            << "\n";
    }



    // sheet background

    SDL_SetRenderDrawColor(
        renderer,
        220,
        220,
        220,
        255
    );


    SDL_RenderFillRect(
        renderer,
        &sheetRect
    );



    // border

    SDL_SetRenderDrawColor(
        renderer,
        0,
        0,
        0,
        255
    );


    SDL_RenderDrawRect(
        renderer,
        &sheetRect
    );



    // draw placed rectangles


    int itemCount = 0;


    for (auto& r : sheet.placed)
    {


        SDL_Rect item;


        item.x =
            screenX +
            r.x * scale;


        item.y =
            screenY +
            r.y * scale;


        item.w =
            r.width * scale;


        item.h =
            r.height * scale;



        if (debugCount % 60 == 0 && itemCount < 3)
        {
            std::cout
                << "Item "
                << itemCount
                << ": "
                << item.x
                << ","
                << item.y
                << " "
                << item.w
                << "x"
                << item.h
                << "\n";
        }



        SDL_SetRenderDrawColor(
            renderer,
            50,
            120,
            220,
            255
        );


        SDL_RenderFillRect(
            renderer,
            &item
        );


        SDL_SetRenderDrawColor(
            renderer,
            0,
            0,
            0,
            255
        );


        SDL_RenderDrawRect(
            renderer,
            &item
        );


        itemCount++;

    }


    if (debugCount % 60 == 0)
    {
        std::cout
            << "Finished drawing sheet\n"
            << "====================================\n";
    }

}