#include "campaign_flow.h"

CampaignExitAction campaign_exit_action(int floor_index, int player_dead,
                                        int at_exit, unsigned bosses_loaded,
                                        unsigned bosses_defeated,
                                        unsigned defeated_boss_mask) {
    if (player_dead || !at_exit) return CAMPAIGN_EXIT_NONE;
    if (floor_index >= 0 && floor_index < 2 && bosses_loaded &&
        bosses_defeated < bosses_loaded)
        return CAMPAIGN_EXIT_BLOCKED_BOSS;
    if (floor_index == 2) {
        if (!bosses_loaded || bosses_defeated < bosses_loaded)
            return CAMPAIGN_EXIT_BLOCKED_BOSS;
        if ((defeated_boss_mask & 7u) != 7u)
            return CAMPAIGN_EXIT_BLOCKED_ROSTER;
        return CAMPAIGN_EXIT_COMPLETE;
    }
    if (floor_index >= 0 && floor_index < 2)
        return CAMPAIGN_EXIT_ADVANCE_FLOOR;
    if (floor_index == -1) return CAMPAIGN_EXIT_ENTER_ACT_ONE;
    return CAMPAIGN_EXIT_NONE;
}
