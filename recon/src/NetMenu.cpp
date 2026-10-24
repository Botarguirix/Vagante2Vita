// NetMenu.cpp: PDB contract + Ghidra 006b7550 onwards.
//
// The network menu only matters for online play (Steam lobbies, direct IP), which the Vita port
// does not have. The constructor keeps every member initialization of the original so the class
// stays in a valid state for Client, but it does not build the online widgets (checkboxes, host /
// connect buttons, GuiFriendBrowser); resized() and updateStrings() lay those widgets out and are
// therefore not called. Client never opens the menu while networking is not ported.
#include "recon_all.h"

// NetMenu::NetMenu @ 006b7550. Values from tools/field_writes.py and tools/call_args.py.
NetMenu::NetMenu(sf::RenderWindow &window_, Input &input_, UGCBrowser *ugcBrowser_)
    : customWorld(false), selectingCustomWorld(false), animateInTimer(0), mouseStillTimer(0),
      currHovered(nullptr), frame(0), drawFrame(0.0f), lastRefreshed(0), window(window_),
      input(input_), ugcBrowser(ugcBrowser_), view(window_.getView()), mousePos(0.0f, 0.0f),
      directionHeld(4, false), framesHoldingDirection(4, std::pair<int, int>(0, 0)),
      cursorIndex(0), selectPos(1, 0), open(false), tryUseSteam(false), tryConnectLobby(false),
      retrieveLobbiesSpinnerAlpha(0.0f), addressTextBox(input_, 206.0f, 187.0f, 100.0f, 0.0f),
      targetIpAddress(""), gameName(""), setDefaultGameName(false), privateGameCheckbox(nullptr),
      customGameCheckbox(nullptr), gameNameField(nullptr), hostGameButton(nullptr),
      hostSteamGameButton(nullptr), connectToIpButton(nullptr), backButton(nullptr),
      refreshButton(nullptr), cancelButton(nullptr), friendBrowser(nullptr),
      guiCursor(512.0f, 384.0f), loading(false), loadingSubtext(""), loadingMusicFade(1.0f),
      spinnerSprite(std::weak_ptr<Entity>(), "gui/spinner.png", 16, 16, -1, 3, 0.0f, 0.0f, true, false),
      overlayDialogString(""), overlayDialogFrames(0) {
    addressTextBox.setHidden(true);
    spinnerSprite.setPosition(12.0f, 12.0f);
    // Not ported: gameNameField (GuiTextBox at 206,216 bound to gameName), privateGameCheckbox and
    // customGameCheckbox (400,236 / 400,256), the host/connect/back/refresh/cancel buttons
    // (x 400, y 276..359), the GuiFriendBrowser (400,16 103x180), resized() and updateStrings().
}
