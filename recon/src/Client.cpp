// Client: the top-level game object owned by main(). Translated from vagante.exe 1.10.1.
//
// Ghidra analysed Client with a stale definition, so its pseudocode names the wrong members.
// The layout here follows recon/include/Client.h (verified against the PDB) and every value
// was read from the instructions with tools/field_writes.py and tools/call_args.py.
#include "recon_all.h"

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

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
    chatTextBox.hidden = true;  // GuiTextBox +0x2C

    VLOG(LOG_DEBUG) << "Client::argc:" << argc;
    for (int i = 0; i < argc; ++i) VLOG(LOG_DEBUG) << "\t" << argv[i];
    // SetUnhandledExceptionFilter(windowsExceptionHandler): Windows crash reporting, not ported.

    introBackdropSprite.setOrigin(0.0f, 0.0f);
    introForegroundSprite.setOrigin(0.0f, 0.0f);
    fairyTransitionBackdropSprite.setOrigin(0.0f, 0.0f);
    fairyTransitionForegroundSprite.setOrigin(0.0f, 0.0f);
    emptyTransitionSprite.setOrigin(0.0f, 0.0f);
    levelTextSlideInTween.endWaitTime = 60.0f;

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
