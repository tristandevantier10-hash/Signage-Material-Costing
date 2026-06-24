#include "NestingCore.h"
#include <algorithm>

void NestingCore::newSheet(
    std::vector<Sheet>& sheets,
    double width,
    double height)
{
    Sheet s;
    s.width = width;
    s.height = height;

    // REQUIRED: initialize full free space
    PlacedRect initial;
    initial.x = 0;
    initial.y = 0;
    initial.width = width;
    initial.height = height;
    initial.rotated = false;

    s.freeRects.clear();
    s.freeRects.push_back(initial);

    sheets.push_back(s);
}

bool NestingCore::tryPlaceInSheet(
    Sheet& sheet,
    double w,
    double h,
    bool& rotated,
    double& outX,
    double& outY,
    int& usedIndex)
{
    if (sheet.freeRects.empty())
        return false;

    for (int i = 0; i < (int)sheet.freeRects.size(); i++)
    {
        auto& fr = sheet.freeRects[i];

        // normal
        if (w <= fr.width && h <= fr.height)
        {
            outX = fr.x;
            outY = fr.y;
            rotated = false;
            usedIndex = i;
            return true;
        }

        // rotated
        if (h <= fr.width && w <= fr.height)
        {
            outX = fr.x;
            outY = fr.y;
            rotated = true;
            usedIndex = i;
            return true;
        }
    }

    return false;
}

void NestingCore::splitFreeRect(
    Sheet& sheet,
    int index,
    double x,
    double y,
    double w,
    double h)
{
    if (index < 0 || index >= (int)sheet.freeRects.size())
        return;

    PlacedRect fr = sheet.freeRects[index];
    sheet.freeRects.erase(sheet.freeRects.begin() + index);

    if (fr.x + fr.width > x + w)
    {
        PlacedRect r;
        r.x = x + w;
        r.y = fr.y;
        r.width = (fr.x + fr.width) - (x + w);
        r.height = fr.height;
        r.rotated = false;
        sheet.freeRects.push_back(r);
    }

    if (fr.y + fr.height > y + h)
    {
        PlacedRect b;
        b.x = fr.x;
        b.y = y + h;
        b.width = fr.width;
        b.height = (fr.y + fr.height) - (y + h);
        b.rotated = false;
        sheet.freeRects.push_back(b);
    }

    if (fr.x < x)
    {
        PlacedRect l;
        l.x = fr.x;
        l.y = fr.y;
        l.width = x - fr.x;
        l.height = fr.height;
        l.rotated = false;
        sheet.freeRects.push_back(l);
    }

    if (fr.y < y)
    {
        PlacedRect t;
        t.x = fr.x;
        t.y = fr.y;
        t.width = fr.width;
        t.height = y - fr.y;
        t.rotated = false;
        sheet.freeRects.push_back(t);
    }
}

std::vector<Sheet> NestingCore::pack(
    const std::vector<Rect>& items,
    double containerWidth,
    double containerHeight)
{
    std::vector<Sheet> sheets;

    newSheet(sheets, containerWidth, containerHeight);

    for (const auto& item : items)
    {
        for (int q = 0; q < item.quantity; q++)
        {
            bool placed = false;

            // PRE-FIT CHECK (CRITICAL FIX)
            bool canFit =
                (item.width <= containerWidth && item.height <= containerHeight) ||
                (item.height <= containerWidth && item.width <= containerHeight);

            if (!canFit)
                continue;

            for (auto& sheet : sheets)
            {
                bool rotated = false;
                double x = 0;
                double y = 0;
                int index = -1;

                if (tryPlaceInSheet(sheet, item.width, item.height,
                    rotated, x, y, index))
                {
                    PlacedRect p;
                    p.x = x;
                    p.y = y;
                    p.width = rotated ? item.height : item.width;
                    p.height = rotated ? item.width : item.height;
                    p.rotated = rotated;

                    sheet.placed.push_back(p);

                    if (index >= 0 && index < (int)sheet.freeRects.size())
                        splitFreeRect(sheet, index, x, y, p.width, p.height);

                    placed = true;
                    break;
                }
            }

            // ----------------------------
            // FALLBACK SHEET CREATION
            // ----------------------------
            if (!placed)
            {
                newSheet(sheets, containerWidth, containerHeight);

                auto& sheet = sheets.back();

                bool rotated = false;
                double x = 0;
                double y = 0;
                int index = -1;

                if (!tryPlaceInSheet(sheet, item.width, item.height,
                    rotated, x, y, index))
                {
                    continue; // still cannot place
                }

                PlacedRect p;
                p.x = x;
                p.y = y;
                p.width = rotated ? item.height : item.width;
                p.height = rotated ? item.width : item.height;
                p.rotated = rotated;

                sheet.placed.push_back(p);

                if (index >= 0 && index < (int)sheet.freeRects.size())
                    splitFreeRect(sheet, index, x, y, p.width, p.height);
            }
        }
    }

    return sheets;
}