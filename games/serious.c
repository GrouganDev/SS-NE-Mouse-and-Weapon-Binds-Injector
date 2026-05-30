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

#define SERIOUS_has_chainsaw 0x4CF
#define SERIOUS_has_pistol 0x4E7
#define SERIOUS_has_shotgun 0x517
#define SERIOUS_has_uzis 0x4FF
#define SERIOUS_has_minigun 0x52F
#define SERIOUS_has_rocket_launcher 0x55F
#define SERIOUS_has_grenade_launcher 0x547
#define SERIOUS_has_flamer 0x577
#define SERIOUS_has_sniper 0x58F
#define SERIOUS_has_powergun 0x5A7
#define SERIOUS_has_cannon 0x5BF

// bytes
#define SERIOUS_grenades 0x2EED3
#define SERIOUS_spidermines 0x2EED7
#define SERIOUS_mines 0x2EED5
#define SERIOUS_sonic_rockets 0x2EEDD
#define SERIOUS_homing_rockets 0x2EEDB
#define SERIOUS_rockets 0x2EED9
#define SERIOUS_cannons 0x2EEE1
#define SERIOUS_rifle 0x2EEC9
#define SERIOUS_shells 0x2EECB
#define SERIOUS_bombs 0x2EEE3

// halfwords (2 bytes)
#define SERIOUS_napalm 0x2EECC
#define SERIOUS_freeze 0x2EECE
#define SERIOUS_laughing_gas 0x2EED0
#define SERIOUS_bullets 0x2EEC2
#define SERIOUS_homing_bullets 0x2EEC4
#define SERIOUS_reflect_bullets 0x2EEC6
#define SERIOUS_energy 0x2EEDE

// STATIC ADDRESSES BELOW
#define SERIOUS_playerbase 0x802D8948 // playable character pointer
#define SERIOUS_playerbase_PAL 0x802D9048 // PAL character pointer

#define SERIOUS_weapon_switch_first 0x80650b28 // first boolean to toggle weapon switch animation
#define SERIOUS_weapon_switch_second 0x80650b60 // second boolean to toggle same thing (BOTH MUST BE ACTIVATED IN SEQUENTIAL ORDER)

#define SERIOUS_weapon_switch_first_PAL 0x8066e828 // same as above for PAL
#define SERIOUS_weapon_switch_second_PAL 0x8066e860 // same as above for PAL

#define SERIOUS_weapons_cheat 0x802DA283 // all weapons cheat boolean
#define SERIOUS_weapons_cheat_PAL 0x802DA983 // all weapons cheat boolean PAL


// WEAPON INDICIES
#define SERIOUS_chainsaw 0x00000000
#define SERIOUS_pistols 0x00000001
#define SERIOUS_uzis 0x00000002
#define SERIOUS_shotgun 0x00000003
#define SERIOUS_minigun 0x00000004
#define SERIOUS_grenade_launcher 0x00000005
#define SERIOUS_rocket_launcher 0x00000006
#define SERIOUS_flamethrower 0x00000007
#define SERIOUS_sniper_rifle 0x00000008
#define SERIOUS_sirian_powergun 0x00000009
#define SERIOUS_cannon 0x0000000A
#define SERIOUS_bomb 0x0000000B




static const SERIOUSKEYS* KEYS;

static const uint8_t WEAPONOFFSETORDER[] = {SERIOUS_has_chainsaw, SERIOUS_has_pistol, SERIOUS_has_uzis, SERIOUS_has_shotgun, SERIOUS_has_minigun, SERIOUS_has_grenade_launcher, 
	SERIOUS_has_rocket_launcher, SERIOUS_has_flamer, SERIOUS_has_sniper, SERIOUS_has_powergun, SERIOUS_has_cannon};


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
	return ((MEM_ReadUInt(0x80000000) == 0x47334245U && MEM_ReadUInt(0x80000004) == 0x39470001U) || (MEM_ReadUInt(0x80000000) == 0x47334250U && MEM_ReadUInt(0x80000004) == 0x39470000U)); // check game header to see if it matches SERIOUS
}

//==========================================================================
// Purpose: return 1 if game is NTSC and 0 if PAL
//==========================================================================
static uint8_t SERIOUS_Region(void)
{
	return (MEM_ReadUInt(0x80000000) == 0x47334245U && MEM_ReadUInt(0x80000004) == 0x39470001U);
}


//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SERIOUS_Inject(void)
{
	SERIOUS_Weapon_Check();

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	const uint32_t playerbase = (SERIOUS_Region() ? MEM_ReadUInt(SERIOUS_playerbase) : MEM_ReadUInt(SERIOUS_playerbase_PAL));

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

/*
//==========================================================================
// NO LONGER IN USE
//==========================================================================

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
	MEM_WriteInt(0x817d88bc, 0x4082000C); //bne 0x817d88c8 
	MEM_WriteInt(0x817d88c0, 0x921F05F8); //stw r16, 0x05F8(r31) 
	MEM_WriteInt(0x817d88c4, 0x4e800020); //blr

	//check if alt weapon is 1 below main weapon index
	MEM_WriteInt(0x817d88c8, 0x3a3efffe); //subi r17, r30, 2 
	MEM_WriteInt(0x817d88cc, 0x7c118000); //cmpw r17, r16 
	MEM_WriteInt(0x817d88d0, 0x40820008); //bne 0x817d88d8 
	MEM_WriteInt(0x817d88d4, 0x923f05f8); //stw r17, 0x05F8(r31)  
	MEM_WriteInt(0x817d88d8, 0x4e800020); //blr 
}


//==========================================================================
// Purpose: restores original assembly instructions where overwritten
//==========================================================================
void SERIOUS_Code_DeInject(void)
{
	MEM_WriteInt(0x800c5aa0, 0x83df05f8); //lwz r30, 0x05F8 (r31)
    MEM_WriteInt(0x800c5afc, 0x93df05f8); //stw r30, 0x05F8 (r31)
}
*/





//==========================================================================
// Purpose: checks key pressed according to 'KEYS' to set up new weapon to switch to
//==========================================================================
static void SERIOUS_Weapon_Check(void)
{
	uint8_t region = SERIOUS_Region();

	const uint32_t playerbase = (region ? MEM_ReadUInt(SERIOUS_playerbase) : MEM_ReadUInt(SERIOUS_playerbase_PAL));

	if(!playerbase) // if playerbase is invalid
		return;
	

	uint8_t allweapons = region ? MEM_ReadUInt8(SERIOUS_weapons_cheat) : MEM_ReadUInt8(SERIOUS_weapons_cheat_PAL);

	uint32_t currentweapon = MEM_ReadInt(0x817D8860);
	uint32_t previousweapon = MEM_ReadInt(playerbase + 0x05F8);

	uint8_t shotgunhasammo = MEM_ReadUInt8(playerbase + SERIOUS_shells);
	uint16_t uzishasammo = MEM_ReadUInt16(playerbase + SERIOUS_bullets) || MEM_ReadUInt16(playerbase + SERIOUS_reflect_bullets);
	uint16_t minigunhasammo = MEM_ReadUInt16(playerbase + SERIOUS_bullets) || MEM_ReadUInt16(playerbase + SERIOUS_homing_bullets);
	uint8_t rockethasammo = MEM_ReadUInt8(playerbase + SERIOUS_rockets) || MEM_ReadUInt8(playerbase + SERIOUS_homing_rockets) || MEM_ReadUInt8(playerbase + SERIOUS_sonic_rockets);
	uint8_t grenadehasammo = MEM_ReadUInt8(playerbase + SERIOUS_grenades) || MEM_ReadUInt8(playerbase + SERIOUS_mines) || MEM_ReadUInt8(playerbase + SERIOUS_spidermines);
	uint16_t flamerhasammo = MEM_ReadUInt16(playerbase + SERIOUS_napalm) || MEM_ReadUInt16(playerbase + SERIOUS_freeze) || MEM_ReadUInt16(playerbase + SERIOUS_laughing_gas);
	uint8_t sniperhasammo = MEM_ReadUInt8(playerbase + SERIOUS_rifle);
	uint8_t cannonhasammo = MEM_ReadUInt8(playerbase + SERIOUS_cannons);
	uint16_t powergunhasammo = MEM_ReadUInt16(playerbase + SERIOUS_energy);
	uint8_t bombhasammo = MEM_ReadUInt8(playerbase + SERIOUS_bombs);

	uint8_t haschainsaw = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_chainsaw);
	uint8_t haspistol = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_pistol);
	uint8_t hasshotgun = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_shotgun);
	uint8_t hasuzis = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_uzis);
	uint8_t hasminigun = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_minigun);
	uint8_t hasrocketlauncher = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_rocket_launcher);
	uint8_t hasgrenadelauncher = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_grenade_launcher);
	uint8_t hasflamer = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_flamer);
	uint8_t hassniper = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_sniper);
	uint8_t haspowergun = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_powergun);
	uint8_t hascannon = allweapons || MEM_ReadUInt8(playerbase + SERIOUS_has_cannon);


	if((KEYS->K_Chainsaw_Pressed || KEYS->K_Chainsaw_Alt_Pressed) && haschainsaw){
		currentweapon = SERIOUS_chainsaw;
	}

	if((KEYS->K_Pistols_Pressed || KEYS->K_Pistols_Alt_Pressed) && haspistol){
		currentweapon = SERIOUS_pistols;
	}

	if((KEYS->K_Shotgun_Pressed || KEYS->K_Shotgun_Alt_Pressed) && shotgunhasammo && hasshotgun){
		currentweapon = SERIOUS_shotgun;
	}

	if((KEYS->K_Bullets_Pressed || KEYS->K_Bullets_Alt_Pressed) && (uzishasammo || minigunhasammo) && (hasuzis || hasminigun))
		{
		SERIOUS_Assign_Weapon_Group(SERIOUS_minigun, SERIOUS_uzis, &currentweapon, previousweapon);

		if(!(uzishasammo && hasuzis))
		{
			currentweapon = SERIOUS_minigun;
		}
		else if(!(minigunhasammo && hasminigun))
		{
			currentweapon = SERIOUS_uzis;
		}
	}

	if((KEYS->K_Explosives_Pressed || KEYS->K_Explosives_Alt_Pressed) && (rockethasammo || grenadehasammo) && (hasrocketlauncher || hasgrenadelauncher))
	{
		SERIOUS_Assign_Weapon_Group(SERIOUS_rocket_launcher, SERIOUS_grenade_launcher, &currentweapon, previousweapon);

		if(!(rockethasammo && hasrocketlauncher))
		{
			currentweapon = SERIOUS_grenade_launcher;
		}
		else if(!(grenadehasammo && hasgrenadelauncher))
		{
			currentweapon = SERIOUS_rocket_launcher;
		}
	}

	if((KEYS->K_FlameRifle_Pressed || KEYS->K_FlameRifle_Alt_Pressed) && (flamerhasammo || sniperhasammo) && (hasflamer || hassniper)) 
	{
		SERIOUS_Assign_Weapon_Group(SERIOUS_flamethrower, SERIOUS_sniper_rifle, &currentweapon, previousweapon);

		if(!(flamerhasammo && hasflamer))
		{
			currentweapon = SERIOUS_sniper_rifle;
		}
		else if(!(sniperhasammo && hassniper))
		{
			currentweapon = SERIOUS_flamethrower;
		}
	}

	if((KEYS->K_CannonPowergun_Pressed || KEYS->K_CannonPowergun_Alt_Pressed) && (cannonhasammo || powergunhasammo) && (hascannon || haspowergun))
	{
		SERIOUS_Assign_Weapon_Group(SERIOUS_cannon, SERIOUS_sirian_powergun, &currentweapon, previousweapon);

		if(!(cannonhasammo && hascannon))
		{
			currentweapon = SERIOUS_sirian_powergun;
		}
		else if(!(powergunhasammo && haspowergun))
		{
			currentweapon = SERIOUS_cannon;
		}

		
	}

	if((KEYS->K_Bomb_Pressed || KEYS->K_Bomb_Alt_Pressed) && bombhasammo){
		currentweapon = SERIOUS_bomb;
	}
	
	SERIOUS_Update_Weapon(currentweapon); // update memory to swap to selected weapon
}

//==========================================================================
// Purpose: updates necessary regions of memory to contain the new weapon index for gameplay
//==========================================================================
static void SERIOUS_Update_Weapon(uint32_t mainweapon)
{
	uint8_t region = SERIOUS_Region();

	const uint32_t playerbase = (region ? MEM_ReadUInt(SERIOUS_playerbase) : MEM_ReadUInt(SERIOUS_playerbase_PAL));

	if(!playerbase) // if playerbase is invalid
		return;


	uint8_t currentweapon = MEM_ReadInt(0x817D8860);

	MEM_WriteInt(playerbase + 0x05F8, mainweapon);

	MEM_WriteInt(0x817D8860, mainweapon);

	if(currentweapon != MEM_ReadInt(0x817D8860))
	{
		// force weapon switch
		region ? MEM_WriteInt(SERIOUS_weapon_switch_first, 0x00000001) : MEM_WriteInt(SERIOUS_weapon_switch_first_PAL, 0x00000001);
		region ? MEM_WriteInt(SERIOUS_weapon_switch_second, 0x00000001) : MEM_WriteInt(SERIOUS_weapon_switch_second_PAL, 0x00000001);
	}
}

//==========================================================================
// Purpose: checks the player's current weapon to determine which weapon in the category is to be selected
//==========================================================================
static void SERIOUS_Assign_Weapon_Group(uint32_t first, uint32_t second, uint32_t* mainweapon, uint32_t previousweapon)
{	
	if (previousweapon == first || *mainweapon == first)
	{
		*mainweapon = second;
	}
	else
	{
		*mainweapon = first;
	}
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