#pragma once
#include "Job.h"
#include <vector>
#include <string>

//DEFINITION//
// ----------------- //
//It defines what a cost result looks like
//It stores :
//what you made
//how big it was
//what it cost to make
//how much labour got involved
//Then it totals everything into a final price

struct CostTrace
{
    std::string materialId;
    std::string variantLabel;

    double width = 0;
    double height = 0;
    int qty = 0;

    double areaPerItem = 0;
    double totalArea = 0;

    double labourFactor = 1.0;
    double labourCost = 0;

    double materialCost = 0;
};

struct ItemCostResult
{
    // ---------------- CORE ----------------
    std::string materialId;
    std::string category;

    double area = 0.0;

    double materialCost = 0.0;
    double labourCost = 0.0;

    double markupValue = 0.0;

    double totalCost = 0.0;      // subtotal (material + labour)
    double sellPrice = 0.0;      // final item price (WITH markup)

    // ---------------- ROLL DEBUG INFO ----------------
    double rollWidth = 0.0;
    double requiredMeters = 0.0;
    double rollRate = 0.0;

    // ---------------- SHEET DEBUG INFO ----------------
    double sheetsUsed = 0.0;
    double sheetRate = 0.0;

    // ---------------- STRUCTURAL DEBUG INFO ----------------
    double totalLength = 0.0;
};

struct CostResult
{
    std::vector<ItemCostResult> items;

    double materialCost = 0.0;
    double labourCost = 0.0;

    double totalCost = 0.0;   // base cost (NO markup)
    double sellPrice = 0.0;   // final invoice total
    double margin = 0.0;
};

class CostEngine
{
public:
    CostResult calculate(Job job);
};

