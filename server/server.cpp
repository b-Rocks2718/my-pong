#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

struct PongGame {
	SOCKET p1;
	SOCKET p2;

	string p1_name;
	string p2_name;

	float p1_p, p2_p;
	float p1_dp, p2_dp;
	float p1_ddp, p2_ddp;

	float ball_p_x, ball_p_y;
	float ball_dp_x, ball_dp_y;

	int p1_score;
	int p2_score;
};

map<SOCKET, string> online_players;
map<SOCKET, string> match_queue;
vector<PongGame> running_games;
int num_players = 0;

float player_half_size_x = 2.5, player_half_size_y = 12;
float arena_half_size_x = 85, arena_half_size_y = 45;
float ball_half_size = 1;
float ball_speed = 130;

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

internal void
run_server(Input* input, SOCKET& listening, fd_set& master, float dt) {
	if (pressed(BUTTON_Q)) { running = false; }

	// select call is destuctive
	fd_set copy = master;

	// See who's talking to us
	timeval wait_time = { 0, 1 };
	int socketCount = select(0, &copy, nullptr, nullptr, &wait_time);

	// Loop through all the current connections / potential connect

	for (int i = 0; i < socketCount; i++)
	{
		// Makes things easy for us doing this assignment
		SOCKET sock = copy.fd_array[i];

		// Is it an inbound communication?
		if (sock == listening)
		{
			// Accept a new connection
			SOCKET client = accept(listening, nullptr, nullptr);

			// Add the new connection to the list of connected clients
			FD_SET(client, &master);

			char buf[300];
			ZeroMemory(buf, 300);

			// Receive client username
			int bytesIn = recv(client, buf, 300, 0);

			if (buf[0] == 1) {
				// receiving new username
				string name;
				int i = 1;
				while (buf[i] != 0 && i < 300) {
					name.push_back(buf[i]);
					++i;
				}
				num_players++;
				online_players[client] = name;
			}

			// Send list of online players
			string players_str;
			players_str.push_back(1);
			for (auto it = online_players.cbegin(); it != online_players.cend(); ++it) {
				for (size_t j = 0; j < (it->second).size(); ++j) {
					players_str.push_back((it->second)[j]);
				}
				players_str.push_back(0);
			}
			send(client, players_str.c_str(), players_str.size() + 1, 0);
		}
		else // It's an inbound message
		{
			char buf[300];
			ZeroMemory(buf, 300);

			// Receive message
			int bytesIn = recv(sock, buf, 300, 0);

			if (buf[0] == 2) {
				match_queue[sock] = online_players[sock];
			}
			else if (buf[0] == 3) {
				// Update game state with new inputs
				bool found = false;
				for (auto it = running_games.begin();
					it != running_games.end(); ++it) {
					if (it->p1 == sock) {
						it->p1_p = *((float*)(buf + 1));
						it->p1_dp = *((float*)(buf + 5));
						it->p1_ddp = *((float*)(buf + 9));
						found = true;
						break;
					}
					else if (it->p2 == sock) {
						it->p2_p = *((float*)(buf + 1));
						it->p2_dp = *((float*)(buf + 5));
						it->p2_ddp = *((float*)(buf + 9));
						found = true;
						break;
					}
				}
				if (!found) {
					char data[1];
					data[0] = 4;
					send(sock, data, 1, 0);
				}
			}
			else if (buf[0] == 4) {
				// Leave match queue
				match_queue.erase(sock);
			}

			if (bytesIn <= 0)
			{
				// Drop the client
				online_players.erase(sock);
				if (match_queue.find(sock) != match_queue.end()) {
					match_queue.erase(sock);
				}
				for (auto it = running_games.begin(); it != running_games.end(); ++it) {
					if (it->p1 == sock) {
						char data[1];
						data[0] = 4;
						send(it->p2, data, 1, 0);
						running_games.erase(it);
						break;
					}
					else if (it->p2 == sock) {
						char data[1];
						data[0] = 4;
						send(it->p1, data, 1, 0);
						running_games.erase(it);
						break;
					}
				}
				closesocket(sock);
				FD_CLR(sock, &master);
				num_players--;
			}
		}
	}
	if (frame * dt > 1) {
		frame = 0;
		for (int i = 0; i < master.fd_count; i++)
		{
			SOCKET outSock = master.fd_array[i];
			if (outSock != listening)
			{
				string players_str;
				players_str.push_back(1);
				for (auto it = online_players.cbegin(); it != online_players.cend(); ++it) {
					for (size_t j = 0; j < (it->second).size(); ++j) {
						players_str.push_back((it->second)[j]);
					}
					players_str.push_back(0);
				}
				send(outSock, players_str.c_str(), players_str.size() + 1, 0);
			}
		}
	}

	for (int i = 0; i < running_games.size(); ++i) {
		simulate_player(&running_games[i].p1_p, &running_games[i].p1_dp, running_games[i].p1_ddp, dt);
		simulate_player(&running_games[i].p2_p, &running_games[i].p2_dp, running_games[i].p2_ddp, dt);

		// Simulate ball
		{
			running_games[i].ball_p_x += running_games[i].ball_dp_x * dt;
			running_games[i].ball_p_y += running_games[i].ball_dp_y * dt;

			if (aabb_vs_aabb(running_games[i].ball_p_x, running_games[i].ball_p_y, 
				ball_half_size, ball_half_size,
				80, running_games[i].p1_p, player_half_size_x, player_half_size_y)) {
					running_games[i].ball_p_x = 80 - player_half_size_x - ball_half_size;
					running_games[i].ball_dp_x *= -1;
					running_games[i].ball_dp_y = running_games[i].p1_dp * 0.75 + (
						running_games[i].ball_p_y - running_games[i].p1_p) * 2;
			}
			else if (aabb_vs_aabb(running_games[i].ball_p_x, running_games[i].ball_p_y, 
				ball_half_size, ball_half_size,
				-80, running_games[i].p2_p, player_half_size_x, player_half_size_y)) {
				running_games[i].ball_p_x = -80 + player_half_size_x + ball_half_size;
				running_games[i].ball_dp_x *= -1;
				running_games[i].ball_dp_y = running_games[i].p2_dp * 0.75 + (
					running_games[i].ball_p_y - running_games[i].p2_p) * 2;
			}

			if (running_games[i].ball_p_y + ball_half_size > arena_half_size_y) {
				running_games[i].ball_p_y = arena_half_size_y - ball_half_size;
				running_games[i].ball_dp_y *= -1;
			}
			else if (running_games[i].ball_p_y - ball_half_size < -arena_half_size_y) {
				running_games[i].ball_p_y = -arena_half_size_y + ball_half_size;
				running_games[i].ball_dp_y *= -1;
			}

			if (running_games[i].ball_p_x + ball_half_size > arena_half_size_x) {
				running_games[i].ball_dp_x = -ball_speed;
				running_games[i].ball_dp_y = 0;
				running_games[i].ball_p_x = 0;
				running_games[i].ball_p_y = 0;
				running_games[i].p2_score++;
				if (running_games[i].p2_score > 10) {
					char data[2];
					data[0] = 4;
					data[1] = 1;
					send(running_games[i].p2, data, 2, 0);
					data[1] = 0;
					running_games.erase(running_games.begin() + i);
					break;
				}
			}
			else if (running_games[i].ball_p_x - ball_half_size < -arena_half_size_x) {
				running_games[i].ball_dp_x = ball_speed;
				running_games[i].ball_dp_y = 0;
				running_games[i].ball_p_x = 0;
				running_games[i].ball_p_y = 0;
				running_games[i].p1_score++;
				if (running_games[i].p1_score > 10) {
					char data[2];
					data[0] = 4;
					data[1] = 1;
					send(running_games[i].p1, data, 2, 0);
					data[1] = 0;
					running_games.erase(running_games.begin() + i);
					break;
				}
			}
		}
		if (frame * dt > 0.01) {
			// Send game state
			char data[32];
			char * data_ptr;

			data[0] = 3;

			data[2] = (char)running_games[i].p1_score;
			data[1] = (char)running_games[i].p2_score;

			// float values take 4 bytes
			float inverted_p = -running_games[i].ball_p_x;
			float inverted_dp = -running_games[i].ball_dp_x;

			data_ptr = (char*)&inverted_p;
			data[3] = *data_ptr;
			data[4] = *(data_ptr + 1);
			data[5] = *(data_ptr + 2);
			data[6] = *(data_ptr + 3);

			data_ptr = (char*)&inverted_dp;
			data[11] = *data_ptr;
			data[12] = *(data_ptr + 1);
			data[13] = *(data_ptr + 2);
			data[14] = *(data_ptr + 3);

			data_ptr = (char*)&running_games[i].ball_p_y;
			data[7] = *data_ptr;
			data[8] = *(data_ptr + 1);
			data[9] = *(data_ptr + 2);
			data[10] = *(data_ptr + 3);

			data_ptr = (char*)&running_games[i].ball_dp_y;
			data[15] = *data_ptr;
			data[16] = *(data_ptr + 1);
			data[17] = *(data_ptr + 2);
			data[18] = *(data_ptr + 3);

			data_ptr = (char*)&running_games[i].p2_p;
			data[19] = *data_ptr;
			data[20] = *(data_ptr + 1);
			data[21] = *(data_ptr + 2);
			data[22] = *(data_ptr + 3);

			data_ptr = (char*)&running_games[i].p2_dp;
			data[23] = *data_ptr;
			data[24] = *(data_ptr + 1);
			data[25] = *(data_ptr + 2);
			data[26] = *(data_ptr + 3);

			data_ptr = (char*)&running_games[i].p2_ddp;
			data[27] = *data_ptr;
			data[28] = *(data_ptr + 1);
			data[29] = *(data_ptr + 2);
			data[30] = *(data_ptr + 3);

			send(running_games[i].p1, data, 32, 0);

			data[2] = (char)running_games[i].p2_score;
			data[1] = (char)running_games[i].p1_score;

			data_ptr = (char*)&running_games[i].ball_p_x;
			data[3] = *data_ptr;
			data[4] = *(data_ptr + 1);
			data[5] = *(data_ptr + 2);
			data[6] = *(data_ptr + 3);

			data_ptr = (char*)&running_games[i].ball_dp_x;
			data[11] = *data_ptr;
			data[12] = *(data_ptr + 1);
			data[13] = *(data_ptr + 2);
			data[14] = *(data_ptr + 3);

			data_ptr = (char*)&running_games[i].p1_p;
			data[19] = *data_ptr;
			data[20] = *(data_ptr + 1);
			data[21] = *(data_ptr + 2);
			data[22] = *(data_ptr + 3);

			data_ptr = (char*)&running_games[i].p1_dp;
			data[23] = *data_ptr;
			data[24] = *(data_ptr + 1);
			data[25] = *(data_ptr + 2);
			data[26] = *(data_ptr + 3);

			data_ptr = (char*)&running_games[i].p1_ddp;
			data[27] = *data_ptr;
			data[28] = *(data_ptr + 1);
			data[29] = *(data_ptr + 2);
			data[30] = *(data_ptr + 3);

			send(running_games[i].p2, data, 32, 0);
		}
	}

	while (match_queue.size() > 1) {
		auto it = match_queue.begin();
		SOCKET p1 = it->first;
		string p1_name = it->second;
		match_queue.erase(p1);

		it = match_queue.begin();
		SOCKET p2 = it->first;
		string p2_name = it->second;
		match_queue.erase(p2);

		PongGame game;
		game.p1 = p1;
		game.p2 = p2;
		game.p1_name = p1_name;
		game.p2_name = p2_name;
		game.p1_score = 0;
		game.p2_score = 0;
		game.p1_p = 0;
		game.p2_p = 0;
		game.p1_dp = 0;
		game.p2_dp = 0;
		game.p1_ddp = 0;
		game.p2_ddp = 0;
		game.ball_dp_y = 0;
		game.ball_p_x = 0;
		game.ball_p_y = 0;
		game.ball_dp_x = ball_speed;

		string data;
		data.push_back(2);
		for (int i = 0; i < p2_name.size(); ++i) {
			data.push_back(p2_name[i]);
		}

		send(p1, data.c_str(), data.size() + 1, 0);

		data.clear();
		data.push_back(2);
		for (int i = 0; i < p1_name.size(); ++i) {
			data.push_back(p1_name[i]);
		}
		send(p2, data.c_str(), data.size() + 1, 0);

		running_games.push_back(game);
	}

	draw_rect(0, 0, 100, 100, 0x000000);
	draw_text("PONG SERVER", -92, 48, 0.5, 0xffffff);
	draw_text("ONLINE PLAYERS:", -92, 40, 0.5, 0xffffff);
	draw_number(online_players.size(), -45, 40, 0.5, 0xffffff);
	int i = 0;
	for (auto it = online_players.cbegin(); it != online_players.cend(); ++it) {
		draw_text((it->second).c_str(), -92, 34 - 6 * i, 0.5, 0xffffff);
		if (i > 10) { break; }
		++i;
	}
	draw_text("MATCH QUEUE:", -20, 40, 0.5, 0xffffff);
	draw_number(match_queue.size(), 20, 40, 0.5, 0xffffff);
	i = 0;
	for (auto it = match_queue.cbegin(); it != match_queue.cend(); ++it) {
		draw_text((it->second).c_str(), -20, 34 - 6 * i, 0.5, 0xffffff);
		if (i > 10) { break; }
		++i;
	}
}