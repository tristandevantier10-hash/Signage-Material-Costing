#pragma once

#include <vector>
#include "NestingEngine.h"   // REQUIRED

class NestingCore {
public:
    std::vector<Sheet> pack(
        const std::vector<Rect>& items,
        double containerWidth,
        double containerHeight
    );

private:
    bool canFit(Sheet& sheet, double w, double h);
    void placeRect(Sheet& sheet, double w, double h, bool rotated);
    void newRow(Sheet& sheet);
    void newSheet(std::vector<Sheet>& sheets,
        double width,
        double height);
};
