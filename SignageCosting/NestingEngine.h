#pragma once
#include <vector>
#include <algorithm>

struct Rect {
    double width = 0;
    double height = 0;
    int quantity = 0;
};

struct Sheet {
    double width = 0;
    double height = 0;

    double cursorX = 0;
    double cursorY = 0;
    double rowHeight = 0;

    std::vector<Rect> placed;
};

struct RollState {
    double width = 0;
    double cursorX = 0;
    double rowHeight = 0;
    double usedLength = 0;
};

class NestingEngine {
public:
    // SHEETS
    double sheetWidth = 2440;
    double sheetHeight = 1220;

    bool canFit(Sheet& sheet, double w, double h);
    void placeRect(Sheet& sheet, double w, double h);
    void newRow(Sheet& sheet);
    void newSheet(std::vector<Sheet>& sheets);
    int calculateSheets(const std::vector<Rect>& items);

    // VINYL ROLL
    double calculateRollMeters(const std::vector<Rect>& items, double rollWidth);
};