#include "CostEngine.h"
#include "PricingRules.h"
#include "NestingEngine.h"
#include "PricingDatabase.h"
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cctype>
#include "NestingCalculator.h"

#define TRACE(msg) std::cout << "[TRACE][CostEngine] " << msg << std::endl;

// ---------------- MAIN ENGINE ----------------

CostResult CostEngine::calculate(Job job)
{
    CostResult result;
    NestingEngine nesting;

    const double labourRate = PricingDatabase::getLabourPerM2();
    const double markup = PricingDatabase::getMarkupPercent();

    TRACE("COST ENGINE START");
    TRACE("Job item count = " + std::to_string(job.items.size()));
    TRACE("Labour rate = " + std::to_string(labourRate));
    TRACE("Markup = " + std::to_string(markup));

    for (size_t i = 0; i < job.items.size(); i++)
    {
        auto& item = job.items[i];
        const Material& m = item.material;

        // =====================================================
        // COST TRACE INIT
        // =====================================================
        CostTrace trace;
        trace.materialId = m.id;
        trace.variantLabel = m.variants[item.variantIndex].label;

        trace.width = item.width;
        trace.height = item.height;
        trace.qty = item.quantity;

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

        trace.areaPerItem = areaPerItem;
        trace.totalArea = totalArea;

        const double baseJobCost = 50.0;

        // ---------------- LABOUR v2 ----------------
        const double materialFactor =
            (v.labour_factor > 0.0) ? v.labour_factor : 1.0;

        const double labourCost =
            baseJobCost + (totalArea * labourRate * materialFactor);

        trace.labourFactor = materialFactor;
        trace.labourCost = labourCost;

        double materialCost = 0.0;

        // =====================================================
        // RULE ENGINE (PURE cost_model DISPATCH)
        // =====================================================

        if (rule == "ROLL_AREA")
        {
            if (item.selectedRollWidth <= 0)
            {
                std::cerr << "[ERROR] Invalid roll width\n";
                continue;
            }

            // ---------------- DEBUG INPUT ----------------
            std::cout << "\n";
            std::cout << "ROLL INPUT CHECK\n";
            std::cout << "Selected Roll Width: " << item.selectedRollWidth << " mm\n";
            std::cout << "Selected Roll Width: " << (item.selectedRollWidth / 1000.0) << " m\n";
            std::cout << "Item Size           : " << item.width << " x " << item.height << " mm\n";
            std::cout << "Quantity            : " << item.quantity << "\n";
            std::cout << "---------------------------------\n";

            NestingResult nest =
                NestingCalculator::Calculate(
                    item.width,
                    item.height,
                    item.quantity,
                    item.selectedRollWidth
                );

            // ---------------- DEBUG OUTPUT ----------------
            std::cout << "ROLL NESTING REPORT\n";
            std::cout << "Pieces Across : " << nest.piecesAcross << "\n";
            std::cout << "Rows          : " << nest.rows << "\n";
            std::cout << "Length Used   : " << nest.requiredLengthM << " m\n";
            std::cout << "Area Used     : " << nest.consumedAreaM2 << " m²\n";
            std::cout << "Rotated       : " << (nest.rotated ? "YES" : "NO") << "\n";
            std::cout << "\n";

            const double unitCost =
                PricingDatabase::getMaterialCost(m.id, v.label);

            materialCost = nest.consumedAreaM2 * unitCost;
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

        trace.materialCost = materialCost;

        // ---------------- ITEM TOTAL ----------------
        const double baseCost = materialCost + labourCost;
        const double markupValue = baseCost * (markup / 100.0);
        const double sellPrice = baseCost + markupValue;

        std::cout << "\n";

        std::cout << " ======== ITEM COST BREAKDOWN ======== ""\n\n";

        std::cout << "Material        : " << trace.materialId << "\n";
        std::cout << "Variant         : " << trace.variantLabel << "\n";
        std::cout << "Dimensions      : " << trace.width << " x " << trace.height << " mm\n";
        std::cout << "Quantity        : " << trace.qty << "\n";

        std::cout << "\n";

        std::cout << "Area per item   : " << trace.areaPerItem << " m²\n";
        std::cout << "Total area      : " << trace.totalArea << " m²\n";
        std::cout << "Labour factor   : " << trace.labourFactor << "\n";

        std::cout << "\n";

        std::cout << "Material cost   : R " << trace.materialCost << "\n";
        std::cout << "Labour cost     : R " << trace.labourCost << "\n";

        std::cout << "\n";

        std::cout << "ITEM TOTAL      : R "
            << (trace.materialCost + trace.labourCost)
            << "\n\n";

        std::cout << "======================================\n";

        // ---------------- GLOBAL RESULT ----------------
        result.materialCost += materialCost;
        result.labourCost += labourCost;
        result.sellPrice += sellPrice;

        result.items.push_back({
            m.id,
            m.category,
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