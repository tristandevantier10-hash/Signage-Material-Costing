#include <iostream>
#include "Job.h"
#include "CostEngine.h"
#include "HttpClient.h"
#include "MaterialDatabase.h"
#include "PricingDatabase.h"
#include <windows.h>
#include "Format.h"
#include "InvoicePrinter.h"
#include "ProductionPricingDatabase.h"
#include "TestJobFactory.h"
#include <thread>
#include <chrono>
#include <limits>
#include "TypeWriter.h"

static constexpr bool ENABLE_MAIN_DEBUG = false;

#if ENABLE_MAIN_DEBUG
#define MAIN_DEBUG(msg) std::cout << "[MAIN DEBUG] " << msg << std::endl;
#else
#define MAIN_DEBUG(msg)
#endif

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
        ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    std::cout <<
        "==================================================\n"
        "          E & G  S I G N S  CC\n"
        "==================================================\n"
        "\n"
        "           ███████╗  ██████╗\n"
        "           ██╔════╝ ██╔════╝\n"
        "           █████╗   ██║  ███╗\n"
        "           ██╔══╝   ██║   ██║\n"
        "           ███████╗ ╚██████╔╝\n"
        "           ╚══════╝  ╚═════╝\n"
        "\n"
        "        SIGNAGE COSTING SYSTEM\n"
        "            VERSION 3.0\n"
        "\n"
        "==================================================\n"
        << std::endl;

    std::cout << "Press ENTER to continue...\n";
    std::cin.get();

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
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1 second
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
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1 second
    //

    // =====================================================
    // LOAD PRICING
    // =====================================================
    std::string pricingJson =
        HttpClient::get("https://raw.githubusercontent.com/tristandevantier10-hash/Signage-Material-Costing/main/pricing.json");

    PricingDatabase::load(pricingJson);
    ProductionPricingDatabase::loadDefaults();
    MAIN_DEBUG("Pricing loaded");

    // =====================================================
    // DEBUG MATERIAL DUMP
    // =====================================================

    TypeWriter::println("==================================================", 2);
    TypeWriter::println("            MATERIAL CATALOGUE v1", 5);
    TypeWriter::println("==================================================\n", 2);

    auto bases = MaterialDatabase::getBaseMaterials();

    for (const auto& id : bases)
    {
        Material m = MaterialDatabase::get(id);

        TypeWriter::print("\n[", 2);
        TypeWriter::print(m.id);
        TypeWriter::print("] - ");
        TypeWriter::println(m.name, 2);

        TypeWriter::print("Category: ");
        TypeWriter::println(m.category, 2);

        TypeWriter::println("-------------------------------------------------------------------------------", 1);

        std::cout << std::left
            << std::setw(4) << "#"
            << std::setw(32) << "Label"
            << std::setw(18) << "Type"
            << std::setw(18) << "Usage"
            << "Cost\n";

        TypeWriter::println("-------------------------------------------------------------------------------", 1);

        for (size_t i = 0; i < m.variants.size(); i++)
        {
            const auto& v = m.variants[i];

            double liveCost = PricingDatabase::getMaterialCost(m.id, v.label);

            // fast row print (NO typewriter here — important)
            std::cout << std::left
                << std::setw(4) << i
                << std::setw(32) << v.label
                << std::setw(18) << v.type
                << std::setw(18) << v.usage
                << "R" << liveCost
                << "\n";
        }

        TypeWriter::println("-------------------------------------------------------------------------------", 1);
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
            job = TestJobFactory::createDefaultTestJob();
        }

        // =====================================================
        // INTERACTIVE MODE
        // =====================================================
        else
        {

            bool addMore = true;

            while (addMore)
            {
                std::cout << "ADD NEW JOB ITEM\n";
                std::cout << "------------------------\n";

                auto materialList = MaterialDatabase::getBaseMaterials();

                std::cout << "\nSelect Material:\n\n";
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

                std::cout << "\nSelected Material: " << selectedMaterial.name << "\n";

                JobItem item;

                std::cout << "\nEnter item width (mm): ";
                std::cin >> item.width;

                std::cout << "Enter item height (mm): ";
                std::cin >> item.height;

                std::cout << "Enter quantity: ";
                std::cin >> item.quantity;

                auto variants = selectedMaterial.variants;

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

                int variantIndex;
                std::cin >> variantIndex;

                if (variantIndex < 0 || variantIndex >= (int)variants.size())
                {
                    std::cout << "[MAIN ERROR] Invalid variant\n";
                    continue;
                }

                const auto& selectedVariant = variants[variantIndex];

                //  ADD THIS RIGHT HERE
                std::cout << "\nSelected Variant: " << selectedVariant.label << "\n\n";

                int selectedWidth = 0;

                if (selectedMaterial.category == "Roll")
                {
                    if (!selectedVariant.roll_widths.empty())
                    {
                        for (size_t i = 0; i < selectedVariant.roll_widths.size(); i++)
                            std::cout << i << ": " << selectedVariant.roll_widths[i] << "mm\n";

                        int autoIndex = (int)selectedVariant.roll_widths.size();
                        std::cout << autoIndex << ": AUTO OPTIMIZER\n";

                        int widthIndex;
                        std::cin >> widthIndex;

                        // ---------------- SAFETY CHECK ----------------
                        if (widthIndex < 0 || widthIndex > autoIndex)
                        {
                            std::cout << "[MAIN ERROR] Invalid roll selection\n";
                            continue;
                        }

                        // ---------------- AUTO MODE ----------------
                        if (widthIndex == autoIndex)
                        {
                            item.autoRoll = true;
                            item.selectedRollWidth = 0; // ignored
                        }
                        else
                        {
                            item.autoRoll = false;
                            selectedWidth = selectedVariant.roll_widths[widthIndex];
                        }
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

                if (!item.autoRoll)
                {
                    item.selectedRollWidth = selectedWidth;
                }

                // =====================================================
                // PRODUCTION OPTIONS
                // =====================================================

                char answer;

                std::cout << "\n=== Production Options ===\n";

                std::cout << "Print? (y/n): ";
                std::cin >> answer;
                item.production.print = (answer == 'y' || answer == 'Y');

                std::cout << "Laminate? (y/n): ";
                std::cin >> answer;
                item.production.laminate = (answer == 'y' || answer == 'Y');

                std::cout << "Plotter Cut? (y/n): ";
                std::cin >> answer;
                item.production.plotterCut = (answer == 'y' || answer == 'Y');

                std::cout << "Router Cut? (y/n): ";
                std::cin >> answer;
                item.production.routerCut = (answer == 'y' || answer == 'Y');

                std::cout << "Application Required? (y/n): ";
                std::cin >> answer;
                item.production.application = (answer == 'y' || answer == 'Y');

                std::cout << "Frame Required? (y/n): ";
                std::cin >> answer;
                item.production.frame = (answer == 'y' || answer == 'Y');

                // =====================================================
                // DEBUG SUMMARY
                // =====================================================

                std::cout << "\n=== Production Summary ===\n";
                std::cout << "Print: " << (item.production.print ? "Yes" : "No") << "\n";
                std::cout << "Laminate: " << (item.production.laminate ? "Yes" : "No") << "\n";
                std::cout << "Plotter Cut: " << (item.production.plotterCut ? "Yes" : "No") << "\n";
                std::cout << "Router Cut: " << (item.production.routerCut ? "Yes" : "No") << "\n";
                std::cout << "Application: " << (item.production.application ? "Yes" : "No") << "\n";
                std::cout << "Frame: " << (item.production.frame ? "Yes" : "No") << "\n";

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