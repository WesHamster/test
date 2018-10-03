#pragma once
#define WIN32_LEAN_AND_MEAN
#include <allegro5\allegro.h>
#include <allegro5\allegro_image.h>
#include <allegro5\allegro_primitives.h>
#include <enet\enet.h>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

#include <iostream>
#include <lmcons.h>
#include <map>
#include <sapi.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <shellapi.h>
#include <fstream>

using namespace std;
using namespace cv;

struct Pixel {
	int x, y;
	ALLEGRO_COLOR color;
};

void SimulateKey(WORD vkey, bool down);

void PressKey(WORD vkey);

void SetClipboard(HWND hwnd, const std::string &s);

void SimulateMouse(int button, bool down);

void SendPacket(ENetPeer *peer, const char *arg, ...);

string GetUsername();

vector<string> GetIpAndPort(string exe);

void DeactivateEffects(map<int, bool> &effects);

bool ChangeVolume(double nVolume, bool bScalar);

void VolumeUp();
wstring s2ws(const std::string& s);
void talk(string sentenceString);
void FindSounds(vector<string> &sounds);

void OpenCloseDrive();

void WhiteRectangles(int screenWidth, int screenHeight);

void ScreenShot(char*BmpName);

void SendScreenData(vector<Pixel> &screenData, ENetPeer *peer);

string GetImageData();