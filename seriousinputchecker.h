#pragma once
#pragma pack(push, 1)

#include "main.h"
#include <windows.h>

typedef struct
{
    uint8_t K_Chainsaw[2];
    uint8_t K_Pistols[2];
    uint8_t K_Shotgun[2];
    uint8_t K_Bullets[2];
    uint8_t K_Explosives[2];
    uint8_t K_FlameRifle[2];
    uint8_t K_CannonPowergun[2];
    uint8_t K_Bomb[2];


    uint8_t K_Chainsaw_Last_Pressed;
	uint8_t K_Chainsaw_Pressed;

    uint8_t K_Chainsaw_Alt_Last_Pressed;
	uint8_t K_Chainsaw_Alt_Pressed;

    uint8_t K_Pistols_Last_Pressed;
	uint8_t K_Pistols_Pressed;

    uint8_t K_Pistols_Alt_Last_Pressed;
	uint8_t K_Pistols_Alt_Pressed;

    uint8_t K_Shotgun_Last_Pressed;
	uint8_t K_Shotgun_Pressed;

    uint8_t K_Shotgun_Alt_Last_Pressed;
	uint8_t K_Shotgun_Alt_Pressed;

    uint8_t K_Bullets_Last_Pressed;
	uint8_t K_Bullets_Pressed;

    uint8_t K_Bullets_Alt_Last_Pressed;
	uint8_t K_Bullets_Alt_Pressed;

    uint8_t K_Explosives_Last_Pressed;
	uint8_t K_Explosives_Pressed;

    uint8_t K_Explosives_Alt_Last_Pressed;
	uint8_t K_Explosives_Alt_Pressed;

    uint8_t K_FlameRifle_Last_Pressed;
	uint8_t K_FlameRifle_Pressed;

    uint8_t K_FlameRifle_Alt_Last_Pressed;
	uint8_t K_FlameRifle_Alt_Pressed;

    uint8_t K_CannonPowergun_Last_Pressed;
	uint8_t K_CannonPowergun_Pressed;

    uint8_t K_CannonPowergun_Alt_Last_Pressed;
	uint8_t K_CannonPowergun_Alt_Pressed;

    uint8_t K_Bomb_Last_Pressed;
	uint8_t K_Bomb_Pressed;

    uint8_t K_Bomb_Alt_Last_Pressed;
	uint8_t K_Bomb_Alt_Pressed;

    uint8_t any_key_just_pressed;
} SERIOUSKEYS;