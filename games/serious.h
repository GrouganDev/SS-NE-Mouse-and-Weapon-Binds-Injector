#pragma once

#include "../seriousinputchecker.h"

extern void UPDATE_Keys_Struct(SERIOUSKEYS* keys);
extern void UPDATE_Serious_Keys(SERIOUSKEYS* keys);


extern void CHECK_Key(bool *pressed, bool *last_pressed, SHORT key);
static void SERIOUS_Code_Inject(void);
extern void SERIOUS_Code_DeInject(void);
static void SERIOUS_Weapon_Check(void);
static void SERIOUS_Assign_Weapon_Group(uint32_t first, uint32_t second, uint32_t* mainweapon, uint32_t* altweapon);
static void SERIOUS_Update_Weapon(uint32_t mainweapon, uint32_t altweapon);


extern void SEARCH_Serious_Keys(bool *pressed, bool *last_pressed, bool *alt_pressed, bool *alt_last_pressed, uint8_t ls[]);