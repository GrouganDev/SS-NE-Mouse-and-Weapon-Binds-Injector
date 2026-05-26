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

	//printf("%d", currentweapon);

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

void SERIOUS_Code_DeInject(void)
{
	MEM_WriteInt(0x800c5aa0, 0x83df05f8);
    MEM_WriteInt(0x800c5afc, 0x93df05f8);
}


static void SERIOUS_Weapon_Check(void)
{
	uint32_t currentweapon = 0xFFFFFFFF;
	uint32_t altweapon = 0xFFFFFFFF;

	if(KEYS->K_1_Pressed){
		currentweapon = 0x00000000;
	}
	if(KEYS->K_2_Pressed){
		currentweapon = 0x00000001;
	}
	if(KEYS->K_3_Pressed){
		currentweapon = 0x00000003;
	}
	if(KEYS->K_4_Pressed){
		SERIOUS_Assign_Weapon_Group(0x00000002, 0x00000004, &currentweapon, &altweapon);
	}
	if(KEYS->K_F_Pressed){
		SERIOUS_Assign_Weapon_Group(0x00000006, 0x00000005, &currentweapon, &altweapon);
	}
	if(KEYS->K_X_Pressed || KEYS->M_XBUTTON2_Pressed){
		SERIOUS_Assign_Weapon_Group(0x00000007, 0x00000008, &currentweapon, &altweapon);
	}
	if(KEYS->K_C_Pressed || KEYS->M_XBUTTON1_Pressed){
		SERIOUS_Assign_Weapon_Group(0x0000000A, 0x00000009, &currentweapon, &altweapon);
	}
	if(KEYS->K_LALT_Pressed){
		currentweapon = 0x0000000B;
	}

	if(currentweapon != 0xFFFFFFFF){
		SERIOUS_Update_Weapon(currentweapon, altweapon);
	}
}

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


static void SERIOUS_Update_Weapon(uint32_t mainweapon, uint32_t altweapon)
{
	uint32_t clampedweaponind = (((int32_t)mainweapon - 1) % 12);

	MEM_WriteInt(0x817D885C, clampedweaponind);

	MEM_WriteInt(0x817D8860, mainweapon);

	MEM_WriteInt(0x817D8868, altweapon);
}






void UPDATE_Keys_Struct(SERIOUSKEYS* keys){
	KEYS = keys;
}



void UPDATE_Serious_Keys(SERIOUSKEYS* keys)
{
    CHECK_Key(&keys->K_1_Pressed, &keys->K_1_Last_Pressed, K_1);

    CHECK_Key(&keys->K_2_Pressed, &keys->K_2_Last_Pressed, K_2);

    CHECK_Key(&keys->K_3_Pressed, &keys->K_3_Last_Pressed, K_3);

    CHECK_Key(&keys->K_4_Pressed, &keys->K_4_Last_Pressed, K_4);
    
    CHECK_Key(&keys->K_F_Pressed, &keys->K_F_Last_Pressed, K_F);

    CHECK_Key(&keys->K_X_Pressed, &keys->K_X_Last_Pressed, K_X);
    
    CHECK_Key(&keys->K_C_Pressed, &keys->K_C_Last_Pressed, K_C);

    CHECK_Key(&keys->K_LALT_Pressed, &keys->K_LALT_Last_Pressed, K_LALT);

    CHECK_Key(&keys->M_XBUTTON1_Pressed, &keys->M_XBUTTON1_Last_Pressed, M_XBUTTON1);

    CHECK_Key(&keys->M_XBUTTON2_Pressed, &keys->M_XBUTTON2_Last_Pressed, M_XBUTTON2);
}


void CHECK_Key(bool *pressed, bool *last_pressed, SHORT key){
    if(key && !*last_pressed){
        *last_pressed = true;
        *pressed = true;
    }
    else if(!key)
    {
        *last_pressed = false;
        *pressed = false;
    }
    else if(*last_pressed)
    {
        *pressed = false;
    }
}