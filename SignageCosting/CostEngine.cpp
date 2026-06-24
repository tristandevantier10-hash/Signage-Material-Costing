#include "CostEngine.h"
#include "PricingRules.h"
#include "NestingEngine.h"
#include "PricingDatabase.h"
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cctype>
#include "NestingCalculator.h"
#include "RollOptimizer.h"
#include "ProductionPricingDatabase.h"
#include "NestingCore.h"

// ---------------- MAIN ENGINE ----------------

CostResult CostEngine::calculate(Job job)
{
    CostResult result;
    NestingEngine nesting;
    NestingCore core;   // SAFE ADDITION (no structural change)

    result.materialCost = 0.0;
    result.labourCost = 0.0;
    result.productionCost = 0.0;
    result.sellPrice = 0.0;

    bool useCoreNesting = true;   // toggle stays for safety

    const double labourRate = PricingDatabase::getLabourPerM2();
    const double markup = PricingDatabase::getMarkupPercent();

    for (size_t i = 0; i < job.items.size(); i++)
    {
        auto& item = job.items[i];
        const Material& m = item.material;

        if (m.variants.empty())
        {
            std::cerr << "[CRITICAL] No variants for " << m.id << "\n";
            continue;
        }

        if (item.variantIndex < 0 || item.variantIndex >= (int)m.variants.size())
        {
            std::cerr << "[CRITICAL] Invalid variant index\n";
            continue;
        }

        const MaterialVariant& v = m.variants[item.variantIndex];

        ItemCostResult itemResult{};

        itemResult.width = item.width;
        itemResult.height = item.height;

        CostTrace trace;
        trace.materialId = m.id;
        trace.variantLabel = v.label;
        trace.width = item.width;
        trace.height = item.height;
        trace.qty = item.quantity;

        const double areaPerItem = PricingRules::calculateArea(item.width, item.height);
        const double totalArea = areaPerItem * item.quantity;

        const double baseJobCost = 50.0;

        const double materialFactor =
            (v.labour_factor > 0.0) ? v.labour_factor : 1.0;

        const double labourCost =
            baseJobCost + (totalArea * labourRate * materialFactor);

        double materialCost = 0.0;
        double productionCostCalc = 0.0;

        // ---------------- PRODUCTION ----------------
        if (item.production.print)
            productionCostCalc += totalArea * ProductionPricingDatabase::getPrintRate();

        if (item.production.laminate)
            productionCostCalc += totalArea * ProductionPricingDatabase::getLaminateRate();

        if (item.production.plotterCut)
            productionCostCalc += item.quantity * ProductionPricingDatabase::getPlotterCutRate();

        if (item.production.routerCut)
            productionCostCalc += item.quantity * ProductionPricingDatabase::getRouterCutRate();

        if (item.production.application)
            productionCostCalc += totalArea * ProductionPricingDatabase::getApplicationRate();

        if (item.production.frame)
            productionCostCalc += totalArea * ProductionPricingDatabase::getFrameRate();

        // =====================================================
        // ROLL AREA (UNCHANGED)
        // =====================================================

        std::string rule = m.cost_model;
        double resolvedRoll = item.selectedRollWidth;

        RollSolution best{};

        if (rule == "ROLL_AREA")
        {
            std::vector<double> rollOptions(v.roll_widths.begin(), v.roll_widths.end());

            bool usedOptimizer = item.autoRoll;
            double localResolvedRoll = item.selectedRollWidth;

            if (usedOptimizer)
            {
                best = RollOptimizer::evaluate(
                    item.width,
                    item.height,
                    item.quantity,
                    rollOptions
                );

                localResolvedRoll = best.rollWidth;
            }
            else
            {
                best.rollWidth = localResolvedRoll;

                double rollWidthM = localResolvedRoll / 1000.0;
                double totalAreaLocal = totalArea;

                double requiredMeters = totalAreaLocal / rollWidthM;
                double rollAreaProvided = rollWidthM * requiredMeters * 1.02;

                best.wasteArea = rollAreaProvided - totalAreaLocal;
                best.lengthUsed = requiredMeters;
                best.efficiency = (totalAreaLocal / rollAreaProvided) * 100.0;
                best.rotated = false;
            }

            double rollWidthM = localResolvedRoll / 1000.0;
            double requiredMeters = totalArea / rollWidthM;

            const double unitCost =
                PricingDatabase::getMaterialCost(m.id, v.label);

            materialCost = requiredMeters * unitCost;

            itemResult.rollSolution = best;
            itemResult.rollWidth = localResolvedRoll;
        }

        // =====================================================
        // SHEET AREA (CORE INTEGRATION ONLY HERE)
        // =====================================================
        else if (m.cost_model == "SHEET_AREA")
        {
            Rect r{
                (int)item.width,
                (int)item.height,
                item.quantity
            };

            std::vector<Rect> rects = { r };
            std::vector<Sheet> sheetLayouts;

            // =====================================================
            // CORE NESTING SWITCH
            // =====================================================
            if (useCoreNesting)
            {
                sheetLayouts = core.pack(rects, 2440, 1220);

                // ================= DEBUG VERIFY CORE =================
                int placedCount = 0;
                for (const auto& s : sheetLayouts)
                    placedCount += (int)s.placed.size();

                std::cout << "\n[CORE NESTING ACTIVE]\n";
                std::cout << "Sheets: " << sheetLayouts.size() << "\n";
                std::cout << "Placed Rectangles: " << placedCount << "\n";
                std::cout << "====================================\n";
            }
            else
            {
                sheetLayouts = nesting.calculateSheets(rects);
            }

            const int sheetCount = (int)sheetLayouts.size();

            const double unitCost =
                PricingDatabase::getMaterialCost(m.id, v.label);

            materialCost = sheetCount * unitCost;

            // =====================================================
            // ORIGINAL DEBUG OUTPUT (UNCHANGED STYLE)
            // =====================================================
            std::cout << "\n====================================\n";
            std::cout << "NESTING DEBUG | Material: " << m.id
                << " | Variant: " << v.label << "\n";

            std::cout
                << "Sheet Size        : "
                << 2440 << " x "
                << 1220
                << " mm\n";

            std::cout
                << "Input Object Size : "
                << item.width << " x "
                << item.height
                << " mm\n";

            std::cout
                << "Quantity          : "
                << item.quantity
                << "\n";

            std::cout
                << "Sheets Used       : "
                << sheetCount
                << "\n";

            double sheetArea =
                (2440.0 * 1220.0) / 1000000.0;

            double objectArea =
                (item.width * item.height * item.quantity) / 1000000.0;

            std::cout
                << "Object Area       : "
                << objectArea
                << " m2\n";

            std::cout
                << "Single Sheet Area : "
                << sheetArea
                << " m2\n";

            double usedArea =
                objectArea / (sheetArea * sheetCount) * 100.0;

            std::cout
                << "Material Usage    : "
                << usedArea
                << "%\n";

            std::cout << "====================================\n";

            for (size_t s = 0; s < sheetLayouts.size(); s++)
            {
                const Sheet& sheet = sheetLayouts[s];

                std::cout << "\n--- Sheet " << (s + 1) << " ---\n";
                std::cout << "Placed Rects: " << sheet.placed.size() << "\n";

                for (size_t i = 0; i < sheet.placed.size(); i++)
                {
                    const PlacedRect& pr = sheet.placed[i];

                    std::cout
                        << "  Rect " << (i + 1)
                        << " | x=" << pr.x
                        << " y=" << pr.y
                        << " w=" << pr.width
                        << " h=" << pr.height
                        << " rot=" << (pr.rotated ? "Y" : "N")
                        << "\n";
                }
            }
        }

        // fallback
        else
        {
            const double unitCost =
                PricingDatabase::getMaterialCost(m.id, v.label);

            materialCost = totalArea * unitCost;
        }

        trace.materialCost = materialCost;

        // =====================================================
        // FINAL COSTING (UNCHANGED)
        // =====================================================
        double baseCost = materialCost + labourCost + productionCostCalc;
        double markupValue = baseCost * (markup / 100.0);
        double sellPrice = baseCost + markupValue;

        itemResult.sellPrice = sellPrice;   // ADD THIS
        itemResult.markupValue = markupValue;   // ADD THIS
        result.materialCost += materialCost;
        result.labourCost += labourCost;
        result.sellPrice += sellPrice;
        result.productionCost += productionCostCalc;

        itemResult.materialId = m.id;
        itemResult.category = m.category;
        itemResult.quantity = item.quantity;
        itemResult.area = totalArea;

        itemResult.materialCost = materialCost;
        itemResult.labourCost = labourCost;
        itemResult.production = item.production;
        itemResult.productionCost = productionCostCalc;

        itemResult.rollSolution = best;

        result.items.push_back(itemResult);
    }

    result.totalCost =
        result.materialCost +
        result.labourCost +
        result.productionCost;

    // sellPrice already accumulated per item, so margin is valid
    result.margin = result.sellPrice - result.totalCost;

    return result;
}