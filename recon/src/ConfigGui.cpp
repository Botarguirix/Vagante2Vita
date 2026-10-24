// ConfigGui.cpp: PDB contract + Ghidra 00583980 onwards.
#include "recon_all.h"

// ConfigGui::ConfigGui @ 00583980. Background sprite arguments recovered from the call site
// (tools/call_args.py): 32x32 frames, no animation. The cursor starts at (139, 110).
ConfigGui::ConfigGui(sf::RenderWindow &window_, Config &config_, Input &input_)
    : exitToMainMenu(false), exitGame(false), window(window_), view(window_.getView()),
      config(config_), input(input_), open(false), netGame(false), frame(0), openingTimer(0),
      closingTimer(0), switchTimer(0), screen(MAIN_SCREEN), mousePos(0.0f, 0.0f),
      mouseLastMovedTimer(70), selectPosition(0, 0),
      bgSprite(std::weak_ptr<Entity>(), "gui/gui_background.png", 32, 32, -1, 0, 0.0f, 0.0f, false, false),
      guiCursor(139.0f, 110.0f), exitButton(nullptr), exitDialogWindow(nullptr),
      exitDialogChoiceOne(nullptr), exitDialogChoiceTwo(nullptr), exitDialogCancel(nullptr),
      creditsButton(nullptr), inputSelection(nullptr), shaderWarningLabel(nullptr),
      statisticsInfoLabel(nullptr), controlsButtonsOffset(0), gammaSlider(nullptr),
      zoomSlider(nullptr), controlsBackButton(nullptr), loadProfileButton(nullptr),
      controlsRebindAllButton(nullptr), enablePlayerCheckbox(nullptr), currentPlayer(0),
      hoveredElement(nullptr), mouseAfk(false), didMouseHover(false),
      profileSelectionOpen(false), runCredits(false) {
    // profileSelection, profileSelectLabel and inputSelectLabel are left for initialize().
    initialize();
    choosePlayer(0);
}
