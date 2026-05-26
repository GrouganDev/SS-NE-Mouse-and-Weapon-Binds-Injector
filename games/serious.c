//==========================================================================
// Mouse Injector for Dolphin
//==========================================================================
// Copyright (C) 2019-2020 Carnivorous
// All rights reserved.
//
// Mouse Injector is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, visit http://www.gnu.org/licenses/gpl-2.0.html
//==========================================================================
#include <stdint.h>
#include "../memory.h"
#include "../mouse.h"
#include "serious.h"
#include "game.h"
#include <stdio.h>
#include <conio.h>
#include "../seriousinputchecker.h"

#define PI 3.14159265f // 0x40490FDB
#define TAU 6.2831853f // 0x40C90FDB
#define CAMYLIMITPLUS 1.308996916f // 0x3FA78D36
#define CAMYLIMITMINUS -1.308996916f // 0xBFA78D36
// SERIOUS ADDRESSES - OFFSET ADDRESSES BELOW (REQUIRES PLAYERBASE TO USE)
#define SERIOUS_camx 0x8133C930 - 0x8133C6A0
#define SERIOUS_camy 0x8133C934 - 0x8133C6A0
#define SERIOUS_fov 0x8133C99C - 0x8133C6A0
// STATIC ADDRESSES BELOW
#define SERIOUS_playerbase 0x802D8948 // playable character pointer


static const SERIOUSKEYS* KEYS;


static uint8_t SERIOUS_Status(void);
static void SERIOUS_Inject(void);


static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Serious Sam: Next Encounter",
	SERIOUS_Status,
	SERIOUS_Inject,
	15, // if tickrate is any lower, mouse input will get sluggish
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SERIOUS = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SERIOUS_Status(void)
{
	return (MEM_ReadUInt(0x80000000) == 0x47334245U && MEM_ReadUInt(0x80000004) == 0x39470001U); // check game header to see if it matches SERIOUS
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SERIOUS_Inject(void)
{
	SERIOUS_Code_Inject();
	SERIOUS_Weapon_Check();

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	const uint32_t playerbase = MEM_ReadUInt(SERIOUS_playerbase);
	if(!playerbase) // if playerbase is invalid
		return;

	const float looksensitivity = (float)sensitivity / 40.f;
	const float fov = MEM_ReadFloat(playerbase + SERIOUS_fov);
	float camx = MEM_ReadFloat(playerbase + SERIOUS_camx);
	float camy = MEM_ReadFloat(playerbase + SERIOUS_camy);
	if(camx > -TAU && camx < TAU && fov > 9.f)
	{
		camx += (float)xmouse / 10.f * looksensitivity / (360.f / TAU) * (fov / 90); // normal calculation method for X
		camy += (float)(invertpitch ? -ymouse : ymouse) / 10.f * looksensitivity / (90.f / PI) * (fov / 90); // normal calculation method for Y
		while(camx >= TAU)
			camx -= TAU;
		while(camx <= TAU)
			camx += TAU;
		camy = ClampFloat(camy, CAMYLIMITMINUS, CAMYLIMITPLUS);
		MEM_WriteFloat(playerbase + SERIOUS_camx, camx);
		MEM_WriteFloat(playerbase + SERIOUS_camy, camy);
	}
}

//==========================================================================
// Purpose: injects custom assembly code into dolphin in order to allow for custom weapon binds
//==========================================================================
static void SERIOUS_Code_Inject(void)
{
	MEM_WriteInt(0x800c5aa0, 0x49712D95); //bl 0x817d8834
	MEM_WriteInt(0x817d8834, 0x3DE0817D); //lis r15, 0x817D 
	MEM_WriteInt(0x817d8838, 0x61EF885C); //ori r15, r15, 0x885C 
	MEM_WriteInt(0x817d883c, 0x83CF0000); //lwz r30, 0x00(r15) 
	MEM_WriteInt(0x817d8840, 0x4e800020); //blr


	//branch if the game plans to swap weapons
	MEM_WriteInt(0x800c5afc, 0x49712D95); //bl 0x817d8890 

	//load current weapon index into r16
	MEM_WriteInt(0x817d8890, 0x3DE0817D); //lis r15, 0x817D 
	MEM_WriteInt(0x817d8894, 0x61EF8860); //ori r15, r15, 0x8860 
	MEM_WriteInt(0x817d8898, 0x820F0000); //lwz r16, 0x00(r15) 

	//Compare r30 to r16 and swap to new weapon if equal. Check alt weapon otherwise
	MEM_WriteInt(0x817d889c, 0x7C10F000); //cmpw r16, r30 
	MEM_WriteInt(0x817d88a0, 0x4082000C); //bne 0x817d88a8 
	MEM_WriteInt(0x817d88a4, 0x93DF05F8); //stw r30, 0x05F8(r31) 
	MEM_WriteInt(0x817d88a8, 0x4e800020); //blr

	//check alt weapon (address 0x817D8868)
	MEM_WriteInt(0x817d88ac, 0x3DE0817D); //lis r15, 0x817D 
	MEM_WriteInt(0x817D88b0, 0x61EF8868); //ori r15, r15, 0x8868 
	MEM_WriteInt(0x817d88b4, 0x820F0000); //lwz r16, 0x00(r15) 
	MEM_WriteInt(0x817d88b8, 0x7C10F000); //cmpw r16, r30 
	MEM_WriteInt(0x817d88bc, 0x40820008); //bne 0x817d88a8 
	MEM_WriteInt(0x817d88c0, 0x921F05F8); //stw r16, 0x05F8(r31) 
	MEM_WriteInt(0x817d88c4, 0x4e800020); //blr
}

//==========================================================================
// Purpose: restores original assembly instructions where overwritten
//==========================================================================
void SERIOUS_Code_DeInject(void)
{
	MEM_WriteInt(0x800c5aa0, 0x83df05f8); //lwz r30, 0x05F8 (r31)
    MEM_WriteInt(0x800c5afc, 0x93df05f8); //stw r30, 0x05F8 (r31)
}



/*
WEAPON INDICES:

0x00000000 - Chainsaw
0x00000001 - Pistol(s)
0x00000002 - Uzis
0x00000003 - Shotgun
0x00000004 - Minigun
0x00000005 - Grenade Launcher
0x00000006 - Rocket Launcher
0x00000007 - Flamethrower
0x00000008 - Sniper Rifle
0x00000009 - Sirian Powergun
0x0000000A - Cannon
0x0000000B - Serious Bomb
*/

//==========================================================================
// Purpose: checks key pressed according to 'KEYS' to set up new weapon to switch to
//==========================================================================
static void SERIOUS_Weapon_Check(void)
{
	uint32_t currentweapon = 0xFFFFFFFF;
	uint32_t altweapon = 0xFFFFFFFF;

	if(KEYS->K_Chainsaw_Pressed || KEYS->K_Chainsaw_Alt_Pressed){
		currentweapon = 0x00000000;
	}
	if(KEYS->K_Pistols_Pressed || KEYS->K_Pistols_Alt_Pressed){
		currentweapon = 0x00000001;
	}
	if(KEYS->K_Shotgun_Pressed || KEYS->K_Shotgun_Alt_Pressed){
		currentweapon = 0x00000003;
	}
	if(KEYS->K_Bullets_Pressed || KEYS->K_Bullets_Alt_Pressed){
		SERIOUS_Assign_Weapon_Group(0x00000002, 0x00000004, &currentweapon, &altweapon);
	}
	if(KEYS->K_Explosives_Pressed || KEYS->K_Explosives_Alt_Pressed){
		SERIOUS_Assign_Weapon_Group(0x00000006, 0x00000005, &currentweapon, &altweapon);
	}
	if(KEYS->K_FlameRifle_Pressed || KEYS->K_FlameRifle_Alt_Pressed){
		SERIOUS_Assign_Weapon_Group(0x00000007, 0x00000008, &currentweapon, &altweapon);
	}
	if(KEYS->K_CannonPowergun_Pressed || KEYS->K_CannonPowergun_Alt_Pressed){
		SERIOUS_Assign_Weapon_Group(0x0000000A, 0x00000009, &currentweapon, &altweapon);
	}
	if(KEYS->K_Bomb_Pressed || KEYS->K_Bomb_Alt_Pressed){
		currentweapon = 0x0000000B;
	}

	if(currentweapon != 0xFFFFFFFF){
		SERIOUS_Update_Weapon(currentweapon, altweapon); // update memory to swap to selected weapon
	}
}

//==========================================================================
// Purpose: checks the player's current weapon to determine which weapon in the category is to be selected
//==========================================================================
static void SERIOUS_Assign_Weapon_Group(uint32_t first, uint32_t second, uint32_t* mainweapon, uint32_t* altweapon)
{	
	uint32_t currentweapon = MEM_ReadInt(0x817D8860);

	if (currentweapon == first)
	{
		*mainweapon = second;
		*altweapon = first;
	}
	else
	{
		*mainweapon = first;
		*altweapon = second;
	}
}

//==========================================================================
// Purpose: updates necessary regions of memory to contain the new weapon index for gameplay
//==========================================================================
static void SERIOUS_Update_Weapon(uint32_t mainweapon, uint32_t altweapon)
{
	uint32_t clampedweaponind = (((int32_t)mainweapon - 1) % 12);

	MEM_WriteInt(0x817D885C, clampedweaponind);

	MEM_WriteInt(0x817D8860, mainweapon);

	MEM_WriteInt(0x817D8868, altweapon);
}


//==========================================================================
// Purpose: sets the 'KEYS' pointer to point to main.c's 'keys'
//==========================================================================
void UPDATE_Keys_Struct(SERIOUSKEYS* keys){
	KEYS = keys;
}


//==========================================================================
// Purpose: large function to check the status of every key for weapon binds
//==========================================================================
void UPDATE_Serious_Keys(SERIOUSKEYS* keys)
{
	SEARCH_Serious_Keys(&keys->K_Chainsaw_Pressed, &keys->K_Chainsaw_Last_Pressed, &keys->K_Chainsaw_Alt_Pressed, &keys->K_Chainsaw_Alt_Last_Pressed, &keys->K_Chainsaw[0]);

	SEARCH_Serious_Keys(&keys->K_Pistols_Pressed, &keys->K_Pistols_Last_Pressed, &keys->K_Pistols_Alt_Pressed, &keys->K_Pistols_Alt_Last_Pressed, &keys->K_Pistols[0]);

	SEARCH_Serious_Keys(&keys->K_Shotgun_Pressed, &keys->K_Shotgun_Last_Pressed, &keys->K_Shotgun_Alt_Pressed, &keys->K_Shotgun_Alt_Last_Pressed, &keys->K_Shotgun[0]);

	SEARCH_Serious_Keys(&keys->K_Bullets_Pressed, &keys->K_Bullets_Last_Pressed, &keys->K_Bullets_Alt_Pressed, &keys->K_Bullets_Alt_Last_Pressed, &keys->K_Bullets[0]);

	SEARCH_Serious_Keys(&keys->K_Explosives_Pressed, &keys->K_Explosives_Last_Pressed, &keys->K_Explosives_Alt_Pressed, &keys->K_Explosives_Alt_Last_Pressed, &keys->K_Explosives[0]);

	SEARCH_Serious_Keys(&keys->K_FlameRifle_Pressed, &keys->K_FlameRifle_Last_Pressed, &keys->K_FlameRifle_Alt_Pressed, &keys->K_FlameRifle_Alt_Last_Pressed, &keys->K_FlameRifle[0]);

	SEARCH_Serious_Keys(&keys->K_CannonPowergun_Pressed, &keys->K_CannonPowergun_Last_Pressed, &keys->K_CannonPowergun_Alt_Pressed, &keys->K_CannonPowergun_Alt_Last_Pressed, &keys->K_CannonPowergun[0]);

	SEARCH_Serious_Keys(&keys->K_Bomb_Pressed, &keys->K_Bomb_Last_Pressed, &keys->K_Bomb_Alt_Pressed, &keys->K_Bomb_Alt_Last_Pressed, &keys->K_Bomb[0]);
}

//==========================================================================
// Purpose: checks to see if there are null values in either the primary or alt weapon keybinds before then checking for that key's status
//==========================================================================
void SEARCH_Serious_Keys(bool *pressed, bool *last_pressed, bool *alt_pressed, bool *alt_last_pressed, uint8_t ls[])
{
	if(ls[0] != 0x00)
	{
		CHECK_Key(pressed, last_pressed, GetAsyncKeyState(ls[0]));
	}
	else
	{
		*pressed = false;
		*last_pressed = false;
	}


	if(ls[1] != 0x00)
	{
		CHECK_Key(alt_pressed, alt_last_pressed, GetAsyncKeyState(ls[1]));
	}
	else
	{
		*alt_pressed = false;
		*alt_last_pressed = false;
	}
}

//==========================================================================
// Purpose: determines the status of a key press with the purpose of only detecting if a key was JUST pressed
//==========================================================================
void CHECK_Key(bool *pressed, bool *last_pressed, SHORT key){
    if(key && !*last_pressed){ // key was just pressed and was not being previously held down
        *last_pressed = true;
        *pressed = true;
    }
    else if(!key) // key was not pressed
    {
        *last_pressed = false;
        *pressed = false;
    }
    else if(*last_pressed) // key is being held down
    {
        *pressed = false;
    }
}