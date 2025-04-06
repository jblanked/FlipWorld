#include "game/icon.h"

static void icon_collision(Entity *self, Entity *other, GameManager *manager, void *context)
{
    UNUSED(manager);
    UNUSED(self);
    IconContext *ictx = (IconContext *)context;
    if (ictx && entity_description_get(other) == &player_desc)
    {
        PlayerContext *player = (PlayerContext *)entity_context_get(other);
        if (player)
        {
            // Set the player's old position to prevent collision
            entity_pos_set(other, player->old_position);

            // Reset movement to prevent re-collision
            player->dx = 0;
            player->dy = 0;
        }
    }
}

static void icon_render(Entity *self, GameManager *manager, Canvas *canvas, void *context)
{
    UNUSED(manager);
    IconContext *ictx = (IconContext *)context;
    furi_check(ictx, "Icon context is NULL");
    Vector pos = entity_pos_get(self);
    int x_pos = pos.x - camera_x - ictx->size.x / 2;
    int y_pos = pos.y - camera_y - ictx->size.y / 2;
    // check if position is within the screen
    if (x_pos + ictx->size.x < 0 || x_pos > SCREEN_WIDTH || y_pos + ictx->size.y < 0 || y_pos > SCREEN_HEIGHT)
        return;
    canvas_draw_icon(canvas, x_pos, y_pos, ictx->icon);
}

static void icon_start(Entity *self, GameManager *manager, void *context)
{
    UNUSED(manager);

    IconContext *ictx_self = (IconContext *)context;
    if (!ictx_self)
    {
        FURI_LOG_E("Game", "Icon context self is NULL");
        return;
    }
    IconContext *ictx = entity_context_get(self);
    if (!ictx)
    {
        FURI_LOG_E("Game", "Icon context is NULL");
        return;
    }

    IconContext *loaded_data = get_icon_context(g_name);
    if (!loaded_data)
    {
        FURI_LOG_E("Game", "Failed to find icon data for %s", g_name);
        return;
    }

    ictx_self->icon = loaded_data->icon;
    ictx_self->size = (Vector){loaded_data->size.x, loaded_data->size.y};
    ictx->icon = loaded_data->icon;
    ictx->size = (Vector){loaded_data->size.x, loaded_data->size.y};

    Vector pos = entity_pos_get(self);
    pos.x += ictx_self->size.x / 2;
    pos.y += ictx_self->size.y / 2;
    entity_pos_set(self, pos);

    entity_collider_add_circle(
        self,
        (ictx_self->size.x + ictx_self->size.y) / 4);

    free(loaded_data);
}

// -------------- Entity description --------------
const EntityDescription icon_desc = {
    .start = icon_start,
    .stop = NULL,
    .update = NULL,
    .render = icon_render,
    .collision = icon_collision,
    .event = NULL,
    .context_size = sizeof(IconContext),
};

static IconContext *icon_generic_alloc(IconID id, const Icon *icon, uint8_t width, uint8_t height)
{
    IconContext *ctx = malloc(sizeof(IconContext));
    if (!ctx)
    {
        FURI_LOG_E("Game", "Failed to allocate IconContext");
        return NULL;
    }
    ctx->id = id;
    ctx->icon = icon;
    ctx->size = (Vector){width, height};
    return ctx;
}

IconContext *get_icon_context(const char *name)
{
    if (is_str(name, "house"))
        return icon_generic_alloc(ICON_ID_HOUSE, &I_icon_house_48x32px, 48, 32);
    else if (is_str(name, "man"))
        return icon_generic_alloc(ICON_ID_MAN, &I_icon_man_7x16, 7, 16);
    else if (is_str(name, "plant"))
        return icon_generic_alloc(ICON_ID_PLANT, &I_icon_plant_16x16, 16, 16);
    else if (is_str(name, "tree"))
        return icon_generic_alloc(ICON_ID_TREE, &I_icon_tree_16x16, 16, 16);
    else if (is_str(name, "woman"))
        return icon_generic_alloc(ICON_ID_WOMAN, &I_icon_woman_9x16, 9, 16);
    else if (is_str(name, "fence"))
        return icon_generic_alloc(ICON_ID_FENCE, &I_icon_fence_16x8px, 16, 8);
    else if (is_str(name, "fence_end"))
        return icon_generic_alloc(ICON_ID_FENCE_END, &I_icon_fence_end_16x8px, 16, 8);
    else if (is_str(name, "fence_vertical_end"))
        return icon_generic_alloc(ICON_ID_FENCE_VERTICAL_END, &I_icon_fence_vertical_end_6x8px, 6, 8);
    else if (is_str(name, "fence_vertical_start"))
        return icon_generic_alloc(ICON_ID_FENCE_VERTICAL_START, &I_icon_fence_vertical_start_6x15px, 6, 15);
    else if (is_str(name, "flower"))
        return icon_generic_alloc(ICON_ID_FLOWER, &I_icon_flower_16x16, 16, 16);
    else if (is_str(name, "lake_bottom"))
        return icon_generic_alloc(ICON_ID_LAKE_BOTTOM, &I_icon_lake_bottom_31x12px, 31, 12);
    else if (is_str(name, "lake_bottom_left"))
        return icon_generic_alloc(ICON_ID_LAKE_BOTTOM_LEFT, &I_icon_lake_bottom_left_24x22px, 24, 22);
    else if (is_str(name, "lake_bottom_right"))
        return icon_generic_alloc(ICON_ID_LAKE_BOTTOM_RIGHT, &I_icon_lake_bottom_right_24x22px, 24, 22);
    else if (is_str(name, "lake_left"))
        return icon_generic_alloc(ICON_ID_LAKE_LEFT, &I_icon_lake_left_11x31px, 11, 31);
    else if (is_str(name, "lake_right"))
        return icon_generic_alloc(ICON_ID_LAKE_RIGHT, &I_icon_lake_right_11x31, 11, 31);
    else if (is_str(name, "lake_top"))
        return icon_generic_alloc(ICON_ID_LAKE_TOP, &I_icon_lake_top_31x12px, 31, 12);
    else if (is_str(name, "lake_top_left"))
        return icon_generic_alloc(ICON_ID_LAKE_TOP_LEFT, &I_icon_lake_top_left_24x22px, 24, 22);
    else if (is_str(name, "lake_top_right"))
        return icon_generic_alloc(ICON_ID_LAKE_TOP_RIGHT, &I_icon_lake_top_right_24x22px, 24, 22);
    else if (is_str(name, "rock_large"))
        return icon_generic_alloc(ICON_ID_ROCK_LARGE, &I_icon_rock_large_18x19px, 18, 19);
    else if (is_str(name, "rock_medium"))
        return icon_generic_alloc(ICON_ID_ROCK_MEDIUM, &I_icon_rock_medium_16x14px, 16, 14);
    else if (is_str(name, "rock_small"))
        return icon_generic_alloc(ICON_ID_ROCK_SMALL, &I_icon_rock_small_10x8px, 10, 8);

    // If no match is found
    FURI_LOG_E("Game", "Icon not found: %s", name);
    return NULL;
}