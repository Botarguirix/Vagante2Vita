#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int path_join(char *out, size_t size, const char *root, const char *tail) {
    int n = snprintf(out, size, "%s/%s", root, tail);
    return n > 0 && (size_t)n < size;
}

static int load_floor(World *world, const char *root, unsigned floor,
                      FILE *log) {
    char world_path[1024], entity_path[1024], enemy_path[1024];
    char melee_path[1024], bow_path[1024], loot_path[1024], attack_path[1024];
    if (floor == 0) {
        if (!path_join(world_path, sizeof(world_path), root, "act-one-world.vwi") ||
            !path_join(entity_path, sizeof(entity_path), root, "act-one-entities.vwe") ||
            !path_join(enemy_path, sizeof(enemy_path), root, "act-one-enemies.vwe")) return 0;
    } else {
        char tail[96];
        snprintf(tail, sizeof(tail), "floors/floor-%02u.vwi", floor);
        if (!path_join(world_path, sizeof(world_path), root, tail)) return 0;
        snprintf(tail, sizeof(tail), "floors/floor-%02u-entities.vwe", floor);
        if (!path_join(entity_path, sizeof(entity_path), root, tail)) return 0;
        snprintf(tail, sizeof(tail), "floors/floor-%02u-enemies.vwe", floor);
        if (!path_join(enemy_path, sizeof(enemy_path), root, tail)) return 0;
    }
    if (!path_join(melee_path, sizeof(melee_path), root, "intro-melee.vwa") ||
        !path_join(bow_path, sizeof(bow_path), root, "intro-bow.vwb") ||
        !path_join(loot_path, sizeof(loot_path), root, "item-loot.vlt") ||
        !path_join(attack_path, sizeof(attack_path), root, "act-one-goblin-attack.vga")) return 0;
    return world_load(world, world_path, log) &&
           world_load_entities(world, entity_path, log) &&
           world_load_enemies(world, enemy_path, log) &&
           melee_load(&world->melee, melee_path, log) &&
           bow_load(&world->bow, bow_path, log) &&
           world_load_loot(world, loot_path, log) &&
           world_load_goblin_attack(world, attack_path, log);
}

static int read_trace(const char *path, uint16_t **masks, unsigned *count) {
    FILE *file = fopen(path, "rb");
    unsigned char header[8];
    if (!file || fread(header, 1, sizeof(header), file) != sizeof(header) ||
        memcmp(header, "VTR1", 4)) {
        if (file) fclose(file);
        return 0;
    }
    *count = (unsigned)header[4] | (unsigned)header[5] << 8 |
             (unsigned)header[6] << 16 | (unsigned)header[7] << 24;
    if (!*count || *count > 36000) { fclose(file); return 0; }
    *masks = calloc(*count, sizeof(**masks));
    if (!*masks) { fclose(file); return 0; }
    for (unsigned i = 0; i < *count; ++i) {
        unsigned char bytes[2];
        if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
            free(*masks); *masks = NULL; fclose(file); return 0;
        }
        (*masks)[i] = (uint16_t)(bytes[0] | (unsigned)bytes[1] << 8);
        if ((*masks)[i] & ~0xf3f9u) {
            free(*masks); *masks = NULL; fclose(file); return 0;
        }
    }
    int valid = fgetc(file) == EOF;
    fclose(file);
    if (!valid) { free(*masks); *masks = NULL; return 0; }
    return 1;
}

static unsigned player_input(uint16_t buttons) {
    unsigned input = 0;
    if (buttons & 0x0010) input |= PLAYER_UP;
    if (buttons & 0x0020) input |= PLAYER_RIGHT;
    if (buttons & 0x0040) input |= PLAYER_DOWN;
    if (buttons & 0x0080) input |= PLAYER_LEFT;
    if (buttons & 0x0100) input |= PLAYER_SLOW;
    if (buttons & 0x0200) input |= PLAYER_RANGE;
    if (buttons & 0x4000) input |= PLAYER_JUMP;
    if (buttons & 0x8000) input |= PLAYER_ATTACK;
    return input;
}

static int proof_matches_carried_state(const char *trace_root, unsigned floor,
                                       int player_class, int hp, int max_hp,
                                       unsigned arrows) {
    char path[1024];
    if (snprintf(path, sizeof(path), "%s/floor-%02u.json", trace_root, floor) <= 0)
        return 0;
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    char text[8192];
    size_t length = fread(text, 1, sizeof(text) - 1u, file);
    int valid = !ferror(file);
    fclose(file);
    if (!valid) return 0;
    text[length] = '\0';
    for (size_t i = 0; i < length; ++i) {
        if (text[i] == '\r') {
            memmove(text + i, text + i + 1u, length - i);
            --length;
            --i;
        }
    }
    char expected[160];
    snprintf(expected, sizeof(expected),
             "\"player_class\": %d,", player_class);
    if (!strstr(text, expected)) return 0;
    snprintf(expected, sizeof(expected),
             "\"hp\": %d,\n    \"max_hp\": %d,\n    \"arrows\": %u",
             hp, max_hp, arrows);
    return strstr(text, expected) != NULL;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: campaign-chain CAMPAIGN_ROOT TRACE_ROOT PLAYER_CLASS\n");
        return 2;
    }
    const char *campaign = argv[1], *trace_root = argv[2];
    int player_class = atoi(argv[3]);
    if (player_class < 0 || player_class > 2) return 2;
    World *previous_world = NULL;
    NativePlayer previous_player;
    int has_previous = 0;
    for (unsigned floor = 0; floor < 3; ++floor) {
        World *world = calloc(1, sizeof(*world));
        if (!world || !load_floor(world, campaign, floor, stdout)) {
            fprintf(stderr, "campaign-chain could not load floor=%u\n", floor);
            free(world); free(previous_world); return 2;
        }
        world->loot.enabled = 1;
        world->loot_level = (int)floor;
        NativePlayer player;
        if (!player_spawn(&player, world, player_class) ||
            (has_previous && !player_carry_campaign_state(&player, &previous_player, world))) {
            fprintf(stderr, "campaign-chain could not spawn/carry player floor=%u\n", floor);
            free(world); free(previous_world); return 2;
        }
        if (has_previous) world->bosses_defeated_mask = previous_world->bosses_defeated_mask;
        unsigned starting_arrows = 0;
        for (unsigned i = 0; i < player.loadout.item_count; ++i)
            if (player.loadout.items[i].type == 3) starting_arrows += player.loadout.items[i].quantity;
        fprintf(stdout, "CAMPAIGN_CHAIN_START floor=%u class=%d hp=%d/%d items=%u bow=%d arrows=%u carried=%d\n",
                floor, player.player_class, player.hp, player.max_hp,
                player.loadout.item_count, player.bow.definition != NULL,
                starting_arrows, has_previous);
        if (!proof_matches_carried_state(trace_root, floor, player_class,
                                         player.hp, player.max_hp, starting_arrows)) {
            fprintf(stderr, "campaign-chain proof state mismatch floor=%u class=%d hp=%d/%d arrows=%u\n",
                    floor, player_class, player.hp, player.max_hp, starting_arrows);
            free(world); free(previous_world); return 9;
        }
        char trace_path[1024];
        snprintf(trace_path, sizeof(trace_path), "%s/floor-%02u.vtr", trace_root, floor);
        uint16_t *masks = NULL; unsigned count = 0;
        if (!read_trace(trace_path, &masks, &count)) {
            fprintf(stderr, "campaign-chain could not read trace floor=%u path=%s\n", floor, trace_path);
            free(world); free(previous_world); return 2;
        }
        int reached = world_goal_touched(world, player.x, player.y, player.w, player.h);
        unsigned tick = 0;
        for (; tick < count && !reached && !player.dead; ++tick) {
            player_step(&player, world, player_input(masks[tick]));
            (void)world_boulder_traps_step(world, player.x, player.y, player.w, player.h);
            world_boulders_step(world);
            (void)player_boulder_contact(&player, world);
            reached = world_goal_touched(world, player.x, player.y, player.w, player.h);
            if ((tick + 1u) % 120u == 0u) {
                unsigned living_common = 0;
                float boss_hp = -1.0f;
                for (unsigned i = 0; i < world->entity_count; ++i) {
                    WorldEntity *entity = &world->entities[i];
                    if (entity->kind == 24 && entity->hp > 0.0f) living_common++;
                    if (entity->kind == WORLD_KIND_BOSS) boss_hp = entity->hp;
                }
                fprintf(stdout, "CAMPAIGN_CHAIN_PROGRESS floor=%u tick=%u hp=%d/%d common_alive=%u boss_hp=%.1f arrows_fired=%u arrow_hits=%u x=%.2f y=%.2f\n",
                        floor, tick + 1u, player.hp, player.max_hp,
                        living_common, boss_hp, player.arrows.fired,
                        player.arrows.enemy_hits, player.x, player.y);
            }
        }
        free(masks);
        unsigned boss_count = 0;
        for (unsigned i = 0; i < world->entity_count; ++i)
            boss_count += world->entities[i].kind == WORLD_KIND_BOSS && world->entities[i].hp <= 0.0f;
        fprintf(stdout, "CAMPAIGN_CHAIN floor=%u ticks=%u/%u hp=%d/%d dead=%d bosses_defeated=%u mask=%u reached_exit=%d\n",
                floor, tick, count, player.hp, player.max_hp, player.dead,
                world->bosses_defeated, world->bosses_defeated_mask, reached);
        if (player.dead || !reached || world->bosses_defeated != 1 || boss_count != 1) {
            fprintf(stderr, "campaign-chain failed floor=%u (trace proved independently, not with carried state)\n", floor);
            free(world); free(previous_world); return 1;
        }
        free(previous_world);
        previous_world = world;
        previous_player = player;
        has_previous = 1;
    }
    if (!previous_world || previous_world->bosses_defeated_mask != 7u) {
        fprintf(stderr, "campaign-chain did not preserve the complete boss roster\n");
        free(previous_world); return 1;
    }
    puts("CAMPAIGN_CHAIN passed=1 floors=3 transitions=2 boss_mask=7");
    free(previous_world);
    return 0;
}
