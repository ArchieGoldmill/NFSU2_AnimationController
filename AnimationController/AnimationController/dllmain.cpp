#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\injector\injector.hpp"
#include <cstdint>
#include "..\includes\IniReader.h"
#include <d3d9.h>

using namespace std;

DWORD WINAPI Thing(LPVOID);
int ThreadDelay = 1;
bool IsOnFocus;
char* iniFile = "AnimationController.ini";

DWORD GameState;
HWND windowHandle;

enum AnimationPart
{
	Hood = 0,
	Trunk = 1,
	LeftDoor = 2,
	RightDoor = 3
};

int blank[100];

char hk_toggleHood, hk_toggleTrunk, hk_toggleDoors, hk_enableHood, hk_enableTrunk;
bool FixEngineInRaces, EnableTrunkContentsInRaces, RemoveBlackCoverUnderHood;

void* GetAbsolutePtr(int* offsets, int len)
{
	void* cur = 0;

	for (int i = 0; i < len - 1; i++)
	{
		cur = (void*)(*(int*)((int)cur + offsets[i]));
	}

	cur = (void*)((int)cur + offsets[len - 1]);

	return cur;
}

bool IsCarDataLoaded()
{
	int val = *((int*)0x00827C9C);
	return val != 0x00827C9C;
}

void(__cdecl* Game_LoadCarData)() = (void(__cdecl*)())0x00633A00;
void LoadCarData() {
	if (IsCarDataLoaded())
	{
		return;
	}

	__asm {
		mov ECX, 0x008A3288
		mov EDI, 0x00799F1C
		mov ESI, blank
		call Game_LoadCarData
	}

	Sleep(300);
}

void(__cdecl* Game_ExecuteAnimation)(int, int, int, int, float) = (void(__cdecl*)(int, int, int, int, float))0x00433E20;
void ExecuteAnimation(AnimationPart part, int animation, int direction, float speed)
{
	if (!IsCarDataLoaded())
	{
		return;
	}

	int ptr[3] = { 0x008A7110, 0x8, 0x1F4 };
	int val = *(int*)GetAbsolutePtr(ptr, 3);

	Game_ExecuteAnimation(val, part, direction, animation, speed);
}

void SetPartState(AnimationPart part, int state)
{
	int ptr[2] = { 0x00827C9C,0x8 };
	int* baseAdr = (int*)GetAbsolutePtr(ptr, 2);

	if ((int)baseAdr == *baseAdr)
	{
		return;
	}

	vector<int> addr;
	addr.push_back(0x00827C9C);
	addr.push_back(0x8);
	addr.push_back(0);

	while (true)
	{
		int* partDataAddr = (int*)GetAbsolutePtr(&addr[0], addr.size());

		int p = *((int*)((int)partDataAddr + 0xC));
		if (p == (int)part)
		{
			*((int*)((int)partDataAddr + 0x10)) = state;
			return;
		}
		else
		{
			addr.push_back(0);

			if ((int*)GetAbsolutePtr(&addr[0], addr.size()) == baseAdr)
			{
				break;
			}
		}
	}
}

int GetPartState(AnimationPart part)
{
	int ptr[2] = { 0x00827C9C,0x8 };
	int* baseAdr = (int*)GetAbsolutePtr(ptr, 2);

	if ((int)baseAdr == *baseAdr)
	{
		return 0;
	}

	vector<int> addr;
	addr.push_back(0x00827C9C);
	addr.push_back(0x8);
	addr.push_back(0);

	while (true)
	{
		int* partDataAddr = (int*)GetAbsolutePtr(&addr[0], addr.size());

		int p = *((int*)((int)partDataAddr + 0xC));
		if (p == (int)part)
		{
			return *((int*)((int)partDataAddr + 0x10));
		}
		else
		{
			addr.push_back(0);

			if ((int*)GetAbsolutePtr(&addr[0], addr.size()) == baseAdr)
			{
				break;
			}
		}
	}

	return 0;
}

void LoadPart(AnimationPart part, int animation = 0)
{
	int state = GetPartState(part);

	ExecuteAnimation(part, animation, 0, -1.0);
}

void EnablePart(AnimationPart part)
{
	LoadPart(part);

	int state = GetPartState(part);
	if (state == -1)
	{
		SetPartState(part, 1);
		return;
	}

	if (state != 0)
	{
		SetPartState(part, 0);
	}

	Sleep(10);

	SetPartState(part, -1);
}

void TogglePart(AnimationPart part, int animation = 0)
{
	int dir = 0;

	int state = GetPartState(part);

	if (state == -1)
	{
		SetPartState(part, 1);
		dir = 1;
	}
	else
	{
		dir = state;
	}

	// Animation
	CIniReader iniReader(iniFile);
	float animationSpeed = iniReader.ReadFloat("Animation", "AnimationSpeed", 1.0);
	if (animationSpeed <= 0) {
		animationSpeed = 0.1;
	}

	if (animationSpeed > 10) {
		animationSpeed = 10;
	}

	ExecuteAnimation(part, animation, dir, animationSpeed);
}


DWORD WINAPI Thing(LPVOID)
{
	while (true)
	{
		Sleep(ThreadDelay);

		GameState = *(DWORD*)0x8654A4;
		windowHandle = *(HWND*)0x870990;
		IsOnFocus = !(*(bool*)0x8709E0);

		if ((GetAsyncKeyState(hk_toggleHood) & 1) && IsOnFocus && (GameState == 3 || GameState == 6))
		{
			// This need to be done after game fully loads, because working with bynamic memmory
			if (RemoveBlackCoverUnderHood) {
				int ptr = *((int*)0x008A1CB0);
				*((int*)(ptr + 0x171)) = 0x49;
				*((int*)(ptr + 0x185)) = 0x49;
				*((int*)(ptr + 0x199)) = 0x49;
			}

			LoadCarData();

			CIniReader iniReader(iniFile);
			int hoodStyle = iniReader.ReadInteger("Animation", "HoodStyle", 0); // 0 - Default, 1 - Front, 2 - Sides, 3 - Dual Front
			TogglePart(AnimationPart::Hood, hoodStyle);

			while ((GetAsyncKeyState(hk_toggleHood) & 0x8000) > 0) { Sleep(0); }
		}

		if ((GetAsyncKeyState(hk_enableHood) & 1) && IsOnFocus && (GameState == 3 || GameState == 6))
		{
			LoadCarData();

			EnablePart(AnimationPart::Hood);

			while ((GetAsyncKeyState(hk_enableHood) & 0x8000) > 0) { Sleep(0); }
		}

		if ((GetAsyncKeyState(hk_toggleTrunk) & 1) && IsOnFocus && (GameState == 3 || GameState == 6))
		{
			LoadCarData();

			CIniReader iniReader(iniFile);
			int trunkStyle = iniReader.ReadInteger("Animation", "TrunkStyle", 0);
			TogglePart(AnimationPart::Trunk, trunkStyle);

			while ((GetAsyncKeyState(hk_toggleTrunk) & 0x8000) > 0) { Sleep(0); }
		}

		if ((GetAsyncKeyState(hk_enableTrunk) & 1) && IsOnFocus && (GameState == 3 || GameState == 6))
		{
			LoadCarData();

			EnablePart(AnimationPart::Trunk);

			while ((GetAsyncKeyState(hk_enableTrunk) & 0x8000) > 0) { Sleep(0); }
		}

		if ((GetAsyncKeyState(hk_toggleDoors) & 1) && IsOnFocus && (GameState == 3 || GameState == 6))
		{
			LoadCarData();

			CIniReader iniReader(iniFile);
			int doorsStyle = iniReader.ReadInteger("Animation", "DoorsStyle", 0); // 0 - Default, 1 - Scissors, 2- Suicide
			if (doorsStyle > 6 || doorsStyle < 0)
			{
				doorsStyle = 0;
			}

			TogglePart(AnimationPart::LeftDoor, doorsStyle);
			TogglePart(AnimationPart::RightDoor, doorsStyle);

			while ((GetAsyncKeyState(hk_toggleDoors) & 0x8000) > 0) { Sleep(0); }
		}
	}
}

void Init()
{
	CIniReader iniReader(iniFile);

	//General
	FixEngineInRaces = iniReader.ReadBoolean("General", "FixEngineInRaces", true);
	EnableTrunkContentsInRaces = iniReader.ReadBoolean("General", "EnableTrunkContentsInRaces", true);
	RemoveBlackCoverUnderHood = iniReader.ReadBoolean("General", "RemoveBlackCoverUnderHood", false);

	// Hotkeys
	hk_toggleHood = toupper(iniReader.ReadString("Hotkeys", "ToggleHood", "U")[0]);
	hk_enableHood = toupper(iniReader.ReadString("Hotkeys", "EnableHood", "J")[0]);

	hk_toggleTrunk = toupper(iniReader.ReadString("Hotkeys", "ToggleTrunk", "I")[0]);
	hk_enableTrunk = toupper(iniReader.ReadString("Hotkeys", "EnableTrunk", "K")[0]);

	hk_toggleDoors = toupper(iniReader.ReadString("Hotkeys", "ToggleDoors", "O")[0]);

	if (FixEngineInRaces) {
		*((char*)0x0057F6BF) = 0xB8;
		*((int*)0x0057F6C0) = 0;
	}

	if (EnableTrunkContentsInRaces) {
		injector::MakeNOP(0x00630C29, 5, true);
	}

	// Fix hood types
	injector::MakeNOP(0x00433B51, 6);
	*((char*)0x00433B51) = 0x75;
	*((char*)0x00433B52) = 0xA1;

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)& Thing, NULL, 0, NULL);
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
		IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(base);
		IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

		if ((base + nt->OptionalHeader.AddressOfEntryPoint + (0x400000 - base)) == 0x75BCC7) // Check if .exe file is compatible - Thanks to thelink2012 and MWisBest
			Init();

		else
		{
			MessageBoxA(NULL, "This .exe is not supported.\nPlease use v1.2 NTSC speed2.exe (4,57 MB (4.800.512 bytes)).", "NFSU2 Animation Controller", MB_ICONERROR);
			return FALSE;
		}
	}
	return TRUE;

}