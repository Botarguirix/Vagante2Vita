// Client: the top-level game object owned by main(). Translated from vagante.exe 1.10.1.
//
// Ghidra analysed Client with a stale definition, so its pseudocode names the wrong members.
// The layout here follows recon/include/Client.h (verified against the PDB) and every value
// was read from the instructions with tools/field_writes.py and tools/call_args.py.
#include "recon_all.h"

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

namespace {
// Client writes these members directly in the original; the PDB does not record the friendship,
// so explicit instantiation names them without touching the generated access labels.
template<class Tag, typename Tag::Type Member> struct MemberAccess {
    friend typename Tag::Type memberRef(Tag) { return Member; }
};
struct ForceHudUpdate { using Type = bool PlayerMenu::*; friend Type memberRef(ForceHudUpdate); };
struct GeneratorRoomDatabase { using Type = RoomDatabase *LevelGenerator::*; friend Type memberRef(GeneratorRoomDatabase); };
template struct MemberAccess<ForceHudUpdate, &PlayerMenu::forceHudUpdate>;
template struct MemberAccess<GeneratorRoomDatabase, &LevelGenerator::roomDatabase>;
}
// Client::Client @ 00549910
Client::Client(sf::RenderWindow &window, Input &input, Config &config, UGCBrowser *ugcBrowser, int argc, char **argv)
    : server(nullptr), window(window), doPostProcessing(false), renderTexture(nullptr), renderSprite(nullptr),
      input(input), config(config), focused(false), mouseHideTimer(0),
      gameEngine(), gameGui(),
      configGui(new ConfigGui(window, config, input)),
      progressionGui(), progressionGuiMenuOpen(false),
      chatTextBox(input, 640.0f, 0.0f, 0.0f, 0.0f),
      minimapTexture(nullptr), enteredTextHistoryIndex(0),
      ugcBrowser(ugcBrowser), netMenu(new NetMenu(window, input, ugcBrowser)),
      netLoadingFrames(0), connectAttempts(0), connectAttemptDelay(0),
      mainMenu(new MainMenu(window, input, *configGui, *netMenu)),
      introBackdropSprite(std::weak_ptr<Entity>(), "scenes/cavemouth/backdrop.png", -1, -1, -1, 0, 0.0f, 0.0f, false, false),
      introForegroundSprite(std::weak_ptr<Entity>(), "scenes/cavemouth/lighting.png", -1, -1, -1, 0, 0.0f, 0.0f, false, false),
      fairyTransitionBackdropSprite(std::weak_ptr<Entity>(), "scenes/fairytransition/backdrop.png", 400, 122, -1, 0, 0.0f, 0.0f, false, false),
      fairyTransitionForegroundSprite(std::weak_ptr<Entity>(), "scenes/fairytransition/foreground.png", 400, 122, -1, 0, 0.0f, 0.0f, false, false),
      emptyTransitionSprite(std::weak_ptr<Entity>(), "scenes/statue/transition.png", 400, 122, -1, 0, 0.0f, 0.0f, false, false),
      levelEditor(nullptr), weaponEditor(nullptr), paletteEditor(nullptr),
      gameView(), guiView(),
      trueViewScale(2.0f), viewCenter(0.0f, 0.0f), viewShakeOffset(0.0f, 0.0f), viewZoom(1.0f),
      paused(false), firstTimeBossMusic(false),
      musicState(MUSIC_NONE), altMusicState(ALT_MUSIC_NONE),
      // Tween(type, b, c, d): time, endWaitTime and pauseTime start at 0 (inline ctor in the original).
      levelTextSlideInTween(Tween::TWEEN_EASE_IN_OUT, 600.0f, -400.0f, 90.0f),
      levelTextSlideOutTween(Tween::TWEEN_QUARTIC_IN, 200.0f, -400.0f, 50.0f),
      gameOverTextTween(Tween::TWEEN_EASE_BOUNCE, 0.0f, 1.0f, 140.0f),
      gameOverSubTextTween(Tween::TWEEN_EASE_IN_OUT, 0.0f, 1.0f, 120.0f),
      transitionRadius(0.0f), screenScaled(0),
      prevTimeFps(std::chrono::steady_clock::now()), fps(0.0),
      showMouseCursor(true), debugEntities(0), debugAi(false), debugShowHotkeys(false),
      debugCollisionScheme(0), debugFpsRate(0),
      frameStepMode(false), debugDoStepFrame(false), debugEnableShader(false), debugSkipIntro(false),
      debugDrawExtraInfo(true), debugCheckViewFix(false), debugInputProfiles(false), debugCauseOfDeathSprites(false),
      debugDrawSounds(false), debugFluidUpdateInSeparateThread(0), debugFluidFps(0.0f), debugFluidFpsCount(0),
      fluidUpdateThread(nullptr), chatString(""),
      connectingRemotePort(0), unServerIP(0), usServerPort(0),
      netState(CLIENT_DISCONNECTED), snap(nullptr),
      netWrongFloorType(false), myPlayerIndex(0),
      frame(0), serverLocalIp(""), serverPublicIp(""),
      serverIntermission(false), serverIntro(false), wrongFloorTypeCounter(0),
      snapshotAck(0), ping(0), lastFrameRichPresenceUpdated(0),
      lastNetworkDataReceivedTime(0), pingSentTime(0), reconnectPlayerData(nullptr),
      reconnecting(false), reconnectingLoadedLevel(false),
      realTime(0), dt(0), avgDrawFrameTime(0.0f), mousePosition(0, 0),
      serverTime(0), targetServerTime(0), playerDps(0.0f), maxPlayerDPS(0.0f),
      gameSave(), testCurseFrames(0), mouseDragEntityOffset(0.0f, 0.0f),
      credits(nullptr), debugNetId(0) {
    debugRecording = DebugRecordingSettings();

    credits = new Credits(window, *configGui, gameEngine);
    debugNetId = -1;
    chatTextBox.setHidden(true);  // inline setter (GuiElement::hidden +0x2C)

    VLOG(LOG_DEBUG) << "Client::argc:" << argc;
    for (int i = 0; i < argc; ++i) VLOG(LOG_DEBUG) << "\t" << argv[i];
    // SetUnhandledExceptionFilter(windowsExceptionHandler): Windows crash reporting, not ported.

    introBackdropSprite.setOrigin(0.0f, 0.0f);
    introForegroundSprite.setOrigin(0.0f, 0.0f);
    fairyTransitionBackdropSprite.setOrigin(0.0f, 0.0f);
    fairyTransitionForegroundSprite.setOrigin(0.0f, 0.0f);
    emptyTransitionSprite.setOrigin(0.0f, 0.0f);
    levelTextSlideInTween.setEndWaitTime(60.0f);  // inline setter (+0x14)

    gameView.setSize(640.0f, 360.0f);
    window.setView(gameView);
    VLOG(LOG_DEBUG) << "Number of entity types: " << 126;

    Message::initializeHuffmanTree();
    updateSteamRichPresence();

    // Count: four player slots (Controller 0..3). The binary passes it in a register Ghidra lost.
    playerInformation.resize(4, PlayerInformation());

    VLOG(LOG_DEBUG) << "Constructing minimapTexture...";
    minimapTexture = new sf::RenderTexture();
    minimapTexture->create(256, 256, false);
    VLOG(LOG_DEBUG) << "Finished constructing minimapTexture.";
}

// Client::resized @ 00567750
// The world is authored for 640x360: the view scale is the largest factor that fits the window
// (1.5 on the Vita's 960x544), optionally truncated for pixel-perfect output.
void Client::resized() {
    const sf::Vector2u size = window.getSize();
    const float width = static_cast<float>(size.x), height = static_cast<float>(size.y);
    float scale = std::min(width * (1.0f / 640.0f), height * (1.0f / 360.0f));
    if (config.forcePerfectPixelScaling) scale = static_cast<float>(static_cast<int>(scale));
    trueViewScale = scale;
    if (!config.fullScreen) config.windowSize = sf::Vector2i(static_cast<int>(size.x), static_cast<int>(size.y));

    const float inverse = 1.0f / scale;
    gameView.setSize(width * inverse, height * inverse);
    window.setView(gameView);
    VLOG(LOG_INFO) << "Client::resized: [" << size.x << "," << size.y << "] {" << gameView.getSize().x << ","
                   << gameView.getSize().y << "}";
    guiView.setSize(width / trueViewScale, height / trueViewScale);
    guiView.setCenter(guiView.getSize().x * 0.5f, guiView.getSize().y * 0.5f);
    // levelEditor / weaponEditor / paletteEditor views: developer tools, not part of the port.

    gameGui.resized(window);
    configGui->resized();
    netMenu->resized();
    mainMenu->resized();
    credits->resized(window);

    const unsigned int maximumTextureSize = sf::Texture::getMaximumSize();
    if (sf::Shader::isAvailable()) {
        if (!ResourceManager::getShaderPointer("postprocessing.frag")) {
            VLOG(LOG_INFO) << "Client::resized: `postprocessing.frag` not found!";
            config.postProcessingEnabled = false;
        }
        if (maximumTextureSize <= size.x) {
            VLOG(LOG_INFO) << "Client::resized: Window width >= maximum texture size! " << size.x << " >= "
                           << maximumTextureSize;
            config.postProcessingEnabled = false;
        }
        if (maximumTextureSize <= size.y) {
            VLOG(LOG_INFO) << "Client::resized: Window height >= maximum texture size! " << size.y << " >= "
                           << maximumTextureSize;
            config.postProcessingEnabled = false;
        }
    } else {
        VLOG(LOG_INFO) << "Client::resized: Shaders are not available!";
        config.postProcessingEnabled = false;
    }
    config.saveIni();
    input.save();

    doPostProcessing = config.postProcessingEnabled;
    if (doPostProcessing) {
        VLOG(LOG_INFO) << "Client::resized: Shaders are available, and window size is smaller than maximum size ( "
                       << maximumTextureSize << " )";
        delete renderSprite;
        renderSprite = nullptr;
        if (!renderTexture) {
            VLOG(LOG_DEBUG) << "Client::resized: Creating new renderTexture for use in post processing...";
            renderTexture = new sf::RenderTexture();
        }
        if (renderTexture->create(size.x, size.y, false)) {
            renderTexture->clear(sf::Color(0, 0, 0, 255));
            renderTexture->display();
            renderSprite = new sf::Sprite(renderTexture->getTexture());
            config.updateGamma();
        } else {
            VLOG(LOG_INFO) << "Client::resized: Failed to create RenderTextuer of size: " << size.x << ", " << size.y;
            VLOG(LOG_INFO) << "Client::resized: Setting doPostProcessing = false.";
            config.postProcessingEnabled = false;
            doPostProcessing = false;
        }
    }
    VLOG(LOG_INFO) << "Client::resized: doPostProcessing == " << (doPostProcessing ? "true" : "false");
    // The editors' render targets (window or renderTexture) are re-pointed here in the original.

    for (const auto &menu : gameEngine.playerMenus) (*menu).*memberRef(ForceHudUpdate{}) = true;
    VLOG(LOG_DEBUG) << "Client::resized: Done.";
}

// Client::step @ 0055e500 (one fixed 60 Hz tick).
// Sections: 1) debug/frame bookkeeping, 2) shared game logic, 3) menus, 4) in-game update.
// Section 4 (input timers, character customization, music, entities, level, camera) is still
// being translated: it is only reached once a run has started.
void Client::step() {
    // 1) Debug frame stepping, cursor auto-hide, new game request, frame timing.
    if (frameStepMode && !debugDoStepFrame) {
        handleInput();
        return;
    }
    // Debug ImGui windows (input profile editor, entity inspector): developer tools, not ported.

    ++mouseHideTimer;
    const bool cursorVisible = mouseHideTimer < 120;
    if (showMouseCursor != cursorVisible) {
        showMouseCursor = cursorVisible;
        window.setMouseCursorVisible(cursorVisible);
    }

    if (gameEngine.doNewGame) {
        newGame(gameEngine.playerCustomizeMenu.getClasses(), gameEngine.playerCustomizeMenu.getPalettes());
        if (mainMenu->isOpen()) mainMenu->close();
    }

    const auto now = std::chrono::steady_clock::now();
    const unsigned long long elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - prevTimeFps).count() / 1000;  // microseconds
    prevTimeFps = now;
    serverTime += elapsed;
    targetServerTime += elapsed;
    ++debugFluidFpsCount;
    debugFluidFps += 1000000.0f / static_cast<float>(elapsed);
    if (elapsed != 0) fps = (1000000.0 / static_cast<double>(elapsed) - fps) * 0.025 + fps;
    ++frame;

    // levelEditor / weaponEditor / paletteEditor take over the whole step while editing:
    // developer tools, always null in the port.

    if (debugRecording.enabled) {
        if (debugRecording.fullBright && gameEngine.getLevel()) gameEngine.getLevel()->revealMap();
        // The original also adjusts every player for recording through processPlayers (debug only).
    }

    // 2) Input, credits, game logic, game over, save data.
    handleInput();
    if (credits->isTriggered()) {
        const bool wasOver = credits->creditsAreOver();
        credits->step();
        if (credits->creditsAreOver()) {
            if (!wasOver)
                for (const auto &menu : gameEngine.getPlayerMenus()) (*menu).*memberRef(ForceHudUpdate{}) = true;
            if (!gameEngine.gameComplete) {
                entityDrawOrder.clear();
                gameEngine.cleanUp();
                mainMenu->reset(false, false);
            }
            credits->setTriggered(false, false);
        }
    }

    gameEngine.step();
    if (gameEngine.networked && netState == CLIENT_CONNECTED_AUTHENTICATED) {
        const auto &players = gameEngine.getPlayers();
        if (myPlayerIndex < players.size() && players[myPlayerIndex]) {
            if (!myPlayer)
                VLOG(LOG_CLIENT) << "Setting myPlayer to gameEngine.players[" << static_cast<int>(myPlayerIndex) << "]";
            myPlayer = players[myPlayerIndex];
        }
    }
    if (gameEngine.gameOver || gameEngine.gameComplete) stepGameOver();
    if (gameEngine.requestGameSaveData) {
        std::vector<GameSave> saves = loadGameSaveData();
        gameEngine.loadGameSaveData(saves);
    }

    // 3) Options/progression GUI and the main or network menu.
    static NetMenuReturnValue prevStep = static_cast<NetMenuReturnValue>(0);
    if (!progressionGuiMenuOpen) {
        progressionGui.step(window, guiView, false);
        configGui->setInGameState(gameEngine.networked, mainMenu->isOpen(), netMenu->isOpen(), gameEngine.intro);
        if (configGui->isOpen()) {
            configGui->step();
            if (configGui->getScreen() == static_cast<ConfigGui::ConfigScreen>(3)) {
                progressionGuiMenuOpen = true;
            } else if (configGui->exitGame) {
                if (!gameEngine.gameOver && !gameEngine.gameComplete && !netMenu->isOpen() && !gameEngine.intro)
                    gameEngine.recordGameStats(false, false);
                window.close();
            } else if (configGui->exitToMainMenu) {
                if (gameEngine.customRoomDatabase) {
                    delete gameEngine.customRoomDatabase;
                    gameEngine.customRoomDatabase = nullptr;
                    gameEngine.levelGenerator.*memberRef(GeneratorRoomDatabase{}) = nullptr;
                }
                if (!gameEngine.gameOver && !gameEngine.gameComplete && !netMenu->isOpen() && !gameEngine.intro)
                    gameEngine.recordGameStats(false, false);
                VLOG(LOG_DEBUG) << "configGui->exitToMainMenu";
                configGui->toggle();
                configGui->exitToMainMenu = false;
                if (!netMenu->isOpen()) {
                    if (gameEngine.networked) {
                        mainMenu->reset(false, false);
                        if (netState != CLIENT_DISCONNECTED) disconnect();
                        gameEngine.setNetworked(false, false);
                        return;
                    }
                    Level *level = gameEngine.getLevel();
                    if (!level || !level->intro) gameEngine.doNewGame = true;
                    else mainMenu->reset(false, false);
                    entityDrawOrder.clear();
                    gameEngine.cleanUp();
                    return;
                }
                mainMenu->reset(false, true);
                netMenu->setOpen(false);
            }
        }

        if (!netMenu->isOpen()) {
            if (mainMenu->isOpen()) {
                mainMenu->setFocus(!configGui->isOpen());
                mainMenu->step();
                if (configGui->doRunCredits()) mainMenu->startCredits();
                if (mainMenu->openProgressionGui()) {
                    progressionGuiMenuOpen = true;
                    musicState = MUSIC_NONE;
                    return;
                }
                if (mainMenu->viewCredits()) {
                    credits->setTriggered(true, true);
                    musicState = MUSIC_NONE;
                    return;
                }
                if (mainMenu->isReadyToStart()) {
                    VLOG(LOG_DEBUG) << "Starting game from Main Menu.";
                    gameEngine.doNewGame = true;
                }
                musicState = MUSIC_NONE;
                return;
            }
        } else {
            // Online play menu. Lobbies, hosting and joining need Steam/networking, which the port
            // leaves out; only leaving the menu is handled.
            const NetMenuReturnValue result = netMenu->step();
            mainMenu->stepBackground(1.0f);
            if (static_cast<int>(result) == 1) {
                netMenu->setOpen(false);
                if (!mainMenu->isOpen()) mainMenu->reset(false, true);
                return;
            }
            if (static_cast<int>(result) > 2)
                VLOG(LOG_INFO) << "Client::step: network menu action " << static_cast<int>(result) << " not ported";
            prevStep = result;
            return;
        }
    } else {
        mainMenu->stepBackground(1.0f);
        progressionGuiMenuOpen = progressionGui.step(window, guiView, true);
        if (!progressionGuiMenuOpen && configGui->isOpen()) configGui->setScreen(static_cast<ConfigGui::ConfigScreen>(0));
    }

    // 4) In-game update: pending translation (Client::step @ 0055e500, from 00560136 onwards).
    static bool reported = false;
    if (!reported) {
        reported = true;
        RECON_UNIMPLEMENTED("Client::step (in-game section)");
    }
}
