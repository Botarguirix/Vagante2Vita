#include "campaign_flow.h"
#include <assert.h>

int main(void) {
    assert(campaign_exit_action(0, 0, 0, 0, 0, 0) == CAMPAIGN_EXIT_NONE);
    assert(campaign_exit_action(0, 1, 1, 0, 0, 0) == CAMPAIGN_EXIT_NONE);
    assert(campaign_exit_action(0, 0, 1, 0, 0, 0) == CAMPAIGN_EXIT_ADVANCE_FLOOR);
    assert(campaign_exit_action(0, 0, 1, 1, 0, 0) == CAMPAIGN_EXIT_BLOCKED_BOSS);
    assert(campaign_exit_action(0, 0, 1, 1, 1, 2) == CAMPAIGN_EXIT_ADVANCE_FLOOR);
    assert(campaign_exit_action(1, 0, 1, 0, 0, 0) == CAMPAIGN_EXIT_ADVANCE_FLOOR);
    assert(campaign_exit_action(2, 0, 1, 0, 0, 0) == CAMPAIGN_EXIT_BLOCKED_BOSS);
    assert(campaign_exit_action(2, 0, 1, 1, 0, 0) == CAMPAIGN_EXIT_BLOCKED_BOSS);
    assert(campaign_exit_action(2, 0, 1, 2, 1, 1) == CAMPAIGN_EXIT_BLOCKED_BOSS);
    assert(campaign_exit_action(2, 0, 1, 1, 1, 1) == CAMPAIGN_EXIT_BLOCKED_ROSTER);
    assert(campaign_exit_action(2, 0, 1, 1, 1, 7) == CAMPAIGN_EXIT_COMPLETE);
    assert(campaign_exit_action(-1, 0, 1, 0, 0, 0) == CAMPAIGN_EXIT_ENTER_ACT_ONE);
    assert(campaign_exit_action(3, 0, 1, 0, 0, 0) == CAMPAIGN_EXIT_NONE);
    return 0;
}
