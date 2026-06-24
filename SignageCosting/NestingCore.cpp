#include "NestingCore.h"
#include "NestingEngine.h"   // ENSURE TYPES ALWAYS DEFINED
#include <algorithm>

bool NestingCore::canFit(Sheet& sheet, double w, double h)
{
    return (sheet.cursorX + w <= sheet.width) &&
        (sheet.cursorY + h <= sheet.height);
}

void NestingCore::placeRect(Sheet& sheet, double w, double h, bool rotated)
{
    sheet.placed.push_back({ sheet.cursorX, sheet.cursorY, w, h, rotated });

    sheet.cursorX += w;
    sheet.rowHeight = std::max(sheet.rowHeight, h);
}

void NestingCore::newRow(Sheet& sheet)
{
    sheet.cursorX = 0;
    sheet.cursorY += sheet.rowHeight;
    sheet.rowHeight = 0;
}

void NestingCore::newSheet(std::vector<Sheet>& sheets, double w, double h)
{
    Sheet s;
    s.width = w;
    s.height = h;
    sheets.push_back(s);
}

std::vector<Sheet> NestingCore::pack(
    const std::vector<Rect>& items,
    double containerWidth,
    double containerHeight)
{
    std::vector<Sheet> sheets;
    newSheet(sheets, containerWidth, containerHeight);

    Sheet* sheet = &sheets.back();

    for (const auto& item : items)
    {
        for (int i = 0; i < item.quantity; i++)
        {
            double w = item.width;
            double h = item.height;
            bool rotated = false;

            if (!canFit(*sheet, w, h) && canFit(*sheet, h, w))
            {
                std::swap(w, h);
                rotated = true;
            }

            if (!canFit(*sheet, w, h))
            {
                // try new row first
                newRow(*sheet);
            }

            if (!canFit(*sheet, w, h))
            {
                // new sheet only if row fails
                newSheet(sheets, containerWidth, containerHeight);
                sheet = &sheets.back();
                newRow(*sheet); // important reset
            }


            // final safety check
            if (!canFit(*sheet, w, h))
            {
                // try rotation on new sheet
                std::swap(w, h);
                rotated = !rotated;
            }


            placeRect(*sheet, w, h, rotated);
        }
    }

    return sheets;
}