#include <alloc/alloc.h>
#include <callback/callback.h>

/**
 * @brief Navigation callback for exiting the application
 * @param context The context - unused
 * @return next view id (VIEW_NONE to exit the app)
 */
static uint32_t callback_exit_app(void *context)
{
    UNUSED(context);
    return VIEW_NONE; // Return VIEW_NONE to exit the app
}

void *global_app;
void flip_world_show_submenu()
{
    FlipWorldApp *app = (FlipWorldApp *)global_app;
    if (app->submenu)
    {
        view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewSubmenu);
    }
}

bool alloc_view_loader(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return false;
    }
    if (!app->view_loader)
    {
        if (!easy_flipper_set_view(&app->view_loader, FlipWorldViewLoader, loader_draw_callback, NULL, callback_to_submenu, &app->view_dispatcher, app))
        {
            return false;
        }
        loader_init(app->view_loader);
    }
    if (!app->widget_result)
    {
        if (!easy_flipper_set_widget(&app->widget_result, FlipWorldViewWidgetResult, "", callback_to_submenu, &app->view_dispatcher))
        {
            return false;
        }
    }
    return true;
}

void free_view_loader(void *context)
{
    FlipWorldApp *app = (FlipWorldApp *)context;
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }
    // Free Widget(s)
    if (app->widget_result)
    {
        view_dispatcher_remove_view(app->view_dispatcher, FlipWorldViewWidgetResult);
        widget_free(app->widget_result);
        app->widget_result = NULL;
    }

    // Free View(s)
    if (app->view_loader)
    {
        view_dispatcher_remove_view(app->view_dispatcher, FlipWorldViewLoader);
        loader_free_model(app->view_loader);
        view_free(app->view_loader);
        app->view_loader = NULL;
    }
}

// Function to allocate resources for the FlipWorldApp
FlipWorldApp *flip_world_app_alloc()
{
    FlipWorldApp *app = (FlipWorldApp *)malloc(sizeof(FlipWorldApp));
    global_app = app;

    Gui *gui = furi_record_open(RECORD_GUI);

    // Allocate ViewDispatcher
    if (!easy_flipper_set_view_dispatcher(&app->view_dispatcher, gui, app))
    {
        return NULL;
    }
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, custom_event_callback);

    // Submenu
    if (!easy_flipper_set_submenu(&app->submenu, FlipWorldViewSubmenu, VERSION_TAG, callback_exit_app, &app->view_dispatcher))
    {
        return NULL;
    }

    submenu_add_item(app->submenu, "Play", FlipWorldSubmenuIndexGameSubmenu, callback_submenu_choices, app);
    submenu_add_item(app->submenu, "About", FlipWorldSubmenuIndexMessage, callback_submenu_choices, app);
    submenu_add_item(app->submenu, "Settings", FlipWorldSubmenuIndexSettings, callback_submenu_choices, app);

    // Switch to the main view
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipWorldViewSubmenu);

    return app;
}

// Function to free the resources used by FlipWorldApp
void flip_world_app_free(FlipWorldApp *app)
{
    if (!app)
    {
        FURI_LOG_E(TAG, "FlipWorldApp is NULL");
        return;
    }

    // Free Submenu(s)
    if (app->submenu)
    {
        view_dispatcher_remove_view(app->view_dispatcher, FlipWorldViewSubmenu);
        submenu_free(app->submenu);
    }

    free_all_views(app, true, true, true);

    // free the view dispatcher
    view_dispatcher_free(app->view_dispatcher);

    // close the gui
    furi_record_close(RECORD_GUI);

    // free the app
    if (app)
        free(app);
}