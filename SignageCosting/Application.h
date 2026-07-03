#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <vector>

#include "AppState.h"
#include "Job.h"
#include "CostEngine.h"
#include "NestingRender.h"

class Application
{
public:

    Application();
    ~Application();

    bool initialise();

    void run();

    void shutdown();

    SDL_Renderer* getRenderer() const;

    bool isRunning() const;

private:

    //-----------------------------
    // Main Engine Functions
    //-----------------------------

    void processEvents();

    void update();

    void render();

    //-----------------------------
    // Rendering
    //-----------------------------

    void drawSheets();

    //-----------------------------
    // Job Handling
    //-----------------------------

    void beginNewJob();

    void calculateCurrentJob();

private:

    //-----------------------------
    // SDL
    //-----------------------------

    SDL_Window* window = nullptr;

    SDL_Renderer* renderer = nullptr;

    //-----------------------------
    // SDL Helpers
    //-----------------------------

    bool createWindow();

    bool createRenderer();

    void destroyRenderer();

    void destroyWindow();

    //-----------------------------
    // Application
    //-----------------------------

    bool running = true;

    AppState state = AppState::MainMenu;

    //-----------------------------
    // Modes
    //-----------------------------



    //-----------------------------
    // Costing
    //-----------------------------

    CostEngine engine;

    Job currentJob;

    CostResult currentResult;

    //-----------------------------
    // Renderinga
    //-----------------------------

    NestingRenderer nestingRenderer;

    std::vector<Sheet> sheets;

};