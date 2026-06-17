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

// ---------------- MAIN ENGINE ----------------

CostResult CostEngine::calculate(Job job)
{
    CostResult result;
    NestingEngine nesting;
   
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

        // ---------------- TRACE ----------------
        CostTrace trace;
        trace.materialId = m.id;
        trace.variantLabel = v.label;
        trace.width = item.width;
        trace.height = item.height;
        trace.qty = item.quantity;

        const double areaPerItem = PricingRules::calculateArea(item.width, item.height);
        const double totalArea = areaPerItem * item.quantity;

        trace.areaPerItem = areaPerItem;
        trace.totalArea = totalArea;

        const double baseJobCost = 50.0;

        const double materialFactor =
            (v.labour_factor > 0.0) ? v.labour_factor : 1.0;

        const double labourCost =
            baseJobCost + (totalArea * labourRate * materialFactor);

        trace.labourFactor = materialFactor;
        trace.labourCost = labourCost;

        double materialCost = 0.0;
        double productionCostCalc = 0.0; // renamed (FIX)

        // =====================================================
        // PRODUCTION COST (SAFE SINGLE SOURCE)
        // =====================================================
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
        // ROLL AREA
        // =====================================================

        std::vector<std::string> rollPrint;

        std::string rule = m.cost_model;
        double resolvedRoll = item.selectedRollWidth;

        RollSolution best{}; // ADD THIS HERE

        if (rule == "ROLL_AREA")
        {
            std::vector<double> rollOptions;
            for (double w : v.roll_widths)
                rollOptions.push_back(w);

            bool usedOptimizer = item.autoRoll;
            double localResolvedRoll = item.selectedRollWidth; // FIXED (no shadow)

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

                double areaPerItemLocal = PricingRules::calculateArea(item.width, item.height);
                double totalAreaLocal = areaPerItemLocal * item.quantity;

                double requiredMeters = totalAreaLocal / rollWidthM;

                // simulate real roll usage (no perfect cancellation)
                double rollAreaProvided = rollWidthM * requiredMeters;

                // add tiny real-world waste factor (cut loss / gaps / handling)
                double wasteFactor = 1.02;

                rollAreaProvided *= wasteFactor;

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

            // =============================
            // STORE ROLL RESULT INTO OUTPUT
            // =============================

            itemResult.width = item.width;
            itemResult.height = item.height;

            itemResult.materialId = m.id;
            itemResult.category = m.category;

            itemResult.quantity = item.quantity;
            itemResult.area = totalArea;

            itemResult.materialCost = materialCost;
            itemResult.labourCost = labourCost;
            itemResult.productionCost = productionCostCalc;

            // STORE ROLL DATA
            itemResult.rollSolution = best;
            itemResult.rollWidth = localResolvedRoll;

        }

        else if (m.cost_model == "SHEET_AREA")
        {
            Rect r{
                (int)item.width,
                (int)item.height,
                item.quantity
            };
            std::vector<Rect> rects = { r };

            const int sheets = nesting.calculateSheets(rects);
            const double unitCost = PricingDatabase::getMaterialCost(m.id, v.label);

            materialCost = sheets * unitCost;
        }
        else
        {
            std::cerr << "[WARN] Unknown cost_model '" << m.cost_model
                << "' for " << m.id << " -> fallback AREA\n";

            const double unitCost =
                PricingDatabase::getMaterialCost(m.id, v.label);

            materialCost = totalArea * unitCost;
        }

        trace.materialCost = materialCost;

        // =====================================================
        // FINAL COSTING (FIXED)
        // =====================================================

        double baseCost = materialCost + labourCost + productionCostCalc;
        double markupValue = baseCost * (markup / 100.0);
        double sellPrice = baseCost + markupValue;

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

        itemResult.markupValue = markupValue;
        itemResult.totalCost = baseCost;
        itemResult.sellPrice = sellPrice;

        itemResult.production = item.production;
        itemResult.productionCost = productionCostCalc;

        // =============================
        // ROLL / SHEET DATA (CORRECT)
        // =============================
        if (m.cost_model == "ROLL_AREA")
        {
            itemResult.rollSolution = best;
        }
        else if (m.cost_model == "SHEET_AREA")
        {
            itemResult.rollSolution.rollWidth = 0;
            itemResult.rollSolution.efficiency = 100.0;
            itemResult.rollSolution.lengthUsed = 0;
            itemResult.rollSolution.wasteArea = 0;
            itemResult.rollSolution.rotated = false;
        }

        itemResult.area = totalArea;
        itemResult.materialCost = materialCost;
        itemResult.productionCost = productionCostCalc;

        result.items.push_back(itemResult);

    }

    // FINAL TOTALS (FIXED)
    result.totalCost = result.materialCost + result.labourCost + result.productionCost;
    result.margin = result.sellPrice - result.totalCost;

        return result;
}