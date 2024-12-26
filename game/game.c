#include "game.h"

/****** Entities: Player ******/

static Level *get_next_level(GameManager *manager)
{
    Level *current_level = game_manager_current_level_get(manager);
    GameContext *game_context = game_manager_game_context_get(manager);
    for (int i = 0; i < game_context->level_count; i++)
    {
        if (game_context->levels[i] == current_level)
        {
            // check if i+1 is out of bounds, if so, return the first level
            game_context->current_level = (i + 1) % game_context->level_count;
            return game_context->levels[(i + 1) % game_context->level_count] ? game_context->levels[(i + 1) % game_context->level_count] : game_context->levels[0];
        }
    }
    game_context->current_level = 0;
    return game_context->levels[0] ? game_context->levels[0] : game_manager_add_level(manager, generic_level("town_world", 0));
}

void player_spawn(Level *level, GameManager *manager)
{
    GameContext *game_context = game_manager_game_context_get(manager);
    game_context->players[0] = level_add_entity(level, &player_desc);

    // Set player position.

    // Depends on your game logic, it can be done in start entity function, but also can be done here.
    entity_pos_set(game_context->players[0], (Vector){WORLD_WIDTH / 2, WORLD_HEIGHT / 2});

    // Add collision box to player entity
    // Box is centered in player x and y, and it's size is 10x10
    entity_collider_add_rect(game_context->players[0], 10 + PLAYER_COLLISION_HORIZONTAL, 10 + PLAYER_COLLISION_VERTICAL);

    // Get player context
    PlayerContext *player_context = entity_context_get(game_context->players[0]);

    // Load player sprite
    player_context->sprite_right = game_manager_sprite_load(manager, "player_right.fxbm");
    player_context->sprite_left = game_manager_sprite_load(manager, "player_left.fxbm");
    player_context->direction = PLAYER_RIGHT; // default direction
    player_context->health = 100;
    player_context->strength = 10;
    player_context->level = 1;
    player_context->xp = 0;
    player_context->start_position = entity_pos_get(game_context->players[0]);
    player_context->attack_timer = 0.5f;
    player_context->elapsed_attack_timer = player_context->attack_timer;

    // Set player username
    if (!load_char("Flip-Social-Username", player_context->username, 32))
    {
        snprintf(player_context->username, 32, "Player");
    }

    game_context->player_context = player_context;
}

// Modify player_update to track direction
static void player_update(Entity *self, GameManager *manager, void *context)
{
    PlayerContext *player = (PlayerContext *)context;
    InputState input = game_manager_input_get(manager);
    Vector pos = entity_pos_get(self);
    GameContext *game_context = game_manager_game_context_get(manager);

    // Increment the elapsed_attack_timer for the player
    player->elapsed_attack_timer += 1.0f / game_context->fps;

    // Store previous direction
    int prev_dx = player->dx;
    int prev_dy = player->dy;

    // Reset movement deltas each frame
    player->dx = 0;
    player->dy = 0;

    // Handle movement input
    if (input.held & GameKeyUp)
    {
        pos.y -= 2;
        player->dy = -1;
        player->direction = PLAYER_UP;
        game_context->user_input = GameKeyUp;
    }
    if (input.held & GameKeyDown)
    {
        pos.y += 2;
        player->dy = 1;
        player->direction = PLAYER_DOWN;
        game_context->user_input = GameKeyDown;
    }
    if (input.held & GameKeyLeft)
    {
        pos.x -= 2;
        player->dx = -1;
        player->direction = PLAYER_LEFT;
        game_context->user_input = GameKeyLeft;
    }
    if (input.held & GameKeyRight)
    {
        pos.x += 2;
        player->dx = 1;
        player->direction = PLAYER_RIGHT;
        game_context->user_input = GameKeyRight;
    }

    // Clamp the player's position to stay within world bounds
    pos.x = CLAMP(pos.x, WORLD_WIDTH - 5, 5);
    pos.y = CLAMP(pos.y, WORLD_HEIGHT - 5, 5);

    // Update player position
    entity_pos_set(self, pos);

    // switch levels if holding OK
    if (input.held & GameKeyOk)
    {
        // if all enemies are dead, allow the "OK" button to switch levels
        // otherwise the "OK" button will be used to attack
        if (game_context->enemy_count == 0)
        {
            game_manager_next_level_set(manager, get_next_level(manager));
            furi_delay_ms(500);
        }
        else
        {
            game_context->user_input = GameKeyOk;
            furi_delay_ms(100);
        }
        return;
    }

    // If the player is not moving, retain the last movement direction
    if (player->dx == 0 && player->dy == 0)
    {
        player->dx = prev_dx;
        player->dy = prev_dy;
        player->state = PLAYER_IDLE;
        game_context->user_input = -1; // reset user input
    }
    else
    {
        player->state = PLAYER_MOVING;
    }

    // Handle back button to stop the game
    if (input.pressed & GameKeyBack)
    {
        game_manager_game_stop(manager);
    }
}

static void player_render(Entity *self, GameManager *manager, Canvas *canvas, void *context)
{
    // Get player context
    UNUSED(manager);
    PlayerContext *player = context;

    // Get player position
    Vector pos = entity_pos_get(self);

    // Draw background (updates camera_x and camera_y)
    draw_background(canvas, pos);

    // Draw player sprite relative to camera, centered on the player's position
    canvas_draw_sprite(
        canvas,
        player->direction == PLAYER_RIGHT ? player->sprite_right : player->sprite_left,
        pos.x - camera_x - 5, // Center the sprite horizontally
        pos.y - camera_y - 5  // Center the sprite vertically
    );

    // draw username over player's head
    canvas_set_font_custom(canvas, FONT_SIZE_SMALL);
    canvas_draw_str(canvas, pos.x - camera_x - (strlen(player->username) * 2), pos.y - camera_y - 7, player->username);
}

const EntityDescription player_desc = {
    .start = NULL,                         // called when entity is added to the level
    .stop = NULL,                          // called when entity is removed from the level
    .update = player_update,               // called every frame
    .render = player_render,               // called every frame, after update
    .collision = NULL,                     // called when entity collides with another entity
    .event = NULL,                         // called when entity receives an event
    .context_size = sizeof(PlayerContext), // size of entity context, will be automatically allocated and freed
};

/****** Game ******/
/*
    Write here the start code for your game, for example: creating a level and so on.
    Game context is allocated (game.context_size) and passed to this function, you can use it to store your game data.
*/
static void game_start(GameManager *game_manager, void *ctx)
{
    // Do some initialization here, for example you can load score from storage.
    // For simplicity, we will just set it to 0.
    GameContext *game_context = ctx;
    game_context->fps = game_fps_choices_2[game_fps_index];
    game_context->player_context = NULL;

    // open the world list from storage, then create a level for each world
    char file_path[128];
    snprintf(file_path, sizeof(file_path), STORAGE_EXT_PATH_PREFIX "/apps_data/flip_world/worlds/world_list.json");
    FuriString *world_list = flipper_http_load_from_file(file_path);
    if (!world_list)
    {
        FURI_LOG_E("Game", "Failed to load world list");
        game_context->levels[0] = game_manager_add_level(game_manager, generic_level("town_world", 0));
        game_context->level_count = 1;
        return;
    }
    for (int i = 0; i < 10; i++)
    {
        FuriString *world_name = get_json_array_value_furi("worlds", i, world_list);
        if (!world_name)
        {
            break;
        }
        game_context->levels[i] = game_manager_add_level(game_manager, generic_level(furi_string_get_cstr(world_name), i));
        furi_string_free(world_name);
        game_context->level_count++;
    }
    furi_string_free(world_list);

    // add one enemy
    game_context->enemies[0] = level_add_entity(game_context->levels[0], enemy(game_manager,
                                                                               "player",
                                                                               0,
                                                                               (Vector){10, 10},
                                                                               (Vector){WORLD_WIDTH / 2 + 11, WORLD_HEIGHT / 2 + 16},
                                                                               (Vector){WORLD_WIDTH / 2 - 11, WORLD_HEIGHT / 2 + 16},
                                                                               1,
                                                                               32,
                                                                               0.5f,
                                                                               10,
                                                                               100));

    // add another enemy
    game_context->enemies[1] = level_add_entity(game_context->levels[0], enemy(game_manager,
                                                                               "player",
                                                                               1,
                                                                               (Vector){10, 10},
                                                                               (Vector){WORLD_WIDTH / 2 + 11, WORLD_HEIGHT / 2 + 32},
                                                                               (Vector){WORLD_WIDTH / 2 - 11, WORLD_HEIGHT / 2 + 32},
                                                                               1,
                                                                               32,
                                                                               0.5f,
                                                                               10,
                                                                               100));

    game_context->enemy_count = 2;
    game_context->current_level = 0;
}

/*
    Write here the stop code for your game, for example, freeing memory, if it was allocated.
    You don't need to free level, sprites or entities, it will be done automatically.
    Also, you don't need to free game_context, it will be done automatically, after this function.
*/
static void game_stop(void *ctx)
{
    GameContext *game_context = ctx;
    // If you want to do other final logic (like saving scores), do it here.
    // But do NOT free levels[] if the engine manages them.

    // Just clear out your pointer array if you like (not strictly necessary)
    for (int i = 0; i < game_context->level_count; i++)
    {
        game_context->levels[i] = NULL;
    }
    game_context->level_count = 0;
}

/*
    Your game configuration, do not rename this variable, but you can change its content here.
*/

const Game game = {
    .target_fps = 0,                     // set to 0 because we set this in game_app (callback.c line 22)
    .show_fps = false,                   // show fps counter on the screen
    .always_backlight = true,            // keep display backlight always on
    .start = game_start,                 // will be called once, when game starts
    .stop = game_stop,                   // will be called once, when game stops
    .context_size = sizeof(GameContext), // size of game context
};
