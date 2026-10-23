// Translated from main @ 006a0570 and its lambdas (vagante.exe 1.10.1).
//
// Not ported, on purpose:
//  - Steamworks (SteamAPI_Init, SteamController, Workshop localization wait, lobby join).
//  - ImGui::SFML (Init/ProcessEvent/Update/Render): only the editors use it.
//  - Win32 console placement, SetThreadStackGuarantee, SetThreadExecutionState.
// Constants recovered from the disassembly: loading-screen sleep 20 ms, unfocused sleep 1 ms,
// loading bar margin 16 px and ghost alpha 50, fixed step Client::dt = 16666 us, catch-up cap 33333 us.
#include "recon_all.h"

#include <cstdlib>
#include <ctime>
#include <thread>

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

#if defined(__vita__)
#include <unistd.h>
// Vita process limits: a large newlib heap (textures are decoded on the CPU first) and a
// main thread stack comparable to the 1 MB + 100 KB guarantee the Windows build asks for.
extern "C" {
int _newlib_heap_size_user = 256 * 1024 * 1024;
unsigned int sceUserMainThreadStackSize = 4 * 1024 * 1024;
}
#endif

namespace recon { const char *glVendor(); const char *glRenderer(); }  // port/gl_info.cpp

namespace {

struct GamepadInit {          // captures of <lambda_8b1a2341dd97765e2ecb263044d86059>
    bool *steamInitialized;
    bool *steamControllerInitialized;
    bool *tryController;      // config.firstTimePlaying, or SteamTenfoot (Big Picture)
    Input *input;
};

void initGamepads(const GamepadInit &g) {
    // Steam: SteamUtils() overlay position and SteamController()->Init() would run here.
    if (*g.tryController) {
        g.input->updateGamepadStatus();
        g.input->tryToUseAController();
    } else if (*g.steamInitialized && *g.steamControllerInitialized) {
        g.input->updateGamepadStatus();
    }
}

// <lambda_ee8f0838ff528ae0e933898b4823bf9c>: "+connect <ip>" / "+connect_lobby <id>" (Steam launch options).
void parseLaunchOptions(const std::string &args, std::string &target, bool &connect, bool &joinLobby) {
    std::size_t pos = args.find("+connect ");
    if (pos != std::string::npos) {
        target = args.substr(pos);
        connect = true;
        VLOG(LOG_DEBUG) << "Command line args, connecting to specific IP: " << target;
    }
    pos = args.find("+connect_lobby ");
    if (pos != std::string::npos) {
        target = args.substr(pos);
        joinLobby = true;
        VLOG(LOG_DEBUG) << "Command line args, connecting to specific steam lobby: " << target;
    }
}

void logContextSettings(const sf::ContextSettings &cs) {
    VLOG(LOG_INFO) << "ContextSettings retrieved:";
    VLOG(LOG_INFO) << "";
    VLOG(LOG_INFO) << "============ CONTEXT SETTINGS =============";
    const char *vendor = recon::glVendor(), *renderer = recon::glRenderer();
    VLOG(LOG_INFO) << "\tOpenGL Vendor:\"" << (vendor ? vendor : "") << "\"";
    VLOG(LOG_INFO) << "\tOpenGL Renderer:\"" << (renderer ? renderer : "") << "\"";
    VLOG(LOG_INFO) << "\tOpenGL version: " << cs.majorVersion << "." << cs.minorVersion;
    VLOG(LOG_INFO) << "\tDepth bits: " << cs.depthBits;
    VLOG(LOG_INFO) << "\tStencil bits: " << cs.stencilBits;
    VLOG(LOG_INFO) << "\tAntialiasing level: " << cs.antialiasingLevel;
    VLOG(LOG_INFO) << "===========================================";
    VLOG(LOG_INFO) << "";
}

}  // namespace

int main(int argc, char **argv) {
#if defined(__vita__)
    chdir("ux0:data/Vagante/");  // data.vra, config/ and logs/ live next to each other, as on PC
#endif
    sf::Clock startupClock;
    Log::init(LOG_DEBUG4);  // the original ignores the argument and sets reportingLevel to 10

    bool fullscreenConsole = false, bottomLeftConsole = false, loadThreads = true;
    bool ignoreLoadScreen = false, ignoreSteam = false;
    const std::string version = GAME_VERSION + PATCH_VERSION;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg.find("--generate-vra") != std::string::npos) {
            std::printf("\n\n\nVagante Version %s, \n\n\n\n", version.c_str());
            ResourceManager::initialize(true, true, true);
            ResourceManager::finishInitialization(false);
            ResourceManager::cleanUp();
            std::printf("\nDone.\n");
            return 0;
        }
        if (arg.find("--fullscreen-console") != std::string::npos) fullscreenConsole = true;
        else if (arg.find("--bottom-left-console") != std::string::npos) bottomLeftConsole = true;
        else if (arg.find("--load-threads-off") != std::string::npos) loadThreads = false;
        else if (arg.find("--ignore-load-screen") != std::string::npos) ignoreLoadScreen = true;
        else ignoreSteam = arg.find("--ignore-steam") != std::string::npos;
    }
    (void)fullscreenConsole; (void)bottomLeftConsole;  // Win32 console placement only

    VLOG(LOG_INFO) << "Vagante Version " << version;
    VLOG(LOG_INFO) << getOperatingSystemName();

    bool steamInitialized = false;  // SteamAPI_Init() unless --ignore-steam; Steam is not ported
    (void)ignoreSteam;
    VLOG(LOG_INFO) << "Steam not available in this build.";

    _randEngine.seed(std::random_device()());

    const std::time_t now = std::time(nullptr);
    const std::tm *t = std::localtime(&now);
    VLOG(LOG_INFO) << (t->tm_year + 1900) << "-" << (t->tm_mon + 1) << "-" << t->tm_mday << " "
                   << t->tm_hour << ":" << t->tm_min;
    VLOG(LOG_INFO) << " ";
    VLOG(LOG_INFO) << "Initializing EJoystick...";
    EJoystick::initialize(nullptr);  // GetModuleHandleA(nullptr) on Windows

    Input input(nullptr);
    VLOG(LOG_INFO) << "Input initialized.";

    Config config;
    ResourceManager::setConfig(&config);
    config.load(input);
    input.load();
    VLOG(LOG_INFO) << "Config loaded.";

    bool steamControllerInitialized = false;
    sf::RenderWindow window;
    std::thread resourceThread, gamepadThread;
    if (!loadThreads) {
        ResourceManager::initialize(config.loadFromArchive, false, false);
        Entity::initEntityStringIdentifiers();
        ItemAttribute::initAttributeStrings();
        ResourceManager::finishInitialization(false);
    } else {
        Config *cfg = &config;
        resourceThread = std::thread([cfg] {            // <lambda_7edb72b04bb0f93b0bed94ef1ec389ec>
            ResourceManager::initialize(cfg->loadFromArchive, false, true);
            Entity::initEntityStringIdentifiers();
            ItemAttribute::initAttributeStrings();
        });
    }

    bool tryController = config.firstTimePlaying;
    if (const char *tenfoot = std::getenv("SteamTenfoot")) {
        if (std::strtol(tenfoot, nullptr, 10) == 1) {
            VLOG(LOG_DEBUG) << "SteamTenFoot environment variable set. Going full screen..";
            config.fullScreen = true;
            config.steamBigPicture = true;
            tryController = true;
        }
    }
    config.updateWindowSize(window);

    GamepadInit gamepads{&steamInitialized, &steamControllerInitialized, &tryController, &input};
    if (!loadThreads) initGamepads(gamepads);
    else gamepadThread = std::thread([&gamepads] { initGamepads(gamepads); });  // <lambda_418e...>

    UGCBrowser *ugcBrowser = new UGCBrowser(input, window);
    if (window.isOpen()) {
        VLOG(LOG_INFO) << "sf::RenderWindow successfully initialized.";
    } else {
        VLOG(LOG_ERROR) << "sf::RenderWindow failed to be created. (window.isOpen() == false).";
    }
    input.setWindow(&window);
    logContextSettings(window.getSettings());

    if (loadThreads) {
        // Loading screen: the loading texture fills from left to right as resource tasks complete.
        sf::Sprite *loadingSprite = nullptr;
        int barWidth = 0, barHeight = 0;
        while (!ResourceManager::isInitialized()) {
            if (!window.isOpen() || ignoreLoadScreen) break;
            if (ResourceManager::initThreadsAreDone())
                ResourceManager::finishInitialization(false);
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    ResourceManager::cancelInitialization();
                    window.close();
                }
            }
            if (!window.isOpen()) break;
            window.clear(sf::Color::Black);
            if (!loadingSprite) {
                if (const sf::Texture *texture = ResourceManager::getNewLoadingTexture()) {
                    loadingSprite = new sf::Sprite(*texture);
                    barWidth = loadingSprite->getTextureRect().width;
                    barHeight = loadingSprite->getTextureRect().height;
                }
            } else {
                const sf::Vector2u size = window.getSize();
                loadingSprite->setPosition(static_cast<float>(size.x - barWidth) - 16.0f,
                                           static_cast<float>(size.y - barHeight) - 16.0f);
                loadingSprite->setTextureRect(sf::IntRect(0, 0, barWidth, barHeight));
                loadingSprite->setColor(sf::Color(255, 255, 255, 50));
                window.draw(*loadingSprite);
                const float progress = ResourceManager::initTaskProgress();
                loadingSprite->setTextureRect(sf::IntRect(0, 0, static_cast<int>(barWidth * progress), barHeight));
                loadingSprite->setColor(sf::Color(255, 255, 255, 255));
                window.draw(*loadingSprite);
            }
            window.display();
            sf::sleep(sf::milliseconds(20));
        }
        delete loadingSprite;
        resourceThread.join();
        gamepadThread.join();
    }

    // Workshop localization (UGC items of type 1) needs Steam: skipped.
    input.updateActionNames();
    Skill::initSkillStringIdentifiers();
    Stats::init();
    VLOG(LOG_INFO) << "Loading finished.";

    if (window.isOpen()) {
        Client client(window, input, config, ugcBrowser, argc, argv);
        std::unique_ptr<ThreadCanary> canary;
        if (loadThreads) canary = std::make_unique<ThreadCanary>("Client Canary", 10);
        ThreadCanary *clientCanary = canary.get();
        SetThreadName(0, "Vagante Client");  // GetCurrentThreadId() on Windows
        client.resized();
        client.dt = 16666;      // fixed step in microseconds (60 Hz)
        client.realTime = 0;
        const auto start = std::chrono::steady_clock::now();
        auto previous = start;
        unsigned long long accumulator = client.dt;
        VLOG(LOG_INFO) << "Client initialized! Time spent: " << startupClock.getElapsedTime().asMilliseconds() << " ms\n";

        std::string launchTarget;
        bool connect = false, joinLobby = false;
        std::string joined;
        for (int i = 1; i < argc; ++i) {
            joined += argv[i];
            if (i < argc - 1) joined += " ";
        }
        parseLaunchOptions(joined, launchTarget, connect, joinLobby);
        if (connect) client.commandLineConnect(launchTarget);
        if (joinLobby) {
            VLOG(LOG_ERROR) << "Unable to command line join lobby: SteamMatchmaking() is nullptr.";
        }

        bool focused = true;
        window.requestFocus();
        while (window.isOpen()) {
            if (clientCanary) clientCanary->check();
            sf::Event event;
            while (window.pollEvent(event)) {
                switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::Resized:
                    VLOG(LOG_DEBUG) << "sf::Event::Resized";
                    client.resized();
                    break;
                case sf::Event::LostFocus:
                    focused = false;
                    break;
                case sf::Event::GainedFocus:
                    focused = true;
                    break;
                case sf::Event::MouseWheelMoved:
                    client.mouseWheelEvent(event.mouseWheel.delta);
                    break;
                case sf::Event::MouseButtonPressed:
                case sf::Event::MouseButtonReleased:
                    client.mouseAction(event.mouseButton.button, event.mouseButton.x, event.mouseButton.y,
                                       event.type == sf::Event::MouseButtonPressed);
                    break;
                case sf::Event::MouseMoved:
                    client.mousePosition = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
                    client.mouseHideTimer = 0;
                    break;
                case sf::Event::JoystickConnected:
                    input.joystickConnectEvent(event.joystickConnect.joystickId);
                    break;
                case sf::Event::JoystickDisconnected:
                    input.joystickDisconnectEvent(event.joystickConnect.joystickId);
                    break;
                default:
                    if (input.enteringText) {
                        if (event.type == sf::Event::TextEntered) input.inputText(event.text.unicode);
                        else if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased)
                            input.enteringTextKeyEvent(event.key, event.type == sf::Event::KeyPressed);
                    }
                    break;
                }
            }
            if (loadThreads && ignoreLoadScreen && ResourceManager::initThreadsAreDone() &&
                !ResourceManager::isInitialized()) {
                ResourceManager::finishInitialization(true);
                if (ResourceManager::isInitialized()) ResourceManager::playSound("menu/gain_affinity_point.wav", false);
            }
            {
                std::lock_guard<std::mutex> lock(EJoystick::gamepadUpdateMutex);
                for (const auto &e : EJoystick::connectDisconnectEvents) {
                    if (e.connect) input.joystickConnectEvent(e.joystickId);
                    else input.joystickDisconnectEvent(e.joystickId);
                }
                EJoystick::connectDisconnectEvents.clear();
            }
            EJoystick::update();

            const auto current = std::chrono::steady_clock::now();
            accumulator += std::chrono::duration_cast<std::chrono::nanoseconds>(current - previous).count() / 1000;
            previous = current;
            client.realTime = std::chrono::duration_cast<std::chrono::nanoseconds>(current - start).count() / 1000;
            if (accumulator > 33332 && client.dt < 33333) accumulator = 33333;  // at most ~2 frames of catch-up
            if (accumulator >= client.dt) {   // one fixed step per rendered frame
                accumulator -= client.dt;
                client.focused = focused;
                if (config.doResized) { client.resized(); config.doResized = false; }
                if (config.doLanguageChange) { client.updateLanguage(); config.doLanguageChange = false; }
                client.step();
            }
            window.clear(sf::Color::Black);
            client.draw(static_cast<float>(accumulator) / static_cast<float>(client.dt));
            window.resetGLStates();
            window.display();
            if (!config.verticalSync && !window.hasFocus()) sf::sleep(sf::milliseconds(1));
        }
        if (clientCanary) clientCanary->disable();
    }
    window.close();
    ResourceManager::cleanUp();
    Message::cleanupHuffmanTree();
    EJoystick::cleanUp();
    VLOG(LOG_INFO) << "Cleanup complete!\n";
    return 0;
}
