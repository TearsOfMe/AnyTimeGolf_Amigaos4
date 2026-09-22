#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "RBGame.h"
#include "RudeDebug.h"
#include "RudeFont.h"
#include "RudeGL.h"
#include "RudeText.h"
#include "RudeUnitTest.h"
#include "RudeTweaker.h"

#ifdef __amigaos4__
static const char* __attribute__((used)) g_stack_cookie = "$STACK:2097152\n";
#endif

namespace {
constexpr int kLogicalWidth = 768;
constexpr int kLogicalHeight = 1024;

void setGLAttributes(bool enableMSAA)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    if (enableMSAA) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    } else {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    }

    const char *forceSoftware = std::getenv("LIBGL_ALWAYS_SOFTWARE");
    if (forceSoftware && (std::strcmp(forceSoftware, "1") == 0 || std::strcmp(forceSoftware, "true") == 0)) {
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 0);
    }
}

RudeScreenVertex mapCoord(int sx, int sy, int winW, int winH)
{
    if (winW <= 0) winW = 1;
    if (winH <= 0) winH = 1;
    int lx = (sx * kLogicalWidth) / winW;
    int ly = (sy * kLogicalHeight) / winH;
    if (RGL.GetUpsideDown()) {
        lx = kLogicalWidth - lx;
        ly = kLogicalHeight - ly;
    }
    return RudeScreenVertex(lx, ly);
}

bool leftButtonDown = false;
RudeScreenVertex leftMousePos(0, 0);

void dispatchEvent(RBGame &game, const SDL_Event &event, int winW, int winH)
{
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            RudeScreenVertex point = mapCoord(event.button.x, event.button.y, winW, winH);
            if (leftButtonDown) {
                game.TouchUp(point, leftMousePos);
            }
            leftButtonDown = true;
            leftMousePos = point;
            game.TouchDown(leftMousePos);
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
            RudeScreenVertex point = mapCoord(event.button.x, event.button.y, winW, winH);
            if (leftButtonDown) {
                game.TouchUp(point, leftMousePos);
            }
            leftButtonDown = false;
            leftMousePos = point;
        }
        break;
    case SDL_MOUSEMOTION:
        if (leftButtonDown) {
            RudeScreenVertex newpos = mapCoord(event.motion.x, event.motion.y, winW, winH);
            game.TouchMove(newpos, leftMousePos);
            leftMousePos = newpos;
        }
        break;
    case SDL_MOUSEWHEEL: {
        RudeScreenVertex delta(event.wheel.x, event.wheel.y);
        game.ScrollWheel(delta);
        break;
    }
    default:
        break;
    }
}
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        std::fprintf(stderr, "SDL_image PNG support unavailable: %s\n", IMG_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    setGLAttributes(true);

    int windowWidth = kLogicalWidth;
    int windowHeight = kLogicalHeight;

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0 && dm.h > 0) {
        if (dm.h < 1080) {
            // For screens smaller than 1080 vertical (e.g. 768 or 800 or 900),
            // fit window within desktop height preserving 3:4 aspect ratio
            windowHeight = dm.h - 60;
            if (windowHeight < 480) windowHeight = 480;
            windowWidth = (windowHeight * kLogicalWidth) / kLogicalHeight;
        }
    }
    const char *envW = std::getenv("GOLF_WIDTH");
    const char *envH = std::getenv("GOLF_HEIGHT");
    if (envW && envH) {
        int w = std::atoi(envW);
        int h = std::atoi(envH);
        if (w >= 320 && h >= 240) {
            windowWidth = w;
            windowHeight = h;
        }
    }

    Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    const char *envFS = std::getenv("GOLF_FULLSCREEN");
    if (envFS && (std::strcmp(envFS, "1") == 0 || std::strcmp(envFS, "true") == 0)) {
        winFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Anytime Golf: Magic Touch",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        windowWidth,
        windowHeight,
        winFlags);
    if (window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        // Fall back to non-MSAA visual
        setGLAttributes(false);
        context = SDL_GL_CreateContext(window);
    }
    if (context == nullptr) {
        // Fall back to software visual if hardware visual failed
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 0);
        context = SDL_GL_CreateContext(window);
    }
    if (context == nullptr) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }
    SDL_GL_SetSwapInterval(1);

    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    const char *envUpsideDown = std::getenv("GOLF_UPSIDEDOWN");
    if (envUpsideDown && (std::strcmp(envUpsideDown, "1") == 0 ||
                          std::strcmp(envUpsideDown, "true") == 0)) {
        RGL.SetUpsideDown(true);
    }

    RGL.SetDeviceWidth(kLogicalWidth);
    RGL.SetDeviceHeight(kLogicalHeight);
    RGL.SetWindowSize(windowWidth, windowHeight);

    RudeDebug::Init();
    RudeUnitTest::UnitTest();
    RudeFontManager::InitFonts();
    RudeText::Init();

    RBGame game;
    bool running = true;
    int currentWinWidth = windowWidth;
    int currentWinHeight = windowHeight;

    Uint64 lastTicks = SDL_GetPerformanceCounter();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_u || event.key.keysym.sym == SDLK_f) {
                    RGL.SetUpsideDown(!RGL.GetUpsideDown());
                    std::printf("Orientation toggled: upsideDown = %s\n",
                                RGL.GetUpsideDown() ? "true" : "false");
                } else if (event.key.keysym.sym == SDLK_F11 ||
                           (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT))) {
                    Uint32 flags = SDL_GetWindowFlags(window);
                    if (flags & SDL_WINDOW_FULLSCREEN) {
                        SDL_SetWindowFullscreen(window, 0);
                    } else {
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                }
            } else if (event.type == SDL_WINDOWEVENT &&
                       event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                currentWinWidth = event.window.data1;
                currentWinHeight = event.window.data2;
                RGL.SetWindowSize(currentWinWidth, currentWinHeight);
                game.Resize();
            } else {
                dispatchEvent(game, event, currentWinWidth, currentWinHeight);
            }
        }

        const Uint64 now = SDL_GetPerformanceCounter();
        const float delta = static_cast<float>(now - lastTicks) /
                            static_cast<float>(SDL_GetPerformanceFrequency());
        lastTicks = now;

        RGL.FlushEnables();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        game.Render(delta, static_cast<float>(kLogicalWidth), static_cast<float>(kLogicalHeight));
        SDL_GL_SwapWindow(window);
        if (delta < (1.0f / 60.0f)) {
            SDL_Delay(static_cast<Uint32>((1.0f / 60.0f - delta) * 1000.0f));
        }
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}
