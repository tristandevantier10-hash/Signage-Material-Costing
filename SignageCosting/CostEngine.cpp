#include "CostEngine.h"
#include "PricingRules.h"
#include "NestingEngine.h"
#include "PricingDatabase.h"

#include <iostream>
#include <algorithm>
#include <cctype>

#define TRACE(msg) std::cout << "[TRACE][CostEngine] " << msg << std::endl;

// ---------------- MAIN ENGINE ----------------

CostResult CostEngine::calculate(Job job)
{
    CostResult result;
    NestingEngine nesting;

    const double labourRate = PricingDatabase::getLabourPerM2();
    const double markup = PricingDatabase::getMarkupPercent();

    TRACE("========== COST ENGINE START ==========");
    TRACE("Job item count = " + std::to_string(job.items.size()));
    TRACE("Labour rate = " + std::to_string(labourRate));
    TRACE("Markup = " + std::to_string(markup));

    for (size_t i = 0; i < job.items.size(); i++)
    {
        auto& item = job.items[i];
        const Material& m = item.material;

        // =====================================================
        // ONLY RULE SOURCE (NO CATEGORY ANYMORE)
        // =====================================================
        std::string rule = m.cost_model;

        if (rule.empty())
        {
            std::cerr << "[WARN] Missing cost_model for " << m.id
                << " -> defaulting to ROLL_AREA\n";
            rule = "ROLL_AREA";
        }

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

        const double areaPerItem = PricingRules::calculateArea(item.width, item.height);
        const double totalArea = areaPerItem * item.quantity;

        const double labourCost = totalArea * labourRate;
        double materialCost = 0.0;

        // =====================================================
        // RULE ENGINE (PURE cost_model DISPATCH)
        // =====================================================

        if (rule == "ROLL_AREA")
        {
            const double rollWidthM = item.selectedRollWidth / 1000.0;

            if (rollWidthM <= 0)
            {
                std::cerr << "[ERROR] Invalid roll width\n";
                continue;
            }

            const double requiredMeters = totalArea / rollWidthM;
            const double unitCost = PricingDatabase::getMaterialCost(m.id, v.label);

            materialCost = requiredMeters * unitCost;
        }
        else if (rule == "SHEET_AREA")
        {
            Rect r{ item.width, item.height, item.quantity };
            std::vector<Rect> rects = { r };

            const int sheets = nesting.calculateSheets(rects);
            const double unitCost = PricingDatabase::getMaterialCost(m.id, v.label);

            materialCost = sheets * unitCost;
        }
        else
        {
            std::cerr << "[WARN] Unknown cost_model '" << rule
                << "' for " << m.id << " -> fallback AREA\n";

            const double unitCost = PricingDatabase::getMaterialCost(m.id, v.label);
            materialCost = totalArea * unitCost;
        }

        // ---------------- ITEM TOTAL ----------------
        const double baseCost = materialCost + labourCost;
        const double markupValue = baseCost * (markup / 100.0);
        const double sellPrice = baseCost + markupValue;

        result.materialCost += materialCost;
        result.labourCost += labourCost;
        result.sellPrice += sellPrice;

        result.items.push_back({
            m.id,
            m.category, // still stored, but NOT used anymore
            totalArea,
            materialCost,
            labourCost,
            baseCost,
            markupValue,
            sellPrice
            });
    }

    result.totalCost = result.materialCost + result.labourCost;
    result.margin = result.sellPrice - result.totalCost;

    return result;
}