#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

float player_1_p = 0, player_1_dp = 0, player_2_p = 0, player_2_dp = 0;
float arena_half_size_x = 85, arena_half_size_y = 45;
float player_half_size_x = 2.5, player_half_size_y = 12;
float ball_p_x = 0, ball_p_y = 0, ball_speed = 130, ball_dp_x = 0, ball_dp_y = 0, ball_half_size = 1;

int player_1_score = 0, player_2_score = 0;

string username;
string win_msg;
bool cursor_showing = false;
float cursor_location;
vector<string> online_players;

internal void
simulate_player(float *p, float *dp, float ddp, float dt) {
	ddp -= *dp * 10.f; // friction

	*p += *dp * dt + ddp * dt * dt * 0.5f;
	*dp += ddp * dt;

	if (*p + player_half_size_y > arena_half_size_y) {
		*p = arena_half_size_y - player_half_size_y;
		*dp = 0;
	}
	else if (*p - player_half_size_y < -arena_half_size_y) {
		*p = -arena_half_size_y + player_half_size_y;
		*dp = 0;
	}
}

internal bool
aabb_vs_aabb(float p1x, float p1y, float hs1x, float hs1y,
	float p2x, float p2y, float hs2x, float hs2y) {
	return (p1x + hs1x > p2x - hs2x &&
		p1x - hs1x < p2x + hs2x &&
		p1y + hs1y > p2y - hs2y &&
		p1y - hs1y < p2y + hs2y);
}

enum Gamemode {
	GM_HOME_MENU,
	GM_GAMEPLAY,
	GM_PAUSED,
	GM_ENTER_USERNAME,
	GM_ONLINE_MENU,
	GM_HELP_MSG,
	GM_DIFFICULTY_SELECT,
	GM_START_MSG,
	GM_WIN_MSG,
	GM_SERVER_ERROR,
	GM_FINDING_MATCH,
	GM_ONLINE_GAMEPLAY,
	GM_OPPONENT_DISCONNECTED,
};

enum Difficulty {
	EASY,
	MEDIUM,
	HARD,
	IMPOSSIBLE,
};

enum Enemy {
	AI_ENEMY,
	LOCAL_ENEMY,
	ONLINE_ENEMY,
};

Gamemode current_gamemode = GM_HOME_MENU;
int hot_button = 0;
Enemy enemy;
Difficulty difficulty;
float player_1_ddp = 0;
string enemy_username;

internal void
simulate_game(Input* input, float dt) {
	float player_2_ddp = 0;
	if (is_down(BUTTON_Q) && current_gamemode != GM_ENTER_USERNAME) {
		running = false;
	}
	draw_rect(0, 0, arena_half_size_x, arena_half_size_y, 0x555555);
	draw_arena_borders(arena_half_size_x, arena_half_size_y, 0x999999);

	if (current_gamemode == GM_ONLINE_MENU ||
		(current_gamemode == GM_ONLINE_GAMEPLAY) ||
		(current_gamemode == GM_WIN_MSG && enemy == ONLINE_ENEMY) ||
		current_gamemode == GM_FINDING_MATCH) {
		if (frame * dt > 0.01) {
			frame = 0;
			// Check for updates from server
			fd_set copy = sock_set;

			// See if the server sent anything
			timeval wait_time = { 0, 1 };
			int is_new_data = select(0, &copy, nullptr, nullptr, &wait_time);

			// Loop through all the current connections / potential connect
			if (is_new_data) {
				char buf[300];
				ZeroMemory(buf, 300);
				SOCKET server = sock_set.fd_array[0];
				// Receive message
				int bytesIn = recv(server, buf, 300, 0);

				if (bytesIn <= 0)
				{
					// Disconnected from server
					closesocket(server);
					FD_CLR(server, &sock_set);
					are_open_sockets = false;
					current_gamemode = GM_SERVER_ERROR;
				}
				else if (buf[0] == 1) {
					// receiving usernames list
					online_players.clear();
					bool last_was_null = false;
					bool current_is_null = false;
					int i = 1;
					string name_str;
					while (!(last_was_null && current_is_null) && i < 4096) {
						last_was_null = current_is_null;
						current_is_null = (buf[i] == 0);
						if (current_is_null) {
							online_players.push_back(name_str);
							name_str.clear();
						}
						else {
							name_str.push_back(buf[i]);
						}
						++i;
					}
				}
				else if (buf[0] == 2 && current_gamemode == GM_FINDING_MATCH) {
					// Match found
					current_gamemode = GM_ONLINE_GAMEPLAY;
					for (int i = 1; (buf[i] != 0 && i < 300); ++i) {
						enemy_username.push_back(buf[i]);
					}
				}
				else if (buf[0] == 3 && current_gamemode == GM_ONLINE_GAMEPLAY) {
					// Game state info
					// Score, ball location + velocity, and other player p, dp, ddp
					player_1_score = buf[2];
					player_2_score = buf[1];
					ball_p_x = *((float*)(buf + 3));
					ball_p_y = *((float*)(buf + 7));
					ball_dp_x = *((float*)(buf + 11));
					ball_dp_y = *((float*)(buf + 15));
					player_1_p = *((float*)(buf + 19));
					player_1_dp = *((float*)(buf + 23));
					player_1_ddp = *((float*)(buf + 27));
					if (player_1_score >= 10) {
						current_gamemode = GM_WIN_MSG;
						win_msg = "YOU WON!";
					}
					else if (player_2_score >= 10) {
						current_gamemode = GM_WIN_MSG;
						win_msg = "YOU LOST!";
					}
				}
				else if (buf[0] == 4 && current_gamemode == GM_ONLINE_GAMEPLAY) {
					current_gamemode = GM_OPPONENT_DISCONNECTED;
					frame = 0;
				}
			}

			if (current_gamemode == GM_ONLINE_GAMEPLAY) {
				char data[16];
				char * data_ptr;

				data[0] = 3;

				// float values take 4 bytes
				data_ptr = (char*)&player_2_p;
				data[1] = *data_ptr;
				data[2] = *(data_ptr + 1);
				data[3] = *(data_ptr + 2);
				data[4] = *(data_ptr + 3);

				data_ptr = (char*)&player_2_dp;
				data[5] = *data_ptr;
				data[6] = *(data_ptr + 1);
				data[7] = *(data_ptr + 2);
				data[8] = *(data_ptr + 3);

				data_ptr = (char*)&player_2_ddp;
				data[9] = *data_ptr;
				data[10] = *(data_ptr + 1);
				data[11] = *(data_ptr + 2);
				data[12] = *(data_ptr + 3);

				send(sock_set.fd_array[0], data, 16, 0);
			}
		}
	}

	if (current_gamemode == GM_GAMEPLAY) {

		if (pressed(BUTTON_P)) {
			hot_button = 0;
			current_gamemode = GM_PAUSED;
		}

		if (enemy == LOCAL_ENEMY) {
			player_1_ddp = 0.f;
			if (is_down(BUTTON_I)) player_1_ddp += 2000;
			if (is_down(BUTTON_K)) player_1_ddp -= 2000;
		} 
		else if (enemy == AI_ENEMY){
			if (difficulty == EASY) {
				if (frame * dt > 0.8) {
					if (ball_p_y > player_1_p) player_1_ddp = 1000;
					if (ball_p_y < player_1_p) player_1_ddp = -1000;
					frame = 0;
				}
			}
			else if (difficulty == MEDIUM) {
				if (frame * dt > 0.5) {
					player_1_ddp = 100 * (ball_p_y - player_1_p);
					if (player_1_ddp > 1300) player_1_ddp = 1300;
					if (player_1_ddp < -1300) player_1_ddp = -1300;
					frame = 0;
				}
			}
			else if (difficulty == HARD) {
				player_1_ddp = 100 * (ball_p_y - player_1_p);
				if (player_1_ddp > 1300) player_1_ddp = 1300;
				if (player_1_ddp < -1300) player_1_ddp = -1300;
			}
			else if (difficulty == IMPOSSIBLE) {
				player_1_p = ball_p_y;
			}
		}

		if (is_down(BUTTON_W)) {
			player_2_ddp += 2000;
		}
		if (is_down(BUTTON_S)) {
			player_2_ddp -= 2000;
		}

		simulate_player(&player_1_p, &player_1_dp, player_1_ddp, dt);
		simulate_player(&player_2_p, &player_2_dp, player_2_ddp, dt);

		// Simulate ball
		{
			ball_p_x += ball_dp_x * dt;
			ball_p_y += ball_dp_y * dt;

			if (aabb_vs_aabb(ball_p_x, ball_p_y, ball_half_size, ball_half_size,
				80, player_1_p, player_half_size_x, player_half_size_y)) {
				ball_p_x = 80 - player_half_size_x - ball_half_size;
				ball_dp_x *= -1;
				ball_dp_y = player_1_dp * 0.75 + (ball_p_y - player_1_p) * 2;
			}
			else if (aabb_vs_aabb(ball_p_x, ball_p_y, ball_half_size, ball_half_size,
				-80, player_2_p, player_half_size_x, player_half_size_y)) {
				ball_p_x = -80 + player_half_size_x + ball_half_size;
				ball_dp_x *= -1;
				ball_dp_y = player_2_dp * 0.75 + (ball_p_y - player_2_p) * 2;
			}

			if (ball_p_y + ball_half_size > arena_half_size_y) {
				ball_p_y = arena_half_size_y - ball_half_size;
				ball_dp_y *= -1;
			}
			else if (ball_p_y - ball_half_size < -arena_half_size_y) {
				ball_p_y = -arena_half_size_y + ball_half_size;
				ball_dp_y *= -1;
			}

			if (ball_p_x + ball_half_size > arena_half_size_x) {
				ball_dp_x = -ball_speed;
				ball_dp_y = 0;
				ball_p_x = 0;
				ball_p_y = 0;
				if (++player_1_score >= 10) {
					current_gamemode = GM_WIN_MSG;
					if (enemy == LOCAL_ENEMY) {
						win_msg = "PLAYER 1 WINS!";
					}
					else {
						win_msg = "YOU WON!";
					}
				}
			}
			else if (ball_p_x - ball_half_size < -arena_half_size_x) {
				ball_dp_x = ball_speed;
				ball_dp_y = 0;
				ball_p_x = 0;
				ball_p_y = 0;
				if (++player_2_score >= 10) {
					current_gamemode = GM_WIN_MSG;
					if (enemy == LOCAL_ENEMY) {
						win_msg = "PLAYER 2 WINS!";
					}
					else {
						win_msg = "YOU LOST!";
					}
				}
			}
		}

		draw_number(player_1_score, -10, 40, 1.f, 0xbbffbb);
		draw_number(player_2_score, 10, 40, 1.f, 0xbbffbb);

		// rendering
		draw_rect(ball_p_x, ball_p_y, ball_half_size, ball_half_size, 0xffffff);

		draw_rect(80, player_1_p, player_half_size_x, player_half_size_y, 0xff0000);
		draw_rect(-80, player_2_p, player_half_size_x, player_half_size_y, 0xff0000);

		if (enemy == AI_ENEMY) {
			draw_text("YOU", -82, 42, 0.5, 0xffffff);
			if (difficulty == EASY) {
				draw_text("EASY AI", 62, 42, 0.5, 0xffffff);
			}
			else if (difficulty == MEDIUM) {
				draw_text("MEDIUM AI", 58, 42, 0.5, 0xffffff);
			}
			else if (difficulty == HARD) {
				draw_text("HARD AI", 62, 42, 0.5, 0xffffff);
			}
			else if (difficulty == IMPOSSIBLE) {
				draw_text("IMPOSSIBLE AI", 46, 42, 0.5, 0xffffff);
			}
		}
		else if (enemy == LOCAL_ENEMY) {
			draw_text("PLAYER 1", -82, 42, 0.5, 0xffffff);
			draw_text("PLAYER 2", 56, 42, 0.5, 0xffffff);
		}
	}
	else if (current_gamemode == GM_HOME_MENU){

		if (pressed(BUTTON_W) || pressed(BUTTON_UP)) {
			hot_button += 3;
			hot_button %= 4;
		} 
		else if (pressed(BUTTON_S) || pressed(BUTTON_DOWN)) {
			hot_button++;
			hot_button %= 4;
		}
		else if (pressed(BUTTON_ENTER)) {
			if (hot_button == 0) {
				current_gamemode = GM_DIFFICULTY_SELECT;
				enemy = AI_ENEMY;
			}
			else if (hot_button == 1) {
				current_gamemode = GM_START_MSG;
				ball_dp_x = ball_speed;
				enemy = LOCAL_ENEMY;
			}
			else if (hot_button == 2) {
				cursor_location = -55;
				if (username.size() == 0) {
					current_gamemode = GM_ENTER_USERNAME;
				}
				else {
					if (!are_open_sockets) {
						SOCKET server = connect_to_server();
						if (server == INVALID_SOCKET) {
							current_gamemode = GM_SERVER_ERROR;
						}
						else {
							FD_SET(server, &sock_set);
							char buf[4096];
							string output_str;
							output_str.push_back(1);
							for (size_t i = 0; i < username.size(); ++i) {
								output_str.push_back(username[i]);
							}

							int sendResult = send(server, output_str.c_str(), output_str.size() + 1, 0);
							if (sendResult != SOCKET_ERROR) {
								// Wait for response
								ZeroMemory(buf, 4096);
								int bytesReceived = recv(server, buf, 4096, 0);
								if (buf[0] == 1) {
									// receiving usernames list
									bool last_was_null = false;
									bool current_is_null = false;
									int i = 1;
									string name_str;
									while (!(last_was_null && current_is_null) && i < 4096) {
										last_was_null = current_is_null;
										current_is_null = (buf[i] == 0);
										if (current_is_null) {
											online_players.push_back(name_str);
											name_str.clear();
										}
										else {
											name_str.push_back(buf[i]);
										}
										++i;
									}
								}
							}
							hot_button = 0;
							current_gamemode = GM_ONLINE_MENU;
						}
					}
					else {
						hot_button = 0;
						current_gamemode = GM_ONLINE_MENU;
					}
				}
				enemy = ONLINE_ENEMY;
			}
			else if (hot_button == 3) {
				current_gamemode = GM_HELP_MSG;
			}
		}

		int colors[4];
		for (int i = 0; i < 4; ++i) {
			colors[i] = 0x00ccff;
		}
		colors[hot_button] = 0x22ff22;
		draw_text("PONG", -25, 40, 2.3, 0xff0000);
		draw_rect(5, -10, 45, 25, 0x887766);
		draw_text("SINGLE PLAYER", -35, 10, 0.75, colors[0]);
		draw_text("LOCAL MULTIPLAYER", -35, -2, 0.75, colors[1]);
		draw_text("ONLINE MULTIPLAYER", -35, -14, 0.75, colors[2]);
		draw_text("HELP", -35, -26, 0.75, colors[3]);
	}
	else if (current_gamemode == GM_HELP_MSG) {

		if (pressed(BUTTON_ENTER)){
			current_gamemode = GM_HOME_MENU;
			hot_button = 0;
		}

		draw_text("GAME INFO", -80, 40, 1.7, 0xff0000);
		draw_text("PRESS Q AT ANY TIME TO QUIT", -80, 25, 0.5, 0xffffff);
		draw_text("USE W TO MOVE UP, AND S TO MOVE DOWN", -80, 20, 0.5, 0xffffff);
		draw_text("IN LOCAL MULTIPLAYER, PLAYER 2 USES I AND K TO MOVE", -80, 15, 0.5, 0xffffff);
		draw_text("PRESS P TO PAUSE A GAME", -80, 10, 0.5, 0xffffff);
		draw_text("PRESS ENTER TO RETURN TO THE MAIN MENU", -80, 5, 0.5, 0xffffff);
	}
	else if (current_gamemode == GM_START_MSG) {
		if (pressed(BUTTON_ENTER)) {
			current_gamemode = GM_GAMEPLAY;
		}

		draw_text("FIRST TO 10 POINTS WINS!", -70, 30, 1, 0xff0000);
		draw_text("PRESS ENTER TO START", -35, 0, 0.6, 0xffffff);
	}
	else if (current_gamemode == GM_WIN_MSG) {
		if (pressed(BUTTON_ENTER)) {
			player_1_p = 0, player_1_dp = 0, player_2_p = 0, player_2_dp = 0;
			player_1_ddp = 0.f;
			arena_half_size_x = 85, arena_half_size_y = 45;
			player_half_size_x = 2.5, player_half_size_y = 12;
			ball_p_x = 0, ball_p_y = 0, ball_speed = 130, ball_dp_x = ball_speed, ball_dp_y = 0, ball_half_size = 1;
			player_1_score = 0, player_2_score = 0;

			if (enemy != ONLINE_ENEMY) {
				current_gamemode = GM_HOME_MENU;
			}
			else {
				current_gamemode = GM_ONLINE_MENU;
			}
			hot_button = 0;
		}

		draw_text(win_msg.c_str(), -25, 30, 1, 0xff0000);
		draw_text("PRESS ENTER TO RETURN TO THE HOME MENU", -67, 0, 0.6, 0xffffff);
	}
	else if (current_gamemode == GM_PAUSED) {
		if (pressed(BUTTON_W) || pressed(BUTTON_UP)) {
			hot_button += 1;
			hot_button %= 2;
		}
		else if (pressed(BUTTON_S) || pressed(BUTTON_DOWN)) {
			hot_button++;
			hot_button %= 2;
		}
		else if (pressed(BUTTON_ENTER)) {
			if (hot_button == 0) {
				current_gamemode = GM_GAMEPLAY;
			}
			else if (hot_button == 1) {
				player_1_p = 0, player_1_dp = 0, player_2_p = 0, player_2_dp = 0;
				player_1_ddp = 0.f;
				arena_half_size_x = 85, arena_half_size_y = 45;
				player_half_size_x = 2.5, player_half_size_y = 12;
				ball_p_x = 0, ball_p_y = 0, ball_speed = 130, ball_dp_x = ball_speed, ball_dp_y = 0, ball_half_size = 1;
				player_1_score = 0, player_2_score = 0;

				current_gamemode = GM_HOME_MENU;
				hot_button = 0;
			}
		}

		int colors[2];
		for (int i = 0; i < 2; ++i) {
			colors[i] = 0x00ccff;
		}
		colors[hot_button] = 0x22ff22;
		draw_text("GAME PAUSED", -50, 40, 1.8, 0xff0000);
		draw_rect(5, 0, 45, 15, 0x887766);
		draw_text("RESUME", -35, 10, 0.75, colors[0]);
		draw_text("RETURN TO MENU", -35, -2, 0.75, colors[1]);
	}
	else if (current_gamemode == GM_DIFFICULTY_SELECT) {
		if (pressed(BUTTON_W) || pressed(BUTTON_UP)) {
			hot_button += 3;
			hot_button %= 4;
		}
		else if (pressed(BUTTON_S) || pressed(BUTTON_DOWN)) {
			hot_button++;
			hot_button %= 4;
		}
		else if (pressed(BUTTON_ENTER)) {
			if (hot_button == 0) {
				difficulty = EASY;
			}
			else if (hot_button == 1) {
				difficulty = MEDIUM;
			}
			else if (hot_button == 2) {
				difficulty = HARD;
			}
			else if (hot_button == 3) {
				difficulty = IMPOSSIBLE;
			}
			ball_dp_x = ball_speed;
			current_gamemode = GM_START_MSG;
		}

		int colors[4];
		for (int i = 0; i < 4; ++i) {
			colors[i] = 0x00ccff;
		}
		colors[hot_button] = 0x22ff22;
		draw_text("SELECT DIFFICULTY", -75, 40, 1.5, 0xff0000);
		draw_rect(5, -10, 45, 25, 0x887766);
		draw_text("EASY", -35, 10, 0.75, colors[0]);
		draw_text("MEDIUM", -35, -2, 0.75, colors[1]);
		draw_text("HARD", -35, -14, 0.75, colors[2]);
		draw_text("IMPOSSIBLE", -35, -26, 0.75, colors[3]);
	}
	else if (current_gamemode == GM_ENTER_USERNAME) {
		if (pressed(BUTTON_ENTER) && username.size() > 0) {
			SOCKET server = connect_to_server();
			hot_button = 0;
			if (server == INVALID_SOCKET) {
				current_gamemode = GM_SERVER_ERROR;
			}
			else {
				FD_SET(server, &sock_set);
				char buf[300];
				string output_str;
				output_str.push_back(1);
				for (size_t i = 0; i < username.size(); ++i) {
					output_str.push_back(username[i]);
				}

				int sendResult = send(server, output_str.c_str(), output_str.size() + 1, 0);
				if (sendResult != SOCKET_ERROR) {
					// Wait for response
					ZeroMemory(buf, 300);
					int bytesReceived = recv(server, buf, 300, 0);
					if (buf[0] == 1) {
						// receiving usernames list
						bool last_was_null = false;
						bool current_is_null = false;
						int i = 1;
						string name_str;
						while (!(last_was_null && current_is_null) && i < 4096) {
							last_was_null = current_is_null;
							current_is_null = (buf[i] == 0);
							if (current_is_null) {
								online_players.push_back(name_str);
								name_str.clear();
							}
							else {
								name_str.push_back(buf[i]);
							}
							++i;
						}
					}
				}
				current_gamemode = GM_ONLINE_MENU;
			}
		}

		if (username.size() < 19) {
			for (int i = 0; i < 26; ++i) {
				if (pressed((Button)i)) {
					username.push_back(i + 'A');
					cursor_location += 6;
				}
			}
			for (int i = 0; i < 10; ++i) {
				if (pressed((Button)(i + 26))) {
					username.push_back('0' + i);
					cursor_location += 6;
				}
			}
			if (pressed(BUTTON_SPACE)) {
				username.push_back(' ');
				cursor_location += 6;
			}
			if (pressed(BUTTON_DASH)) {
				if (is_down(BUTTON_SHIFT)) {
					username.push_back('_');
				}
				else {
					username.push_back('-');
				}
				cursor_location += 6;
			}
		}

		if (pressed(BUTTON_BACKSPACE) && username.size() > 0) {
			username.pop_back();
			cursor_location -= 6;
		}

		if (frame * dt > 0.7) {
			cursor_showing = !cursor_showing;
			frame = 0;
		}

		draw_text("ENTER YOUR USERNAME", -75, 40, 1.4, 0xff0000);
		draw_rect(0, 0, 60, 7, 0x887766);
		if (cursor_showing) {
			draw_rect(cursor_location, 0, 0.4, 5, 0xffffff);
		}
		draw_text(username.c_str(), -55, 3, 1, 0x00ccff);
	}
	else if (current_gamemode == GM_SERVER_ERROR) {
		if (pressed(BUTTON_ENTER)) {
			current_gamemode = GM_HOME_MENU;
			hot_button = 0;
		}

		draw_text("UNABLE TO CONNECT TO SERVER", -80, 40, 1, 0xff0000);
		draw_text("THIS PROBABLY MEANS YOU ARE NOT CONNECTED", -80, 25, 0.5, 0xffffff);
		draw_text("TO THE INTERNET, OR BROOKS IS NOT CURRENTLY", -80, 20, 0.5, 0xffffff);
		draw_text("RUNNING THE SERVER.", -80, 15, 0.5, 0xffffff);
		draw_text("PRESS ENTER TO RETURN TO THE MAIN MENU", -80, 10, 0.5, 0xffffff);
	}
	else if (current_gamemode == GM_ONLINE_MENU) {
		if (pressed(BUTTON_W) || pressed(BUTTON_UP)) {
			hot_button += 1;
			hot_button %= 2;
		}
		else if (pressed(BUTTON_S) || pressed(BUTTON_DOWN)) {
			hot_button++;
			hot_button %= 2;
		}
		else if (pressed(BUTTON_ENTER)) {
			if (hot_button == 0) {
				// Tell server to enter match queue

				char data[1] = { 2 };
				int sendResult = send(sock_set.fd_array[0], data, 1, 0);
				ball_dp_x = 0;
				enemy_username.clear();
				current_gamemode = GM_FINDING_MATCH;
			}
			else if (hot_button == 1) {
				player_1_p = 0, player_1_dp = 0, player_2_p = 0, player_2_dp = 0;
				player_1_ddp = 0.f;
				arena_half_size_x = 85, arena_half_size_y = 45;
				player_half_size_x = 2.5, player_half_size_y = 12;
				ball_p_x = 0, ball_p_y = 0, ball_speed = 130, ball_dp_x = ball_speed, ball_dp_y = 0, ball_half_size = 1;
				player_1_score = 0, player_2_score = 0;

				current_gamemode = GM_HOME_MENU;
				hot_button = 0;
			}
		}

		int colors[2];
		for (int i = 0; i < 2; ++i) {
			colors[i] = 0x00ccff;
		}
		colors[hot_button] = 0x22ff22;
		draw_text("PONG SERVER", -50, 40, 1.8, 0xff0000);
		draw_rect(-43, -15, 36, 15, 0x887766);
		draw_text("FIND GAME", -75, -5, 0.75, colors[0]);
		draw_text("RETURN TO MENU", -75, -17, 0.75, colors[1]);

		draw_rect(40, -10, 36, 34, 0x887766);
		draw_text("ONLINE PLAYERS", 6, 22, 0.7, 0xffcc00);
		
		// Show who else is online
		int y = 13;
		for (size_t i = 0; i < online_players.size(); ++i) {
			draw_text(online_players[i].c_str(), 6, y, 0.7, 0xffcc00);
			y -= 6;
			if (i >= 9) { break; }
		}
	}
	else if (current_gamemode == GM_FINDING_MATCH) {
		draw_text("FINDING MATCH", -70, 40, 1.8, 0xff0000);
		draw_text("CANCEL", -20, 0, 1, 0x22ff22);
		if (pressed(BUTTON_ENTER)) {
			char data[1];
			data[0] = 4;
			send(sock_set.fd_array[0], data, 1, 0);
			current_gamemode = GM_ONLINE_MENU;
		}
	}
	else if (current_gamemode == GM_ONLINE_GAMEPLAY) {

		if (is_down(BUTTON_W)) {
			player_2_ddp = 2000;
		}
		if (is_down(BUTTON_S)) {
			player_2_ddp = -2000;
		}

		simulate_player(&player_1_p, &player_1_dp, player_1_ddp, dt);
		simulate_player(&player_2_p, &player_2_dp, player_2_ddp, dt);

		// Simulate ball
		{
			ball_p_x += ball_dp_x * dt;
			ball_p_y += ball_dp_y * dt;

			if (aabb_vs_aabb(ball_p_x, ball_p_y, ball_half_size, ball_half_size,
				80, player_1_p, player_half_size_x, player_half_size_y)) {
				ball_p_x = 80 - player_half_size_x - ball_half_size;
				ball_dp_x *= -1;
				ball_dp_y = player_1_dp * 0.75 + (ball_p_y - player_1_p) * 2;
			}
			else if (aabb_vs_aabb(ball_p_x, ball_p_y, ball_half_size, ball_half_size,
				-80, player_2_p, player_half_size_x, player_half_size_y)) {
				ball_p_x = -80 + player_half_size_x + ball_half_size;
				ball_dp_x *= -1;
				ball_dp_y = player_2_dp * 0.75 + (ball_p_y - player_2_p) * 2;
			}

			if (ball_p_y + ball_half_size > arena_half_size_y) {
				ball_p_y = arena_half_size_y - ball_half_size;
				ball_dp_y *= -1;
			}
			else if (ball_p_y - ball_half_size < -arena_half_size_y) {
				ball_p_y = -arena_half_size_y + ball_half_size;
				ball_dp_y *= -1;
			}

			if (ball_p_x + ball_half_size > arena_half_size_x) {
				ball_dp_x = 0;
				ball_dp_y = 0;
				ball_p_x = 0;
				ball_p_y = 0;
			}
			else if (ball_p_x - ball_half_size < -arena_half_size_x) {
				ball_dp_x = 0;
				ball_dp_y = 0;
				ball_p_x = 0;
				ball_p_y = 0;
			}
		}

		draw_number(player_1_score, -10, 40, 1.f, 0xbbffbb);
		draw_number(player_2_score, 10, 40, 1.f, 0xbbffbb);

		// rendering
		draw_rect(ball_p_x, ball_p_y, ball_half_size, ball_half_size, 0xffffff);

		draw_rect(80, player_1_p, player_half_size_x, player_half_size_y, 0xff0000);
		draw_rect(-80, player_2_p, player_half_size_x, player_half_size_y, 0xff0000);

		draw_text(username.c_str(), -82, 42, 0.5, 0xffffff);
		draw_text(enemy_username.c_str(), 82 - 6 * 0.5 * enemy_username.size(), 42, 0.5, 0xffffff);
	}
	else if (current_gamemode == GM_OPPONENT_DISCONNECTED) {
		if (frame * dt > 4) {
			current_gamemode = GM_ONLINE_MENU;
			hot_button = 0;
		}
		draw_text("OPPONENT DISCONNECTED", -10.5 * 6 * 1.2, 4, 1.2, 0xff0000);
	}
}