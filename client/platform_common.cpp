struct ButtonState {
	bool is_down;
	bool changed;
};

enum Button {
	BUTTON_A,
	BUTTON_B,
	BUTTON_C,
	BUTTON_D,
	BUTTON_E,
	BUTTON_F,
	BUTTON_G,
	BUTTON_H,
	BUTTON_I,
	BUTTON_J,
	BUTTON_K,
	BUTTON_L,
	BUTTON_M,
	BUTTON_N,
	BUTTON_O,
	BUTTON_P,
	BUTTON_Q,
	BUTTON_R,
	BUTTON_S,
	BUTTON_T,
	BUTTON_U,
	BUTTON_V,
	BUTTON_W,
	BUTTON_X,
	BUTTON_Y,
	BUTTON_Z,

	BUTTON_0,
	BUTTON_1,
	BUTTON_2,
	BUTTON_3,
	BUTTON_4,
	BUTTON_5,
	BUTTON_6,
	BUTTON_7,
	BUTTON_8,
	BUTTON_9,

	BUTTON_BACKSPACE,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_ENTER,
	BUTTON_SPACE,
	BUTTON_SHIFT,
	BUTTON_DASH,

	BUTTON_COUNT, // needs to be the last item
};

struct Input {
	ButtonState buttons[BUTTON_COUNT];
};