// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "injector/injector.hpp"
#include "ini/IniReader.h"

CIniReader iniReader("AnimationController.ini");

char hk_toggleHood, hk_toggleTrunk, hk_toggleDoors, hk_enableHood, hk_enableTrunk;
bool FixEngineTextureInRaces, ShowTrunkComponentsInRaces, ALwaysEnableTrunk, AlwaysShowEngineModel, HideHood = false;
float animationSpeed;

enum CarAnimState
{
	Opened = 0,
	Closed = 1,
	Animating = 2
};

enum CarAnimLocation
{
	Hood = 0,
	Trunk = 1,
	LeftDoor = 2,
	RightDoor = 3
};

struct PartAnimation
{
	PartAnimation* next;
	PartAnimation* prev;
	float speed;
	CarAnimLocation part;
	CarAnimState state;
};

int* GameState = (int*)0x8654A4;
bool* IsNoFocus = (bool*)0x8709E0;

float* TrunkClosedNeon = (float*)0x00615C04;
float* TrunkClosed = (float*)0x00620C02;

float* EngineClosed = (float*)0x00620BC9;

void* TheCarLoader = (void*)0x008A3288;
int* PlayersByNumber = (int*)0x008900AC;
int** CarPartsScenesList = (int**)0x00827C9C;
auto Game_LoadCareerCarPartsAnims = (int(__fastcall*)(void*))0x00633A00;
auto Game_TriggerCarPartAnimation = (int(__cdecl*)(int, CarAnimLocation part, bool direction, float speed))0x0061F820;
auto Game_FEDoCarPartAnimNow = (int(__cdecl*)(CarAnimLocation part, bool direction, float speed))0x004C1860;

void LoadCareerCarPartsAnims()
{
	if (*GameState == 6)
	{
		Game_LoadCareerCarPartsAnims(TheCarLoader);
	}
}

void TriggerCarPartAnimation(CarAnimLocation part, bool direction, float speed)
{
	if (*GameState == 3)
	{
		Game_FEDoCarPartAnimNow(part, direction, speed);
	}
	if (*GameState == 6)
	{
		int val = *(int*)(*(int*)(*PlayersByNumber + 4) + 0x554);
		Game_TriggerCarPartAnimation(val, part, direction, speed);
	}
}

CarAnimState GetPartState(CarAnimLocation part)
{
	PartAnimation** CarPartsScenesListHead = (PartAnimation * *)(*CarPartsScenesList + 2);
	if ((int)CarPartsScenesListHead != (int)* CarPartsScenesListHead)
	{
		PartAnimation* cur = *CarPartsScenesListHead;

		while (cur->next != *CarPartsScenesListHead)
		{
			if (cur->part == part)
			{
				if (cur->state != CarAnimState::Closed && cur->state != CarAnimState::Opened)
				{
					return CarAnimState::Animating;
				}

				return cur->state;

				break;
			}

			cur = cur->next;
		}
	}

	return CarAnimState::Closed;
}

void TriggerCarPartAnimation(CarAnimLocation part)
{
	LoadCareerCarPartsAnims();	

	CarAnimState state = GetPartState(part);
	if (state == CarAnimState::Animating)
	{
		return;
	}

	bool direction = false;
	if (state == CarAnimState::Closed)
	{
		direction = true;
	}

	TriggerCarPartAnimation(part, direction, animationSpeed);
	if (part == CarAnimLocation::LeftDoor)
	{
		TriggerCarPartAnimation(CarAnimLocation::RightDoor, direction, animationSpeed);
	}
}

void CheckKeyAndTrigger(char hotKey, CarAnimLocation part)
{
	if (GetAsyncKeyState(hotKey))
	{
		if (hotKey == hk_toggleHood)
		{
			HideHood = false;
		}

		TriggerCarPartAnimation(part);

		while ((GetAsyncKeyState(hotKey) & 0x8000) > 0) { Sleep(0); }
	}
}

void ToggleTrunk(bool enabled)
{
	if (enabled)
	{
		*TrunkClosedNeon = 1.0;
		*TrunkClosed = 0.7;
		injector::MakeNOP(0x00615BF8, 6, true); // Enables neon in races
	}
	else
	{
		*TrunkClosedNeon = 0.0;
		*TrunkClosed = 0.0;
	}
}

DWORD HoodRenderCave1 = 0x006233AB;
void __declspec(naked) HoodRenderCave()
{
	__asm
	{
		cmp HideHood, 1;
		jne HoodRenderDefault;

		mov edx, [esp + 0x0000030C];
		xor edx, 1;
		jmp HoodRenderCave1;

	HoodRenderDefault:
		mov edx, 1;
		jmp HoodRenderCave1;
	}
}

DWORD HoodUnderRender1 = 0x006233D0;
void __declspec(naked) HoodUnderRenderCave()
{
	__asm
	{
		cmp HideHood, 1;
		jne HoodUnderRenderDefault;
		mov edx, 0;
		jmp HoodUnderRender1;

	HoodUnderRenderDefault:
		mov edx, [esp + 0x0000030C];
		jmp HoodUnderRender1;
	}
}

DWORD WINAPI Thing(LPVOID)
{
	while (true)
	{
		Sleep(1000 / 30);
		if (*IsNoFocus && !(*GameState == 3 || *GameState == 6))
		{
			continue;
		}

		CheckKeyAndTrigger(hk_toggleHood, CarAnimLocation::Hood);
		CheckKeyAndTrigger(hk_toggleTrunk, CarAnimLocation::Trunk);
		CheckKeyAndTrigger(hk_toggleDoors, CarAnimLocation::LeftDoor);

		if (GetAsyncKeyState(hk_enableTrunk))
		{
			ALwaysEnableTrunk = !ALwaysEnableTrunk;
			ToggleTrunk(ALwaysEnableTrunk);

			iniReader.WriteInteger("General", "ALwaysEnableTrunk", ALwaysEnableTrunk);

			while ((GetAsyncKeyState(hk_enableTrunk) & 0x8000) > 0) { Sleep(0); }
		}

		if (GetAsyncKeyState(hk_enableHood))
		{
			if (!(!HideHood && GetPartState(CarAnimLocation::Hood) == CarAnimState::Opened))
			{
				TriggerCarPartAnimation(CarAnimLocation::Hood);
			}

			HideHood = true;			

			while ((GetAsyncKeyState(hk_enableHood) & 0x8000) > 0) { Sleep(0); }
		}
	}
}

void Init()
{
	//General
	FixEngineTextureInRaces = iniReader.ReadInteger("General", "FixEngineTextureInRaces", 1);
	ShowTrunkComponentsInRaces = iniReader.ReadInteger("General", "ShowTrunkComponentsInRaces", 1);
	ALwaysEnableTrunk = iniReader.ReadInteger("General", "ALwaysEnableTrunk", 1);
	AlwaysShowEngineModel = iniReader.ReadInteger("General", "AlwaysShowEngineModel", 0);

	// Hotkeys
	hk_toggleHood = toupper(iniReader.ReadString("Hotkeys", "ToggleHood", "1")[0]);
	hk_enableHood = toupper(iniReader.ReadString("Hotkeys", "HideHood", "2")[0]);

	hk_toggleTrunk = toupper(iniReader.ReadString("Hotkeys", "ToggleTrunk", "3")[0]);
	hk_enableTrunk = toupper(iniReader.ReadString("Hotkeys", "EnableTrunk", "4")[0]);

	hk_toggleDoors = toupper(iniReader.ReadString("Hotkeys", "ToggleDoors", "5")[0]);

	animationSpeed = iniReader.ReadFloat("General", "AnimationSpeed", 1.0);

	if (ShowTrunkComponentsInRaces)
	{
		injector::WriteMemory<char>(0x00630C27, 0xEB, true);
	}

	if (FixEngineTextureInRaces)
	{
		injector::WriteMemory<int>(0x0057F6BF, 0xB8, true);
	}

	if (ALwaysEnableTrunk)
	{
		ToggleTrunk(true);
	}

	if (AlwaysShowEngineModel)
	{
		injector::MakeNOP(0x006304CD, 6, true);
		*EngineClosed = 1.0;
	}

	injector::MakeJMP(0x006233A6, HoodRenderCave, true);
	injector::MakeJMP(0x006233C9, HoodUnderRenderCave, true);

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)& Thing, NULL, 0, NULL);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
		IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(base);
		IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

		if ((base + nt->OptionalHeader.AddressOfEntryPoint + (0x400000 - base)) == 0x75BCC7) // Check if .exe file is compatible - Thanks to thelink2012 and MWisBest
		{
			Init();
		}

		else
		{
			MessageBoxA(NULL, "This .exe is not supported.\nPlease use v1.2 NTSC speed2.exe (4,57 MB (4.800.512 bytes)).", "NFSU2 Animation Controller", MB_ICONERROR);
			return FALSE;
		}
	}
	break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

