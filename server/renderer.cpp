internal void
clear_screen(u32 color) {
	u32* pixel = (u32*)render_state.memory;
	for (int y = 0; y < render_state.height; ++y) {
		for (int x = 0; x < render_state.width; ++x) {
			*pixel++ = color;
		}
	}
}

internal void
draw_rect_in_pixels(int x0, int y0, int x1, int y1, u32 color) {

	x0 = clamp(0, x0, render_state.width);
	y0 = clamp(0, y0, render_state.height);
	x1 = clamp(0, x1, render_state.width);
	y1 = clamp(0, y1, render_state.height);


	for (int y = y0; y < y1; ++y) {
		u32* pixel = (u32*)render_state.memory + x0 + y * render_state.width;
		for (int x = x0; x < x1; ++x) {
			*pixel++ = color;
		}
	}
}

global_variable float render_scale = 0.01f;

internal void
draw_arena_borders(float arena_x, float arena_y, u32 color) {
	arena_x *= render_state.height * render_scale;
	arena_y *= render_state.height * render_scale;

	int x0 = (int)((float)render_state.width * .5f - arena_x);
	int x1 = (int)((float)render_state.width * .5f + arena_x);
	int y0 = (int)((float)render_state.height * .5f - arena_y);
	int y1 = (int)((float)render_state.height * .5f + arena_y);

	draw_rect_in_pixels(0, 0, render_state.width, y0, color);
	draw_rect_in_pixels(0, y1, x1, render_state.height, color);
	draw_rect_in_pixels(0, y0, x0, y1, color);
	draw_rect_in_pixels(x1, y0, render_state.width, render_state.height, color);
}

internal void
draw_rect(float x, float y, float half_size_x, float half_size_y, u32 color) {
	x *= render_state.height * render_scale;
	y *= render_state.height * render_scale;
	half_size_x *= render_state.height * render_scale;
	half_size_y *= render_state.height * render_scale;

	x += render_state.width / 2.f;
	y += render_state.height / 2.f;

	int x0 = x - half_size_x;
	int y0 = y - half_size_y;
	int x1 = x + half_size_x;
	int y1 = y + half_size_y;

	draw_rect_in_pixels(x0, y0, x1, y1, color);
}

const char* letters[][7] = {
	"  0",
	" 0",
	"0",
	"0",
	"0",
	" 0",
	"  0",

	"0",
	" 0",
	"  0",
	"  0",
	"  0",
	" 0",
	"0",

	"  0",
	"00000",
	" 0 0",
	"",
	"",
	"",
	"",

	"",
	"  0",
	"  0",
	"00000",
	"  0",
	"  0",
	"",

	"",
	"",
	"",
	"",
	"",
	" 0",
	"00",

	"",
	"",
	"",
	"000",
	"",
	"",
	"",

	"",
	"",
	"",
	"",
	"",
	"",
	"0",

	"   0",
	"  0",
	"  0",
	" 0",
	" 0",
	"0",
	"0",

	"0000",
	"0  0",
	"0  0",
	"0  0",
	"0  0",
	"0  0",
	"0000",

	" 0",
	"00",
	" 0",
	" 0",
	" 0",
	" 0",
	"000",

	"0000",
	"   0",
	"   0",
	"0000",
	"0",
	"0",
	"0000",

	"0000",
	"   0",
	"   0",
	"0000",
	"   0",
	"   0",
	"0000",

	"0  0",
	"0  0",
	"0  0",
	"0000",
	"   0",
	"   0",
	"   0",

	"0000",
	"0",
	"0",
	"0000",
	"   0",
	"   0",
	"0000",

	"0000",
	"0",
	"0",
	"0000",
	"0  0",
	"0  0",
	"0000",

	"0000",
	"   0",
	"   0",
	"  0",
	"  0",
	" 0",
	" 0",

	"0000",
	"0  0",
	"0  0",
	"0000",
	"0  0",
	"0  0",
	"0000",

	"0000",
	"0  0",
	"0  0",
	"0000",
	"   0",
	"   0",
	"0000",

	"",
	"",
	"0",
	"",
	"0",
	"",
	"",

	"",
	"",
	" 0",
	"",
	" 0",
	"00",
	"",

	"",
	"  0",
	" 0",
	"0",
	" 0",
	"  0",
	"",

	"",
	"",
	"0000",
	"",
	"0000",
	"",
	"",

	"",
	"0",
	" 0",
	"  0",
	" 0",
	"0",
	"",

	" 00",
	"0  0",
	"   0",
	"  0",
	"  0",
	"",
	"  0",

	"",
	" 00",
	"0 00",
	"00 0",
	"0000",
	" 00",
	"",

	" 00",
	"0  0",
	"0  0",
	"0000",
	"0  0",
	"0  0",
	"0  0",

	"000",
	"0  0",
	"0  0",
	"000",
	"0  0",
	"0  0",
	"000",

	" 000",
	"0",
	"0",
	"0",
	"0",
	"0",
	" 000",

	"000",
	"0  0",
	"0  0",
	"0  0",
	"0  0",
	"0  0",
	"000",

	"0000",
	"0",
	"0",
	"000",
	"0",
	"0",
	"0000",

	"0000",
	"0",
	"0",
	"000",
	"0",
	"0",
	"0",

	" 000",
	"0",
	"0",
	"0 00",
	"0  0",
	"0  0",
	" 000",

	"0  0",
	"0  0",
	"0  0",
	"0000",
	"0  0",
	"0  0",
	"0  0",

	"000",
	" 0",
	" 0",
	" 0",
	" 0",
	" 0",
	"000",

	" 000",
	"   0",
	"   0",
	"   0",
	"0  0",
	"0  0",
	" 000",

	"0  0",
	"0  0",
	"0 0",
	"00",
	"0 0",
	"0  0",
	"0  0",

	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0000",

	"00 00",
	"0 0 0",
	"0 0 0",
	"0   0",
	"0   0",
	"0   0",
	"0   0",

	"00  0",
	"0 0 0",
	"0 0 0",
	"0 0 0",
	"0 0 0",
	"0 0 0",
	"0  00",

	"0000",
	"0  0",
	"0  0",
	"0  0",
	"0  0",
	"0  0",
	"0000",

	" 000",
	"0  0",
	"0  0",
	"000",
	"0",
	"0",
	"0",

	" 000 ",
	"0   0",
	"0   0",
	"0   0",
	"0 0 0",
	"0  0 ",
	" 00 0",

	"000",
	"0  0",
	"0  0",
	"000",
	"0  0",
	"0  0",
	"0  0",

	" 000",
	"0",
	"0 ",
	" 00",
	"   0",
	"   0",
	"000 ",

	"000",
	" 0",
	" 0",
	" 0",
	" 0",
	" 0",
	" 0",

	"0  0",
	"0  0",
	"0  0",
	"0  0",
	"0  0",
	"0  0",
	" 00",

	"0   0",
	"0   0",
	"0   0",
	"0   0",
	"0   0",
	" 0 0",
	"  0",

	"0   0 ",
	"0   0",
	"0   0",
	"0 0 0",
	"0 0 0",
	"0 0 0",
	" 0 0 ",

	"0   0",
	"0   0",
	" 0 0",
	"  0",
	" 0 0",
	"0   0",
	"0   0",

	"0   0",
	"0   0",
	" 0 0",
	"  0",
	"  0",
	"  0",
	"  0",

	"0000",
	"   0",
	"  0",
	" 0",
	"0",
	"0",
	"0000",

	"0",
	"0",
	"0",
	"0",
	"0",
	"",
	"0",

	"",
	"",
	"",
	"",
	"",
	"",
	"0000",
};

internal void
draw_text(const char *text, float x, float y, float size, u32 color) {
	float half_size = size * .5f;
	float original_y = y;

	while (*text) {
		const char** letter = nullptr;
		if (*text <= 'Z' && *text >= '(') {
			letter = letters[*text - '('];
		}
		else if (*text == '!') {
			letter = letters['Z' - '(' + 1];
		}
		else if (*text == '_') {
			letter = letters['Z' - '(' + 2];
		}
		if (letter != nullptr) {
			float original_x = x;

			for (int i = 0; i < 7; i++) {
				const char* row = letter[i];
				while (*row) {
					if (*row == '0') {
						draw_rect(x, y, half_size, half_size, color);
					}
					x += size;
					row++;
				}
				y -= size;
				x = original_x;
			}
		}
		text++;
		x += size * 6.f;
		y = original_y;
	}
}

internal void
draw_number(int number, float x, float y, float size, u32 color) {
	if (number >= 0) {
		char digit_str[11] = { 0 };
		int i = 9;
		do {
			int digit = number % 10;
			number /= 10;
			digit_str[i] = digit + '0';
			i--;
		} while (number && i >= 0);
		draw_text(digit_str + i + 1, x, y, size, color);
	}
}