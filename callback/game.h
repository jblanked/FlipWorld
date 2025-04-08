#pragma once
#include <flip_world.h>
extern bool user_hit_back;
extern uint32_t lobby_index;
extern char *lobby_list[10];
extern FuriThread *game_thread;
extern FuriThread *waiting_thread;
extern bool game_thread_running;
extern bool waiting_thread_running;
//
void game_run(FlipWorldApp *app);
bool game_fetch_lobby(FlipperHTTP *fhttp, char *lobby_name);
bool game_join_lobby(FlipperHTTP *fhttp, char *lobby_name);
size_t game_lobby_count(FlipperHTTP *fhttp, FuriString *lobby);
bool game_in_lobby(FlipperHTTP *fhttp, FuriString *lobby);
void game_start_pvp(FlipperHTTP *fhttp, FuriString *lobby, void *context);
void game_waiting_lobby(void *context);
void game_waiting_process(FlipperHTTP *fhttp, void *context);