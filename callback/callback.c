#include <callback/callback.h>

static void frame_cb(GameEngine *engine, Canvas *canvas, InputState input, void *context)
{
    UNUSED(engine);
    GameManager *game_manager = context;
    game_manager_input_set(game_manager, input);
    game_manager_update(game_manager);
    game_manager_render(game_manager, canvas);
}

static int32_t game_app(void *p)
{
    UNUSED(p);
    GameManager *game_manager = game_manager_alloc();
    if (!game_manager)
    {
        FURI_LOG_E("Game", "Failed to allocate game manager");
        return -1;
    }
    GameEngineSettings settings = game_engine_settings_init();
    settings.target_fps = game.target_fps;
    settings.show_fps = game.show_fps;
    settings.always_backlight = game.always_backlight;
    settings.frame_callback = frame_cb;
    settings.context = game_manager;

    GameEngine *engine = game_engine_alloc(settings);
    if (!engine)
    {
        FURI_LOG_E("Game", "Failed to allocate game engine");
        game_manager_free(game_manager);
        return -1;
    }
    game_manager_engine_set(game_manager, engine);

    void *game_context = NULL;
    if (game.context_size > 0)
    {
        game_context = malloc(game.context_size);
        game_manager_game_context_set(game_manager, game_context);
    }
    game.start(game_manager, game_context);

    game_engine_run(engine);
    game_engine_free(engine);

    game_manager_free(game_manager);

    game.stop(game_context);
    if (game_context)
    {
        free(game_context);
    }

    int32_t entities = entities_get_count();
    if (entities != 0)
    {
        FURI_LOG_E("Game", "Memory leak detected: %ld entities still allocated", entities);
        return -1;
    }

    return 0;
}

static bool alloc_about_view(void *context);
static bool alloc_text_input_view(void *context, char *title);
static bool alloc_variable_item_list(void *context, uint32_t view_id);
//
static void wifi_settings_item_selected(void *context, uint32_t index);
static void text_updated_ssid(void *context);
static void text_updated_pass(void *context);
//
static void flip_world_game_fps_change(VariableItem *item);
static void game_settings_item_selected(void *context, uint32_t index);

uint32_t callback_to_submenu(void *context)
{
    UNUSED(context);
    return FlipWorldViewSubmenu;
}
static uint32_t callback_to_wifi_settings(void *context)
{
    UNUSED(context);
    return FlipWorldViewVariableItemList;
}
static uint32_t callback_to_settings(void *context)
{
    UNUSED(context);
    return FlipWorldViewSettings;
}

static void flip_world_view_about_draw_callback(Canvas *canvas, void *model)
{
    UNUSED(model);
    canvas_clear(canvas);
    canvas_set_font_custom(canvas, FONT_SIZE_XLARGE);
    canvas_draw_str(canvas, 0, 10, VERSION_TAG);
    canvas_set_font_custom(canvas, FONT_SIZE_MEDIUM);
    canvas_draw_str(canvas, 0, 20, "- @JBlanked @codeallnight");
    canvas_set_font_custom(canvas, FONT_SIZE_SMALL);
    canvas_draw_str(canvas, 0, 30, "- github.com/JBlanked/FlipWorld");

    canvas_draw_str_multi(canvas, 0, 55, "The first open world multiplayer\ngame on the Flipper Zero.");
}

// alloc
static bool alloc_about_view(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return false;
    }
    if (!app->view_about)
    {
        if (!easy_flipper_set_view(&app->view_about, FlipWorldViewAbout, flip_world_view_about_draw_callback, NULL, callback_to_submenu, &app->view_dispatcher, app))
        {
            return false;
        }
        if (!app->view_about)
        {
            return false;
        }
    }
    return true;
}

static bool alloc_text_input_view(void *context, char *title)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return false;
    }
    if (!title)
    {
        FURI_LOG_E(TAG, "Title is NULL");
        return false;
    }
    app->text_input_buffer_size = 64;
    if (!app->text_input_buffer)
    {
        if (!easy_flipper_set_buffer(&app->text_input_buffer, app->text_input_buffer_size))
        {
            return false;
        }
    }
    if (!app->text_input_temp_buffer)
    {
        if (!easy_flipper_set_buffer(&app->text_input_temp_buffer, app->text_input_buffer_size))
        {
            return false;
        }
    }
    if (!app->text_input)
    {
        if (!easy_flipper_set_uart_text_input(
                &app->text_input,
                FlipWorldViewTextInput,
                title,
                app->text_input_temp_buffer,
                app->text_input_buffer_size,
                strcmp(title, "SSID") == 0 ? text_updated_ssid : text_updated_pass,
                callback_to_wifi_settings,
                &app->view_dispatcher,
                app))
        {
            return false;
        }
        if (!app->text_input)
        {
            return false;
        }
        char ssid[64];
        char pass[64];
        if (load_settings(ssid, sizeof(ssid), pass, sizeof(pass)))
        {
            if (strcmp(title, "SSID") == 0)
            {
                strncpy(app->text_input_temp_buffer, ssid, app->text_input_buffer_size);
            }
            else
            {
                strncpy(app->text_input_temp_buffer, pass, app->text_input_buffer_size);
            }
        }
    }
    return true;
}
static bool alloc_variable_item_list(void *context, uint32_t view_id)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return false;
    }
    if (!app->variable_item_list)
    {
        switch (view_id)
        {
        case FlipWorldSubmenuIndexWiFiSettings:
            if (!easy_flipper_set_variable_item_list(&app->variable_item_list, FlipWorldViewVariableItemList, wifi_settings_item_selected, callback_to_settings, &app->view_dispatcher, app))
            {
                FURI_LOG_E(TAG, "Failed to allocate variable item list");
                return false;
            }

            if (!app->variable_item_list)
            {
                FURI_LOG_E(TAG, "Variable item list is NULL");
                return false;
            }

            if (!app->variable_item_wifi_ssid)
            {
                app->variable_item_wifi_ssid = variable_item_list_add(app->variable_item_list, "SSID", 0, NULL, NULL);
                variable_item_set_current_value_text(app->variable_item_wifi_ssid, "");
            }
            if (!app->variable_item_wifi_pass)
            {
                app->variable_item_wifi_pass = variable_item_list_add(app->variable_item_list, "Password", 0, NULL, NULL);
                variable_item_set_current_value_text(app->variable_item_wifi_pass, "");
            }
            char ssid[64];
            char pass[64];
            if (load_settings(ssid, sizeof(ssid), pass, sizeof(pass)))
            {
                variable_item_set_current_value_text(app->variable_item_wifi_ssid, ssid);
                // variable_item_set_current_value_text(app->variable_item_wifi_pass, pass);
                save_char("WiFi-SSID", ssid);
                save_char("WiFi-Password", pass);
            }
            break;
        case FlipWorldSubmenuIndexGameSettings:
            if (!easy_flipper_set_variable_item_list(&app->variable_item_list, FlipWorldViewVariableItemList, game_settings_item_selected, callback_to_settings, &app->view_dispatcher, app))
            {
                FURI_LOG_E(TAG, "Failed to allocate variable item list");
                return false;
            }

            if (!app->variable_item_list)
            {
                FURI_LOG_E(TAG, "Variable item list is NULL");
                return false;
            }

            if (!app->variable_item_game_fps)
            {
                app->variable_item_game_fps = variable_item_list_add(app->variable_item_list, "FPS", 4, flip_world_game_fps_change, NULL);
                variable_item_set_current_value_index(app->variable_item_game_fps, 0);
                variable_item_set_current_value_text(app->variable_item_game_fps, game_fps_choices[0]);
            }
            char _game_fps[8];
            if (load_char("Game-FPS", _game_fps, sizeof(_game_fps)))
            {
                int index = strcmp(_game_fps, "30") == 0 ? 0 : strcmp(_game_fps, "60") == 0 ? 1
                                                           : strcmp(_game_fps, "120") == 0  ? 2
                                                           : strcmp(_game_fps, "240") == 0  ? 3
                                                                                            : 0;
                variable_item_set_current_value_text(app->variable_item_game_fps, game_fps_choices[index]);
                variable_item_set_current_value_index(app->variable_item_game_fps, index);
                snprintf(game_fps, 8, "%s", _game_fps);
            }
            break;
        }
    }
    return true;
}
static bool alloc_submenu_settings(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return false;
    }
    if (!app->submenu_settings)
    {
        if (!easy_flipper_set_submenu(&app->submenu_settings, FlipWorldViewSettings, "Settings", callback_to_submenu, &app->view_dispatcher))
        {
            return NULL;
        }
        if (!app->submenu_settings)
        {
            return false;
        }
        submenu_add_item(app->submenu_settings, "WiFi", FlipWorldSubmenuIndexWiFiSettings, callback_submenu_choices, app);
        submenu_add_item(app->submenu_settings, "Game", FlipWorldSubmenuIndexGameSettings, callback_submenu_choices, app);
        submenu_add_item(app->submenu_settings, "User", FlipWorldSubmenuIndexUserSettings, callback_submenu_choices, app);
    }
    return true;
}
// free
static void free_about_view(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    if (app->view_about)
    {
        view_dispatcher_remove_view(app->view_dispatcher, FlipWorldViewAbout);
        view_free(app->view_about);
        app->view_about = NULL;
    }
}
static void free_main_view(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    if (app->view_main)
    {
        view_dispatcher_remove_view(app->view_dispatcher, FlipWorldViewMain);
        view_free(app->view_main);
        app->view_main = NULL;
    }
}

static void free_text_input_view(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    if (app->text_input)
    {
        view_dispatcher_remove_view(app->view_dispatcher, FlipWorldViewTextInput);
        uart_text_input_free(app->text_input);
        app->text_input = NULL;
    }
    if (app->text_input_buffer)
    {
        free(app->text_input_buffer);
        app->text_input_buffer = NULL;
    }
    if (app->text_input_temp_buffer)
    {
        free(app->text_input_temp_buffer);
        app->text_input_temp_buffer = NULL;
    }
}
static void free_variable_item_list(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    if (app->variable_item_list)
    {
        view_dispatcher_remove_view(app->view_dispatcher, FlipWorldViewVariableItemList);
        variable_item_list_free(app->variable_item_list);
        app->variable_item_list = NULL;
    }
    if (app->variable_item_wifi_ssid)
    {
        free(app->variable_item_wifi_ssid);
        app->variable_item_wifi_ssid = NULL;
    }
    if (app->variable_item_wifi_pass)
    {
        free(app->variable_item_wifi_pass);
        app->variable_item_wifi_pass = NULL;
    }
    if (app->variable_item_game_fps)
    {
        free(app->variable_item_game_fps);
        app->variable_item_game_fps = NULL;
    }
    if (app->variable_item_user_username)
    {
        free(app->variable_item_user_username);
        app->variable_item_user_username = NULL;
    }
    if (app->variable_item_user_password)
    {
        free(app->variable_item_user_password);
        app->variable_item_user_password = NULL;
    }
}
static void free_submenu_settings(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    if (app->submenu_settings)
    {
        view_dispatcher_remove_view(app->view_dispatcher, FlipWorldViewSettings);
        submenu_free(app->submenu_settings);
        app->submenu_settings = NULL;
    }
}
static FuriThreadId thread_id;
static bool game_thread_running = false;
void free_all_views(void *context, bool should_free_variable_item_list, bool should_free_submenu_settings)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    if (should_free_variable_item_list)
    {
        free_variable_item_list(app);
    }
    free_about_view(app);
    free_main_view(app);
    free_text_input_view(app);
    if (app->view_main)
    {
        view_free(app->view_main);
        view_dispatcher_remove_view(app->view_dispatcher, FlipWorldViewMain);
        app->view_main = NULL;
    }
    // free game thread
    if (game_thread_running)
    {
        game_thread_running = false;
        furi_thread_flags_set(thread_id, WorkerEvtStop);
        furi_thread_free(thread_id);
    }

    if (should_free_submenu_settings)
        free_submenu_settings(app);
}

void callback_submenu_choices(void *context, uint32_t index)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    switch (index)
    {
    case FlipWorldSubmenuIndexRun:
        // free game thread
        if (game_thread_running)
        {
            game_thread_running = false;
            furi_thread_flags_set(thread_id, WorkerEvtStop);
            furi_thread_free(thread_id);
        }
        free_all_views(app, true, true);
        if (!app->view_main)
        {
            if (!easy_flipper_set_view(&app->view_main, FlipWorldViewMain, NULL, NULL, callback_to_submenu, &app->view_dispatcher, app))
            {
                return;
            }
            if (!app->view_main)
            {
                return;
            }
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewMain);
        FuriThread *thread = furi_thread_alloc_ex("game", 4096, game_app, app);
        if (!thread)
        {
            FURI_LOG_E(TAG, "Failed to allocate game thread");
            return;
        }
        furi_thread_start(thread);
        thread_id = furi_thread_get_id(thread);
        game_thread_running = true;
        break;
    case FlipWorldSubmenuIndexAbout:
        free_all_views(app, true, true);
        if (!alloc_about_view(app))
        {
            FURI_LOG_E(TAG, "Failed to allocate about view");
            return;
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewAbout);
        break;
    case FlipWorldSubmenuIndexSettings:
        free_all_views(app, true, true);
        if (!alloc_submenu_settings(app))
        {
            FURI_LOG_E(TAG, "Failed to allocate settings view");
            return;
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewSettings);
        break;
    case FlipWorldSubmenuIndexWiFiSettings:
        free_all_views(app, true, false);
        if (!alloc_variable_item_list(app, FlipWorldSubmenuIndexWiFiSettings))
        {
            FURI_LOG_E(TAG, "Failed to allocate variable item list");
            return;
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewVariableItemList);
        break;
    case FlipWorldSubmenuIndexGameSettings:
        free_all_views(app, true, false);
        if (!alloc_variable_item_list(app, FlipWorldSubmenuIndexGameSettings))
        {
            FURI_LOG_E(TAG, "Failed to allocate variable item list");
            return;
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewVariableItemList);
        break;
    case FlipWorldSubmenuIndexUserSettings:
        easy_flipper_dialog("User Settings", "Coming soon...");
        break;
    default:
        break;
    }
}

static void text_updated_ssid(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }

    // store the entered text
    strncpy(app->text_input_buffer, app->text_input_temp_buffer, app->text_input_buffer_size);

    // Ensure null-termination
    app->text_input_buffer[app->text_input_buffer_size - 1] = '\0';

    // save the setting
    save_char("WiFi-SSID", app->text_input_buffer);

    // update the variable item text
    if (app->variable_item_wifi_ssid)
    {
        variable_item_set_current_value_text(app->variable_item_wifi_ssid, app->text_input_buffer);

        // get value of password
        char pass[64];
        if (load_char("WiFi-Password", pass, sizeof(pass)))
        {
            if (strlen(pass) > 0 && strlen(app->text_input_buffer) > 0)
            {
                // save the settings
                save_settings(app->text_input_buffer, pass);

                // initialize the http
                if (flipper_http_init(flipper_http_rx_callback, app))
                {
                    // save the wifi if the device is connected
                    if (!flipper_http_save_wifi(app->text_input_buffer, pass))
                    {
                        easy_flipper_dialog("FlipperHTTP Error", "Ensure your WiFi Developer\nBoard or Pico W is connected\nand the latest FlipperHTTP\nfirmware is installed.");
                    }

                    // free the resources
                    flipper_http_deinit();
                }
                else
                {
                    easy_flipper_dialog("FlipperHTTP Error", "The UART is likely busy.\nEnsure you have the correct\nflash for your board then\nrestart your Flipper Zero.");
                }
            }
        }
    }

    // switch to the settings view
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewVariableItemList);
}
static void text_updated_pass(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }

    // store the entered text
    strncpy(app->text_input_buffer, app->text_input_temp_buffer, app->text_input_buffer_size);

    // Ensure null-termination
    app->text_input_buffer[app->text_input_buffer_size - 1] = '\0';

    // save the setting
    save_char("WiFi-Password", app->text_input_buffer);

    // update the variable item text
    if (app->variable_item_wifi_pass)
    {
        // variable_item_set_current_value_text(app->variable_item_wifi_pass, app->text_input_buffer);
    }

    // get value of ssid
    char ssid[64];
    if (load_char("WiFi-SSID", ssid, sizeof(ssid)))
    {
        if (strlen(ssid) > 0 && strlen(app->text_input_buffer) > 0)
        {
            // save the settings
            save_settings(ssid, app->text_input_buffer);

            // initialize the http
            if (flipper_http_init(flipper_http_rx_callback, app))
            {
                // save the wifi if the device is connected
                if (!flipper_http_save_wifi(ssid, app->text_input_buffer))
                {
                    easy_flipper_dialog("FlipperHTTP Error", "Ensure your WiFi Developer\nBoard or Pico W is connected\nand the latest FlipperHTTP\nfirmware is installed.");
                }

                // free the resources
                flipper_http_deinit();
            }
            else
            {
                easy_flipper_dialog("FlipperHTTP Error", "The UART is likely busy.\nEnsure you have the correct\nflash for your board then\nrestart your Flipper Zero.");
            }
        }
    }

    // switch to the settings view
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewVariableItemList);
}

static void wifi_settings_item_selected(void *context, uint32_t index)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    char ssid[64];
    char pass[64];
    switch (index)
    {
    case 0: // Input SSID
        free_all_views(app, false, false);
        if (!alloc_text_input_view(app, "SSID"))
        {
            FURI_LOG_E(TAG, "Failed to allocate text input view");
            return;
        }
        // load SSID
        if (load_settings(ssid, sizeof(ssid), pass, sizeof(pass)))
        {
            strncpy(app->text_input_temp_buffer, ssid, app->text_input_buffer_size - 1);
            app->text_input_temp_buffer[app->text_input_buffer_size - 1] = '\0';
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewTextInput);
        break;
    case 1: // Input Password
        free_all_views(app, false, false);
        if (!alloc_text_input_view(app, "Password"))
        {
            FURI_LOG_E(TAG, "Failed to allocate text input view");
            return;
        }
        // load password
        if (load_settings(ssid, sizeof(ssid), pass, sizeof(pass)))
        {
            strncpy(app->text_input_temp_buffer, pass, app->text_input_buffer_size - 1);
            app->text_input_temp_buffer[app->text_input_buffer_size - 1] = '\0';
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewTextInput);
        break;
    default:
        FURI_LOG_E(TAG, "Unknown configuration item index");
        break;
    }
}
static void flip_world_game_fps_change(VariableItem *item)
{
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, game_fps_choices[index]);

    // save the fps
    snprintf(game_fps, 8, "%s", game_fps_choices[index]);
    save_char("Game-FPS", game_fps);
}

static void game_settings_item_selected(void *context, uint32_t index)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    switch (index)
    {
    case 0:    // Game FPS
        break; // handled by flip_world_game_fps_change
    }
}