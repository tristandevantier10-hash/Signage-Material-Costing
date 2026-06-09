#include <iostream>
#include "Job.h"
#include "CostEngine.h"
#include "HttpClient.h"
#include "MaterialDatabase.h"
#include "PricingDatabase.h"
#include <windows.h>
#include "Format.h"
#include "InvoicePrinter.h"

static constexpr bool ENABLE_MAIN_DEBUG = false;

#if ENABLE_MAIN_DEBUG
#define MAIN_DEBUG(msg) std::cout << "[MAIN DEBUG] " << msg << std::endl;
#else
#define MAIN_DEBUG(msg)
#endif

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout <<
        "==================================================\n"
        "            MATERIAL COST ENGINE v2\n"
        "==================================================\n\n";

    // =====================================================
    // MODE SWITCH (CHANGE THIS)
    // =====================================================
    enum RunMode { INTERACTIVE, TEST };
    RunMode mode = TEST;   // <-- SWITCH HERE // TEST , INTERACTIVE

    //
    std::cout << "[DATA] Loading materials...\n";
    //

    // =====================================================
    // LOAD MATERIALS
    // =====================================================
    std::string jsonData =
        HttpClient::get("https://raw.githubusercontent.com/tristandevantier10-hash/Signage-Material-Costing/main/materials.json");

    if (jsonData.empty()) {
        std::cerr << "Failed to fetch material data.\n";
        return 1;
    }

    MAIN_DEBUG("Materials JSON loaded: " + std::to_string(jsonData.size()) + " bytes");

    MaterialDatabase::load(jsonData);
    MAIN_DEBUG("Materials loaded from DB");

    //
    std::cout << "[DATA] Loading pricing...\n";
    //

    // =====================================================
    // LOAD PRICING
    // =====================================================
    std::string pricingJson =
        HttpClient::get("https://raw.githubusercontent.com/tristandevantier10-hash/Signage-Material-Costing/main/pricing.json");

    PricingDatabase::load(pricingJson);
    MAIN_DEBUG("Pricing loaded");

    // =====================================================
    // DEBUG MATERIAL DUMP
    // =====================================================
    std::cout <<
        "==================================================\n"
        "            MATERIAL CATALOGUE v1\n"
        "==================================================\n\n";

    auto bases = MaterialDatabase::getBaseMaterials();

    for (const auto& id : bases)
    {
        Material m = MaterialDatabase::get(id);

        std::cout << "\n[" << m.id << "] - " << m.name << "\n";
        std::cout << "Category: " << m.category << "\n";

        std::cout << "-------------------------------------------------------------------------------\n";
        std::cout << std::left
            << std::setw(4) << "#"
            << std::setw(32) << "Label"
            << std::setw(18) << "Type"
            << std::setw(18) << "Usage"
            << "Cost\n";
        std::cout << "-------------------------------------------------------------------------------\n";

        for (size_t i = 0; i < m.variants.size(); i++)
        {
            const auto& v = m.variants[i];

            double liveCost = PricingDatabase::getMaterialCost(m.id, v.label);

            std::cout << std::left
                << std::setw(4) << i
                << std::setw(32) << v.label
                << std::setw(18) << v.type
                << std::setw(18) << v.usage
                << "R" << liveCost
                << "\n";
        }

        std::cout << "-------------------------------------------------------------------------------\n";
    }

    // =====================================================
    // OUTER LOOP (NEW JOB CYCLE)
    // =====================================================
    while (true)
    {
        std::cout << "\n===========================\n";
        std::cout << "NEW JOB STARTING...\n";
        std::cout << "===========================\n";

        Job job;
        job.clientName = (mode == TEST) ? "TEST JOB" : "Interactive Job";

        // =====================================================
        // TEST MODE (FIXED)
        // =====================================================
        if (mode == TEST)
        {
            std::cout << "[MAIN] RUNNING IN TEST MODE\n";

            // ---------------- VINYL ----------------
            JobItem vinylItem;
            vinylItem.material = MaterialDatabase::get("VINYL");
            vinylItem.width = 300;
            vinylItem.height = 300;
            vinylItem.quantity = 5;

            vinylItem.variantIndex = 4;

            // ADD THIS (critical for roll materials)
            const auto& v = vinylItem.material.variants[vinylItem.variantIndex];

            // pick first available roll width (safe test default)
            vinylItem.selectedRollWidth = v.roll_widths.empty() ? 0 : v.roll_widths[0];

            job.addItem(vinylItem);

            // ---------------- CHROMADEK ----------------
            JobItem chromadekItem;
            chromadekItem.material = MaterialDatabase::get("CHROMADEK"); // correct ID
            chromadekItem.width = 500;
            chromadekItem.height = 500;
            chromadekItem.quantity = 5;

            chromadekItem.variantIndex = 2; // 1.2mm
            job.addItem(chromadekItem);
        }

        // =====================================================
        // INTERACTIVE MODE
        // =====================================================
        else
        {
            bool addMore = true;

            while (addMore)
            {
                std::cout << "\n===========================\n";
                std::cout << "ADD NEW JOB ITEM\n";
                std::cout << "===========================\n";

                auto materialList = MaterialDatabase::getBaseMaterials();

                std::cout << "\nSelect Material:\n";
                for (int i = 0; i < (int)materialList.size(); i++)
                {
                    std::cout << i << ": " << materialList[i] << "\n";
                }

                int matIndex;
                std::cin >> matIndex;

                if (matIndex < 0 || matIndex >= (int)materialList.size())
                {
                    std::cout << "[MAIN ERROR] Invalid material index\n";
                    continue;
                }

                std::string baseId = materialList[matIndex];
                Material selectedMaterial = MaterialDatabase::get(baseId);

                JobItem item;

                std::cout << "\nEnter item width (mm): ";
                std::cin >> item.width;

                std::cout << "Enter item height (mm): ";
                std::cin >> item.height;

                std::cout << "Enter quantity: ";
                std::cin >> item.quantity;

                auto variants = selectedMaterial.variants;

                std::cout << "\nSelect Variant:\n";

                std::cout << "\nSelect Variant:\n";
                std::cout << "-------------------------------------------------------------------------------\n";
                std::cout << std::left
                    << std::setw(4) << "#"
                    << std::setw(32) << "Label"
                    << std::setw(18) << "Type"
                    << "Usage\n";
                std::cout << "-------------------------------------------------------------------------------\n";

                for (int i = 0; i < (int)variants.size(); i++)
                {
                    std::cout << std::left
                        << std::setw(4) << i
                        << std::setw(32) << variants[i].label
                        << std::setw(18) << variants[i].type
                        << variants[i].usage
                        << "\n";
                }

                std::cout << "-------------------------------------------------------------------------------\n";
                std::cout << "Select variant #: ";

                int variantIndex;
                std::cin >> variantIndex;

                if (variantIndex < 0 || variantIndex >= (int)variants.size())
                {
                    std::cout << "[MAIN ERROR] Invalid variant\n";
                    continue;
                }

                const auto& selectedVariant = variants[variantIndex];

                int selectedWidth = 0;

                if (selectedMaterial.category == "Roll")
                {
                    if (!selectedVariant.roll_widths.empty())
                    {
                        for (size_t i = 0; i < selectedVariant.roll_widths.size(); i++)
                            std::cout << i << ": " << selectedVariant.roll_widths[i] << "mm\n";

                        int widthIndex;
                        std::cin >> widthIndex;

                        selectedWidth = selectedVariant.roll_widths[widthIndex];
                    }
                }
                else if (selectedMaterial.category == "Sheet")
                {
                    if (!selectedVariant.sheet_formats.empty())
                    {
                        for (size_t i = 0; i < selectedVariant.sheet_formats.size(); i++)
                            std::cout << i << ": "
                            << selectedVariant.sheet_formats[i].width
                            << " x "
                            << selectedVariant.sheet_formats[i].height
                            << " mm\n";

                        int sheetIndex;
                        std::cin >> sheetIndex;

                        // optional: store only width or both later
                        selectedWidth = selectedVariant.sheet_formats[sheetIndex].width;
                    }
                }

                item.material = selectedMaterial;
                item.variantIndex = variantIndex;
                item.selectedRollWidth = selectedWidth;

                job.addItem(item);

                std::cout << "\nAdd another item? (1 = yes, 0 = no): ";
                std::cin >> addMore;
            }
        }

        // =====================================================
        // COST ENGINE
        // =====================================================
        MAIN_DEBUG("Calling CostEngine");

        CostEngine engine;
        CostResult result = engine.calculate(job);

        MAIN_DEBUG("CostEngine complete");

        // =====================================================
        // INVOICE PRINT
        // =====================================================
        InvoicePrinter::print(result);

        // =====================================================
        // CONTROL FLOW
        // =====================================================
        char choice;
        std::cout << "\nStart new job? (N = next, Q = quit, T = toggle mode): ";
        std::cin >> choice;

        if (choice == 'Q' || choice == 'q')
            break;

        if (choice == 'T' || choice == 't')
        {
            mode = (mode == INTERACTIVE) ? TEST : INTERACTIVE;
            std::cout << "[MAIN] MODE TOGGLED\n";
        }
    }

    return 0;
}