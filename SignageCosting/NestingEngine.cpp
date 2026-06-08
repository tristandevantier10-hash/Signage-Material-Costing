#include "NestingEngine.h"
#include <algorithm>

bool NestingEngine::canFit(Sheet& sheet, double w, double h) {
    return (sheet.cursorX + w <= sheet.width) &&
        (sheet.cursorY + h <= sheet.height);
}

void NestingEngine::placeRect(Sheet& sheet, double w, double h) {
    sheet.placed.push_back({ w, h, 1 });

    sheet.cursorX += w;
    sheet.rowHeight = std::max(sheet.rowHeight, h);
}

void NestingEngine::newRow(Sheet& sheet) {
    sheet.cursorX = 0;
    sheet.cursorY += sheet.rowHeight;
    sheet.rowHeight = 0;
}

void NestingEngine::newSheet(std::vector<Sheet>& sheets) {
    Sheet s;
    s.width = sheetWidth;
    s.height = sheetHeight;
    sheets.push_back(s);
}

int NestingEngine::calculateSheets(const std::vector<Rect>& items) {

    std::vector<Sheet> sheets;
    newSheet(sheets);

    Sheet* sheet = &sheets.back();

    for (const auto& item : items) {

        for (int i = 0; i < item.quantity; i++) {

            double w = item.width;
            double h = item.height;

            if (!canFit(*sheet, w, h) && canFit(*sheet, h, w)) {
                std::swap(w, h);
            }

            if (!canFit(*sheet, w, h)) {
                newRow(*sheet);
            }

            if (!canFit(*sheet, w, h)) {
                newSheet(sheets);
                sheet = &sheets.back();
            }

            placeRect(*sheet, w, h);
        }
    }

    return (int)sheets.size();
}