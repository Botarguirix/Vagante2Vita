#pragma once

typedef enum {
    CAMPAIGN_EXIT_NONE = 0,
    CAMPAIGN_EXIT_BLOCKED_BOSS,
    CAMPAIGN_EXIT_BLOCKED_ROSTER,
    CAMPAIGN_EXIT_ADVANCE_FLOOR,
    CAMPAIGN_EXIT_COMPLETE,
    CAMPAIGN_EXIT_ENTER_ACT_ONE
} CampaignExitAction;

CampaignExitAction campaign_exit_action(int floor_index, int player_dead,
                                        int at_exit, unsigned bosses_loaded,
                                        unsigned bosses_defeated,
                                        unsigned defeated_boss_mask);
