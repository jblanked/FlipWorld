#pragma once
#include "engine/engine.h"
#include <flip_world.h>
#include <game/game.h>
#include "engine/sensors/imu.h"

#define MAX_ENEMIES 10
#define MAX_LEVELS 10
#define MAX_NPCS 10

typedef struct
{
    Vector old_position;        // previous position of the player
    EntityDirection direction;  // direction the player is facing
    EntityState state;          // current state of the player
    Vector start_position;      // starting position of the player
    Sprite *sprite_right;       // player sprite looking right
    Sprite *sprite_left;        // player sprite looking left
    int8_t dx;                  // x direction
    int8_t dy;                  // y direction
    uint32_t xp;                // experience points
    uint32_t level;             // player level
    uint32_t strength;          // player strength
    uint32_t health;            // player health
    uint32_t max_health;        // player maximum health
    uint32_t health_regen;      // player health regeneration rate per second/frame
    float elapsed_health_regen; // time elapsed since last health regeneration
    float attack_timer;         // Cooldown duration between attacks
    float elapsed_attack_timer; // Time elapsed since the last attack
    char username[32];          // player username
    bool left;                  // track player sprite direction
} PlayerContext;

// two screens for the game menu
typedef enum
{
    GAME_MENU_INFO, // level, health, xp, etc.
    GAME_MENU_MORE, // more settings
} GameMenuScreen;

typedef struct
{
    PlayerContext *player_context;
    Level *levels[MAX_LEVELS];
    Entity *enemies[MAX_ENEMIES];
    Entity *npcs[MAX_NPCS];
    Entity *player;
    float fps;
    int level_count;
    int enemy_count;
    int npc_count;
    int current_level;
    bool ended_early;
    Imu *imu;
    bool imu_present;
    //
    bool is_switching_level;
    bool is_menu_open;
    //
    uint32_t elapsed_button_timer;
    uint32_t last_button;
    //
    GameMenuScreen menu_screen;
    uint8_t menu_selection;
    //
    int icon_count;
    int icon_offset;
} GameContext;

typedef struct
{
    char id[16];
    char left_file_name[64];
    char right_file_name[64];
    uint8_t width;
    uint8_t height;
} SpriteContext;

extern const EntityDescription player_desc;
void player_spawn(Level *level, GameManager *manager);
SpriteContext *get_sprite_context(const char *name);
