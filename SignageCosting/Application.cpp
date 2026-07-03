#include "Application.h"

#include <iostream>
#include "InvoicePrinter.h"

Application::Application()
{
}

Application::~Application()
{
    shutdown();
}

bool Application::initialise()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL_Init Failed\n";
        return false;
    }

    if (!createWindow())
        return false;

    if (!createRenderer())
        return false;

    state = AppState::MainMenu;

    running = true;

    return true;
}

bool Application::createWindow()
{
    window = SDL_CreateWindow(
        "Signage Costing System",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1200,
        800,
        SDL_WINDOW_SHOWN
    );

    if (!window)
    {
        std::cout << "Failed creating window\n";
        return false;
    }

    return true;
}

bool Application::createRenderer()
{
    renderer =
        SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
        );

    if (!renderer)
    {
        renderer =
            SDL_CreateRenderer(
                window,
                -1,
                SDL_RENDERER_SOFTWARE
            );
    }

    if (!renderer)
    {
        std::cout << "Failed creating renderer\n";
        return false;
    }

    return true;
}

void Application::run()
{
    while (running)
    {
        processEvents();

        update();

        render();

        SDL_Delay(16);
    }
}

void Application::processEvents()
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            running = false;
        }
    }
}

void Application::update()
{
    switch (state)
    {
    case AppState::Splash:
        break;

    case AppState::MainMenu:
        break;

    case AppState::InteractiveJob:
        break;

    case AppState::TestJob:
        break;

    case AppState::Calculating:

        calculateCurrentJob();

        state = AppState::Results;

        break;

    case AppState::Results:
        break;

    case AppState::Exit:

        running = false;

        break;
    }
}

void Application::render()
{
    SDL_SetRenderDrawColor(
        renderer,
        30,
        30,
        30,
        255
    );

    SDL_RenderClear(renderer);

    switch (state)
    {
    case AppState::Results:

        drawSheets();

        break;

    default:

        break;
    }

    SDL_RenderPresent(renderer);
}

void Application::drawSheets()
{
    int y = 100;

    for (const auto& sheet : sheets)
    {
        nestingRenderer.drawSheet(
            renderer,
            sheet,
            100,
            y,
            800,
            500
        );

        y += 520;
    }
}

void Application::calculateCurrentJob(

void Application::beginNewJob()
{
    currentJob.items.clear();

    sheets.clear();
}

void Application::destroyRenderer()
{
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
}

void Application::destroyWindow()
{
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void Application::shutdown()
{
    destroyRenderer();

    destroyWindow();

    SDL_Quit();
}

SDL_Renderer* Application::getRenderer() const
{
    return renderer;
}

bool Application::isRunning() const
{
    return running;
}