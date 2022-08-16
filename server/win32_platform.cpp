#include <string>
#include <vector>
#include <map>
#include <ws2tcpip.h>
#include <windows.h>
#include "utils.cpp"

#pragma comment(lib, "ws2_32.lib")

using std::string;
using std::vector;
using std::map;

global_variable bool running = true;

struct RenderState {
	int height, width;
	void* memory;

	BITMAPINFO bitmap_info;
};

global_variable RenderState render_state;
global_variable int frame = 0;
global_variable int frame2 = 0;

#include "platform_common.cpp"
#include "renderer.cpp"
#include "server.cpp"

LRESULT CALLBACK window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LRESULT result = 0;
	switch (uMsg) {
	case WM_CLOSE:
	case WM_DESTROY: {
		running = false;
	} break;

	case WM_SIZE: {
		RECT rect;
		GetClientRect(hwnd, &rect);
		render_state.width = rect.right - rect.left;
		render_state.height = rect.bottom - rect.top;

		int buffer_size = render_state.width * render_state.height * sizeof(u32);

		if (render_state.memory) VirtualFree(render_state.memory, 0, MEM_RELEASE);
		render_state.memory = VirtualAlloc(0, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		render_state.bitmap_info.bmiHeader.biSize = sizeof(render_state.bitmap_info.bmiHeader);
		render_state.bitmap_info.bmiHeader.biWidth = render_state.width;
		render_state.bitmap_info.bmiHeader.biHeight = render_state.height;
		render_state.bitmap_info.bmiHeader.biPlanes = 1;
		render_state.bitmap_info.bmiHeader.biBitCount = 32;
		render_state.bitmap_info.bmiHeader.biCompression = BI_RGB;
	} break;

	default: {
		result = DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	}
	return result;
}


int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

	// create window class
	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "Game Window Class";
	window_class.lpfnWndProc = window_callback;

	// register class
	RegisterClass(&window_class);

	// create window
	HWND window = CreateWindow(window_class.lpszClassName, "Pong", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, 0, 0, hInstance, 0);

	HDC hdc = GetDC(window);

	Input input = {};

	// Initialze winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		return -1;
	}

	// Create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		return -1;
	}

	// Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // Could also use inet_pton .... 

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// Tell Winsock the socket is for listening 
	listen(listening, SOMAXCONN);

	// Create and zero the master file descriptor set
	fd_set master;
	FD_ZERO(&master);

	// Add our first socket that we're interested in interacting with; the listening socket!
	// It's important that this socket is added for our server or else we won't 'hear' incoming
	// connections 
	FD_SET(listening, &master);

	float delta_time = 0.016666f;
	LARGE_INTEGER frame_begin_time;
	QueryPerformanceCounter(&frame_begin_time);

	float performance_frequency;
	{
		LARGE_INTEGER perf;
		QueryPerformanceFrequency(&perf);
		performance_frequency = (float)perf.QuadPart;
	}

	while (running) {

		// Input
		MSG message;

		for (int i = 0; i < BUTTON_COUNT; i++) {
			input.buttons[i].changed = false;
		}

		while (PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
			switch (message.message) {
			case WM_KEYUP:
			case WM_KEYDOWN: {
				u32 vk_code = (u32)message.wParam;
				bool is_down = ((message.lParam & (1 << 31)) == 0);

#define process_button(b, vk)\
case vk: {\
input.buttons[b].changed = (is_down != input.buttons[b].is_down);\
input.buttons[b].is_down = is_down;\
} break;

				switch (vk_code) {
					process_button(BUTTON_A, 'A');
					process_button(BUTTON_B, 'B');
					process_button(BUTTON_C, 'C');
					process_button(BUTTON_D, 'D');
					process_button(BUTTON_E, 'E');
					process_button(BUTTON_F, 'F');
					process_button(BUTTON_G, 'G');
					process_button(BUTTON_H, 'H');
					process_button(BUTTON_I, 'I');
					process_button(BUTTON_J, 'J');
					process_button(BUTTON_K, 'K');
					process_button(BUTTON_L, 'L');
					process_button(BUTTON_M, 'M');
					process_button(BUTTON_N, 'N');
					process_button(BUTTON_O, 'O');
					process_button(BUTTON_P, 'P');
					process_button(BUTTON_Q, 'Q');
					process_button(BUTTON_R, 'R');
					process_button(BUTTON_S, 'S');
					process_button(BUTTON_T, 'T');
					process_button(BUTTON_U, 'U');
					process_button(BUTTON_V, 'V');
					process_button(BUTTON_W, 'W');
					process_button(BUTTON_X, 'X');
					process_button(BUTTON_Y, 'Y');
					process_button(BUTTON_Z, 'Z');
					process_button(BUTTON_BACKSPACE, VK_BACK);
					process_button(BUTTON_UP, VK_UP);
					process_button(BUTTON_DOWN, VK_DOWN);
					process_button(BUTTON_ENTER, VK_RETURN);
					process_button(BUTTON_SPACE, VK_SPACE);
				}
			} break;
			default: {
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
			}
		}

		// Simulate
		run_server(&input, listening, master, delta_time);

		// Render
		StretchDIBits(hdc, 0, 0, render_state.width, render_state.height, 0, 0,
			render_state.width, render_state.height, render_state.memory, &render_state.bitmap_info,
			DIB_RGB_COLORS, SRCCOPY);

		LARGE_INTEGER frame_end_time;
		QueryPerformanceCounter(&frame_end_time);
		delta_time = (float)(frame_end_time.QuadPart - frame_begin_time.QuadPart) / performance_frequency;
		frame_begin_time = frame_end_time;

		frame++;
		frame2++;
	}
	// Remove the listening socket from the master file descriptor set and close it
	// to prevent anyone else trying to connect.
	FD_CLR(listening, &master);
	closesocket(listening);

	// Cleanup winsock
	WSACleanup();

	return 0;
}