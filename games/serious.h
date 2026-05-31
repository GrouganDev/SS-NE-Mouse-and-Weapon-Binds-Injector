#pragma once

#include "../seriousinputchecker.h"
#include <stdarg.h>

extern void UPDATE_Keys_Struct(SERIOUSKEYS* keys);
extern void UPDATE_Serious_Keys(SERIOUSKEYS* keys);


extern void CHECK_Key(uint8_t *pressed, uint8_t *last_pressed, SHORT key);
//static void SERIOUS_Code_Inject(void);
//extern void SERIOUS_Code_DeInject(void);
static void SERIOUS_Weapon_Check(void);
static void SERIOUS_Assign_Weapon_Group(uint32_t first, uint32_t second, uint32_t* mainweapon, uint32_t previousweapon);
static void SERIOUS_Update_Weapon(uint32_t mainweapon, uint32_t playerbase, uint8_t region);
static void SERIOUS_Update_Banner(uint32_t playerbase);


extern void SEARCH_Serious_Keys(uint8_t *pressed, uint8_t *last_pressed, uint8_t *alt_pressed, uint8_t *alt_last_pressed, uint8_t ls[]);