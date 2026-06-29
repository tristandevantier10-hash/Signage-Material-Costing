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
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "NestingRender.h"
#include "NestingCore.h"
#include <iomanip>
#include <atomic>
#include <mutex>

static constexpr bool ENABLE_MAIN_DEBUG = false;

#if ENABLE_MAIN_DEBUG
#define MAIN_DEBUG(msg) std::cout << "[MAIN DEBUG] " << msg << std::endl;
#else
#define MAIN_DEBUG(msg)
#endif

// =====================================================
// GLOBAL STATE (UNCHANGED BEHAVIOUR)
// =====================================================
std::vector<Sheet> allSheets;
int scrollY = 0;
std::mutex sheetMutex;

void runSDLViewer(
    SDL_Window* window,
    SDL_Renderer* renderer,
    NestingRenderer& nestingRenderer)
{
    bool running = true;

    std::cout << "SDL LOOP STARTED\n";

    int frameCount = 0;

    while (running)
    {
        frameCount++;

        // =========================
        // FRAME DEBUG (every ~60 frames)
        // =========================
        if (frameCount % 60 == 0)
        {
            std::cout << "[SDL] Frame: " << frameCount
                << " | scrollY: " << scrollY << std::endl;
        }

        SDL_Event e;

        // =========================
        // EVENT HANDLING
        // =========================
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                std::cout << "[SDL] Quit event received\n";
                running = false;
            }

            if (e.type == SDL_MOUSEWHEEL)
            {
                scrollY -= e.wheel.y * 40;
                std::cout << "[SDL] Mouse wheel -> scrollY: " << scrollY << std::endl;
            }
        }

        const Uint8* state = SDL_GetKeyboardState(NULL);

        if (state[SDL_SCANCODE_UP])
            scrollY -= 10;

        if (state[SDL_SCANCODE_DOWN])
            scrollY += 10;

        // =========================
        // COPY DATA
        // =========================
        std::vector<Sheet> localCopy;

        {
            std::lock_guard<std::mutex> lock(sheetMutex);
            localCopy = allSheets;
        }

        //=========================
        // CLAMP SCROLL
        //=========================
        if (scrollY < 0)
            scrollY = 0;

        int contentHeight = (int)allSheets.size() * 520;
        int viewHeight = 600;

        int maxScroll = contentHeight - viewHeight;

        if (maxScroll < 0)
            maxScroll = 0;

        if (scrollY > maxScroll)
            scrollY = maxScroll;

        // =========================
        // DATA DEBUG
        // =========================
        if (frameCount % 60 == 0)
        {
            std::cout << "[SDL] Sheets in memory: "
                << localCopy.size() << std::endl;
        }

        // =========================
        // RENDER
        // =========================
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        int offsetY = 100 - scrollY;

        int drawCount = 0;

        for (const auto& sheet : localCopy)
        {
            nestingRenderer.drawSheet(
                renderer,
                sheet,
                100,
                offsetY,
                800,
                500
            );

            offsetY += 520;
            drawCount++;
        }

        if (frameCount % 60 == 0)
        {
            std::cout << "[SDL] Sheets drawn this frame: "
                << drawCount << std::endl;
        }

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    std::cout << "SDL LOOP EXITED\n";
}

// =====================================================
// MAIN (UNCHANGED EXCEPT SDL LOOP CALL)
// =====================================================
int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init failed\n";
        return 1;
    }

    SDL_Window* window =
        SDL_CreateWindow(
            "MaxRects Nesting Viewer",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            1200,
            800,
            SDL_WINDOW_SHOWN
        );

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!window || !renderer)
    {
        std::cerr << "SDL init failed\n";
        return 1;
    }

    NestingRenderer nestingRenderer;

    std::thread sdlThread(
        runSDLViewer,
        window,
        renderer,
        std::ref(nestingRenderer)
    );

    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
        ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    SDL_Window* windowRef = window;
    SDL_Renderer* rendererRef = renderer;

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

    enum RunMode { INTERACTIVE, TEST };
    RunMode mode = TEST;

    std::string jsonData =
        HttpClient::get("https://raw.githubusercontent.com/tristandevantier10-hash/Signage-Material-Costing/main/materials.json");

    if (jsonData.empty())
        return 1;

    MaterialDatabase::load(jsonData);

    std::string pricingJson =
        HttpClient::get("https://raw.githubusercontent.com/tristandevantier10-hash/Signage-Material-Costing/main/pricing.json");

    PricingDatabase::load(pricingJson);
    ProductionPricingDatabase::loadDefaults();

    //New Test Job Placement
    std::cout << "CREATING TEST JOB\n";


    Job job;

    job = TestJobFactory::createDefaultTestJob();



    CostEngine engine;


    CostResult result =
        engine.calculate(job);



    {
        std::lock_guard<std::mutex> lock(sheetMutex);


        allSheets.insert(
            allSheets.end(),
            result.nestingSheets.begin(),
            result.nestingSheets.end()
        );

    }

    InvoicePrinter::print(result);

    std::cout << "\n";

    std::cout
        << "TOTAL SHEETS STORED: "
        << allSheets.size()
        << std::endl;
    std::cout << "\n";

    bool running = true;

    while (running)
    {
        std::cout << "NEW JOB...\n\n";

        char choice;
        std::cout << "Next job? (Q - Quit /T - Toggle Mode /N - Next Job): ";
        std::cin >> choice;


        if (choice == 'Q' || choice == 'q')
        {
            running = false;
            break;
        }


        if (choice == 'T' || choice == 't')
        {
            mode = (mode == INTERACTIVE)
                ? TEST
                : INTERACTIVE;

            std::cout << "MODE CHANGED\n";
        }


        if (choice == 'N' || choice == 'n')
        {
            std::cout << "CREATING NEXT JOB...\n";
        }


        Job job;

        if (mode == TEST)
        {
            job = TestJobFactory::createDefaultTestJob();
        }
        else
        {
            bool addMore = true;

            while (addMore)
            {
                std::cout << "ADD NEW JOB ITEM\n";

                auto materialList = MaterialDatabase::getBaseMaterials();

                for (int i = 0; i < (int)materialList.size(); i++)
                    std::cout << i << ": " << materialList[i] << "\n";

                int matIndex;
                std::cin >> matIndex;

                std::string baseId = materialList[matIndex];
                Material selectedMaterial = MaterialDatabase::get(baseId);

                JobItem item;

                std::cout << "Enter width: ";
                std::cin >> item.width;

                std::cout << "Enter height: ";
                std::cin >> item.height;

                std::cout << "Enter quantity: ";
                std::cin >> item.quantity;

                auto variants = selectedMaterial.variants;

                std::cout << "\nAvailable variants:\n";
                for (size_t i = 0; i < variants.size(); i++)
                    std::cout << i << ": " << variants[i].label << "\n";

                int variantIndex;
                std::cin >> variantIndex;

                const auto& selectedVariant = variants[variantIndex];

                int selectedWidth = 0;

                if (selectedMaterial.category.find("Roll") != std::string::npos)
                {
                    if (!selectedVariant.roll_widths.empty())
                    {
                        for (size_t i = 0; i < selectedVariant.roll_widths.size(); i++)
                            std::cout << i << ": " << selectedVariant.roll_widths[i] << "\n";

                        int autoIndex = (int)selectedVariant.roll_widths.size();
                        std::cout << autoIndex << ": AUTO\n";

                        int widthIndex;
                        std::cin >> widthIndex;

                        if (widthIndex == autoIndex)
                            item.autoRoll = true;
                        else
                            selectedWidth = selectedVariant.roll_widths[widthIndex];
                    }
                }
                else if (selectedMaterial.category == "Sheet")
                {
                    if (!selectedVariant.sheet_formats.empty())
                    {
                        for (size_t i = 0; i < selectedVariant.sheet_formats.size(); i++)
                        {
                            std::cout << i << ": "
                                << selectedVariant.sheet_formats[i].width
                                << " x "
                                << selectedVariant.sheet_formats[i].height
                                << "\n";
                        }

                        int sheetIndex;
                        std::cin >> sheetIndex;

                        selectedWidth = selectedVariant.sheet_formats[sheetIndex].width;
                    }
                }

                item.material = selectedMaterial;
                item.variantIndex = variantIndex;

                if (!item.autoRoll)
                    item.selectedRollWidth = selectedWidth;

                job.addItem(item);

                std::cout << "Add another? (Y/N): ";
                char input;
                std::cin >> input;
                addMore = (input == 'Y' || input == 'y');
            }
        }

        CostEngine engine;
        CostResult result = engine.calculate(job);

        // 1. update SDL data
        {
            std::lock_guard<std::mutex> lock(sheetMutex);
            allSheets.insert(
                allSheets.end(),
                result.nestingSheets.begin(),
                result.nestingSheets.end()
            );
        }

        // 2. invoice output (RESTORED)
        InvoicePrinter::print(result);

        std::cout << "\n";

        // 3. console summary
        std::cout << "TOTAL SHEETS: " << allSheets.size() << std::endl;
        std::cout << "\n";
    }

    // wake SDL thread safely
    SDL_Event quitEvent;
    quitEvent.type = SDL_QUIT;
    SDL_PushEvent(&quitEvent);

    if (sdlThread.joinable())
        sdlThread.join();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}