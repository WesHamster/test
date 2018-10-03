//Server
#define WIN32_LEAN_AND_MEAN

#include <enet\enet.h>
#include <iostream>
#include <sstream>
#include <string>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <map>

using namespace std;

void LoadTranslations(map<string, string> &translations);
void ClearScreen();

int main() {
	bool done = false;
	bool connected = false;

	map<string, string> translations;
	LoadTranslations(translations);

	system("title Server");

	if (!enet_initialize())
		cout << "Initialized ENet\n";

	ENetAddress address;
	ENetHost * server;
	ENetEvent event;

	address.host = ENET_HOST_ANY;
	address.port = 5555;
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

	cout << "\nWaiting for connection..." << endl;

	while (!done) {
		while (enet_host_service (server, &event, 0) > 0) {
            switch(event.type) {
                case ENET_EVENT_TYPE_CONNECT:
					{
						printf ("New connection from: %x:%u.\n", event.peer -> address.host, event.peer -> address.port);
						event.peer->timeoutLimit = 10000000000000000000;
						connected = true;
						Beep(400, 150);
						Beep(300, 150);

						map<string, string>::iterator i;
						for(i = translations.begin(); i != translations.end(); i++) {
							//cout << p->first << "->" << p->second << endl;
							char packet[256];
							sprintf_s(packet, sizeof(packet), "%s:%s", i->first.c_str(), i->second.c_str());
							ENetPacket *p = enet_packet_create((char*)packet, strlen(packet)+1, ENET_PACKET_FLAG_RELIABLE);
							enet_host_broadcast(server, 0, p);
						}
						char packet[256];
						sprintf_s(packet, sizeof(packet), "~");
						ENetPacket *p = enet_packet_create((char*)packet, strlen(packet)+1, ENET_PACKET_FLAG_RELIABLE);
						enet_host_broadcast(server, 0, p);
					}
                    break;

                case ENET_EVENT_TYPE_RECEIVE:
					{
						if(event.packet->data[0] == 'S') {
							
						}

						enet_packet_destroy(event.packet);
					}

                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    printf("(Client) %s disconnected.\n", event.peer->data);
                    event.peer->data = NULL;
					connected = false;
                    break;
            }
		}
	}
}

void ClearScreen()
{
    HANDLE hOut;
    COORD Position;

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    Position.X = 0;
    Position.Y = 0;
    SetConsoleCursorPosition(hOut, Position);
}

void LoadTranslations(map<string, string> &translations) {
	string file = "";
	string word1 = "", word2 = "";
	vector<string> fileLines;
	fstream config("translations.txt");
	if (config.good()) {
		while (!config.eof()) {
			getline(config, file);
			//cout << file << endl;
			fileLines.push_back(file);
		}
		config.close();
	}
	else {
		cout << "Unable to find file..." << endl;
		cout << "Created config file. Enter information into config and restart" << endl;
		ofstream config;
		config.open ("translations.txt");
		config << "example:exampool\n";
		config.close();
		cout << "Press any key to exit..." << endl;
		system("pause>nul");
		exit(EXIT_FAILURE);
	}
	
	for (unsigned int i = 0; i < fileLines.size(); i++) {
		istringstream ss(fileLines[i]);
		string token = "";
		int filePlace = 0;

		while(getline(ss, token, ':')) {
			if (filePlace == 0)
				word1 = token;
			else if (filePlace == 1)
				word2 = token;
			filePlace++;
		}
		if (i == 0)
			word1.erase(0, 3);
		translations[word1] = word2;
	}
}