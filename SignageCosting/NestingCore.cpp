#include "NestingCore.h"
#include <algorithm>
#include <limits>
#include <iostream>

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
    {
        std::cout << "[TRY] FAIL - no free rects\n";
        return false;
    }

    double bestScore = std::numeric_limits<double>::max();
    int bestIndex = -1;
    bool bestRotated = false;
    double bestX = 0;
    double bestY = 0;

    std::cout << "\n[TRY PLACE]\n";
    std::cout << "FreeRects: " << sheet.freeRects.size()
        << " | Item: " << w << "x" << h << "\n";

    for (int i = 0; i < (int)sheet.freeRects.size(); i++)
    {
        const auto& fr = sheet.freeRects[i];

        std::cout << "  FR[" << i << "] "
            << "x=" << fr.x
            << " y=" << fr.y
            << " w=" << fr.width
            << " h=" << fr.height << "\n";

        auto evaluate = [&](double rw, double rh, bool isRot)
            {
                double leftoverW = fr.width - rw;
                double leftoverH = fr.height - rh;

                if (leftoverW < 0 || leftoverH < 0)
                    return;

                double areaFit = (fr.width * fr.height) - (rw * rh);

                double waste = leftoverW + leftoverH;

                // IMPORTANT: penalize rotation slightly (prevents over-rotation stacking)
                double rotationPenalty = isRot ? 5000.0 : 0.0;

                double score =
                    areaFit * 1000.0 +
                    waste * 10.0 +
                    rotationPenalty;

                if (score < bestScore)
                {
                    bestScore = score;
                    bestIndex = i;
                    bestRotated = isRot;
                    bestX = fr.x;
                    bestY = fr.y;
                }
            };

        // NORMAL
        if (w <= fr.width && h <= fr.height)
            evaluate(w, h, false);

        // ROTATED
        if (h <= fr.width && w <= fr.height)
            evaluate(h, w, true);
    }

    if (bestIndex == -1)
    {
        std::cout << "[TRY] FAIL - no fit found\n";
        return false;
    }

    std::cout << "[TRY] SUCCESS -> index=" << bestIndex
        << " rotated=" << bestRotated
        << " x=" << bestX
        << " y=" << bestY
        << " score=" << bestScore << "\n";

    outX = bestX;
    outY = bestY;
    rotated = bestRotated;
    usedIndex = bestIndex;

    return true;
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

    double frRight = fr.x + fr.width;
    double frTop = fr.y + fr.height;

    double rectRight = x + w;
    double rectTop = y + h;

    // =====================================================
    // MAXRECTS RULE: generate ONLY non-overlapping remainders
    // =====================================================

    // RIGHT STRIP (space to the right of placed rect)
    if (rectRight < frRight)
    {
        PlacedRect r;
        r.x = rectRight;
        r.y = fr.y;
        r.width = frRight - rectRight;
        r.height = h;

        if (r.width > 0 && r.height > 0)
            sheet.freeRects.push_back(r);
    }

    // TOP STRIP (space above placed rect)
    if (rectTop < frTop)
    {
        PlacedRect t;
        t.x = fr.x;
        t.y = rectTop;
        t.width = fr.width;
        t.height = frTop - rectTop;

        if (t.width > 0 && t.height > 0)
            sheet.freeRects.push_back(t);
    }

    pruneFreeRects(sheet);
}

void NestingCore::pruneFreeRects(Sheet& sheet)
{
    for (int i = 0; i < (int)sheet.freeRects.size(); i++)
    {
        auto& a = sheet.freeRects[i];

        // remove invalid rectangles
        if (a.width <= 0 || a.height <= 0)
        {
            sheet.freeRects.erase(sheet.freeRects.begin() + i);
            i--;
            continue;
        }

        for (int j = i + 1; j < (int)sheet.freeRects.size(); j++)
        {
            auto& b = sheet.freeRects[j];

            // ==============================
            // CASE 1: B fully inside A
            // ==============================
            if (b.x >= a.x &&
                b.y >= a.y &&
                b.x + b.width <= a.x + a.width &&
                b.y + b.height <= a.y + a.height)
            {
                sheet.freeRects.erase(sheet.freeRects.begin() + j);
                j--;
                continue;
            }

            // ==============================
            // CASE 2: A fully inside B
            // ==============================
            if (a.x >= b.x &&
                a.y >= b.y &&
                a.x + a.width <= b.x + b.width &&
                a.y + a.height <= b.y + b.height)
            {
                sheet.freeRects.erase(sheet.freeRects.begin() + i);
                i--;
                goto next_i;
            }

            // ==============================
            // CASE 3: OVERLAP CLEANUP (NEW)
            // remove redundant overlapped fragments
            // ==============================
            bool overlapX =
                !(a.x + a.width <= b.x || b.x + b.width <= a.x);

            bool overlapY =
                !(a.y + a.height <= b.y || b.y + b.height <= a.y);

            if (overlapX && overlapY)
            {
                double intersectionArea =
                    std::max(0.0, std::min(a.x + a.width, b.x + b.width) - std::max(a.x, b.x)) *
                    std::max(0.0, std::min(a.y + a.height, b.y + b.height) - std::max(a.y, b.y));

                // If overlap is significant → remove smaller contributor
                double areaA = a.width * a.height;
                double areaB = b.width * b.height;

                if (intersectionArea > 0.0)
                {
                    if (areaA < areaB)
                    {
                        sheet.freeRects.erase(sheet.freeRects.begin() + i);
                        i--;
                        goto next_i;
                    }
                    else
                    {
                        sheet.freeRects.erase(sheet.freeRects.begin() + j);
                        j--;
                    }
                }
            }
        }

    next_i:
        continue;
    }
}

void NestingCore::pruneMaxRects(Sheet& sheet)
{
    for (int i = 0; i < (int)sheet.freeRects.size(); i++)
    {
        for (int j = i + 1; j < (int)sheet.freeRects.size(); j++)
        {
            const PlacedRect& a = sheet.freeRects[i];
            const PlacedRect& b = sheet.freeRects[j];

            // -----------------------------
            // A INSIDE B - remove A
            // -----------------------------
            if (a.x >= b.x &&
                a.y >= b.y &&
                a.x + a.width <= b.x + b.width &&
                a.y + a.height <= b.y + b.height)
            {
                sheet.freeRects.erase(sheet.freeRects.begin() + i);
                i--;
                goto next_i;
            }

            // -----------------------------
            // B INSIDE A - remove B
            // -----------------------------
            if (b.x >= a.x &&
                b.y >= a.y &&
                b.x + b.width <= a.x + a.width &&
                b.y + b.height <= a.y + a.height)
            {
                sheet.freeRects.erase(sheet.freeRects.begin() + j);
                j--;
            }
        }

    next_i:
        continue;
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

            // PRE-FIT CHECK
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
                    {
                        double usedW = rotated ? item.height : item.width;
                        double usedH = rotated ? item.width : item.height;

                        splitFreeRect(sheet, index, x, y, usedW, usedH);
                    }

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
                    continue;
                }

                PlacedRect p;
                p.x = x;
                p.y = y;
                p.width = rotated ? item.height : item.width;
                p.height = rotated ? item.width : item.height;
                p.rotated = rotated;

                sheet.placed.push_back(p);

                if (index >= 0 && index < (int)sheet.freeRects.size())
                {
                    double usedW = rotated ? item.height : item.width;
                    double usedH = rotated ? item.width : item.height;

                    splitFreeRect(sheet, index, x, y, usedW, usedH);
                }
            }
        }
    }

    return sheets;
}