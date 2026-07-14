/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== lights.cpp ========================================================

  spawn and think functions for editor-placed lights

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CLight : public CPointEntity
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	// PS2HLU
	void EXPORT CreateELight();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

private:
	int m_iStyle;
	int m_iszPattern;

	// PS2HLU these aren't saved, seemingly only used for ELIGHTs
	// I guess initializing to zero is correct? Would RAD actually strip
	// lights out with no _light parameter? Am I just creating a future bug?
	unsigned int colors[4] = {0};
};
LINK_ENTITY_TO_CLASS(light, CLight);

TYPEDESCRIPTION CLight::m_SaveData[] =
	{
		DEFINE_FIELD(CLight, m_iStyle, FIELD_INTEGER),
		DEFINE_FIELD(CLight, m_iszPattern, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CLight, CPointEntity);


//
// Cache user-entity-field values until spawn is called.
//
bool CLight::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		pev->angles.x = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "pattern"))
	{
		m_iszPattern = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	// PS2HLU
	else if (FStrEq(pkvd->szKeyName, "_light"))
	{
		//int r = 0, g = 0, b = 0, v = 0;
		const int j = sscanf(pkvd->szValue, "%d %d %d %d\n", &colors[0], &colors[1], &colors[2], &colors[3]);

		if (j == 1)
		{
			colors[1] = colors[2] = colors[0];
		}
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) LIGHT_START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
*/

void CLight::Spawn()
{
	// PS2HLU
	// Entity_light
	// Any light can become an ELIGHT emitter if spawnflag 2 is checked
	if (pev->spawnflags & SF_LIGHT_ELIGHT)
	{
		SetThink(&CLight::CreateELight);
		pev->nextthink = gpGlobals->time + 1.0f;

		// Is this the result of a bizarre optimization
		// by the compiler, or does doing this actually
		// change something? TODO: Investigate
		if (FStringNull(pev->targetname))
			return;
	}

	if (FStringNull(pev->targetname))
	{ // inert light
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	if (m_iStyle >= 32)
	{
		//		CHANGE_METHOD(ENT(pev), em_use, light_use);
		if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
			LIGHT_STYLE(m_iStyle, "a");
		else if (!FStringNull(m_iszPattern))
			LIGHT_STYLE(m_iStyle, (char*)STRING(m_iszPattern));
		else
			LIGHT_STYLE(m_iStyle, "m");
	}
}


void CLight::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (m_iStyle >= 32)
	{
		if (!ShouldToggle(useType, !FBitSet(pev->spawnflags, SF_LIGHT_START_OFF)))
			return;

		if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
		{
			if (!FStringNull(m_iszPattern))
				LIGHT_STYLE(m_iStyle, (char*)STRING(m_iszPattern));
			else
				LIGHT_STYLE(m_iStyle, "m");
			ClearBits(pev->spawnflags, SF_LIGHT_START_OFF);
		}
		else
		{
			LIGHT_STYLE(m_iStyle, "a");
			SetBits(pev->spawnflags, SF_LIGHT_START_OFF);
		}
	}
}

void CLight::CreateELight()
{
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_ELIGHT);
	WRITE_SHORT(0);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(colors[3]);	// TODO: is this correct radius?
	WRITE_BYTE(colors[0]);	// R
	WRITE_BYTE(colors[1]);	// G
	WRITE_BYTE(colors[2]);	// B
	WRITE_BYTE(0);	// life * 10
	WRITE_COORD(0);	// decay
	MESSAGE_END();

	return;
}

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS(light_spot, CLight);

// PS2HLU
// ELIGHT emitter, unused but seemingly used in ht04dampen during the
// resonance reversal sequence according to a prerelease magazine screenshot
//
// Though 

LINK_ENTITY_TO_CLASS(entity_light, CLight);

class CEnvLight : public CLight
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS(light_environment, CEnvLight);

bool CEnvLight::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "_light"))
	{
		int r = 0, g = 0, b = 0, v = 0;
		char szColor[64];
		const int j = sscanf(pkvd->szValue, "%d %d %d %d\n", &r, &g, &b, &v);
		if (j == 1)
		{
			g = b = r;
		}
		else if (j == 4)
		{
			r = r * (v / 255.0f);
			g = g * (v / 255.0f);
			b = b * (v / 255.0f);
		}


		// PS2HLU: subtle change here, can't be seen ingame in PS2 HL
		// simulate qrad direct, ambient,and gamma adjustments, as well as engine scaling
		r = pow(r / 114.0f, 0.5f) * 264.0d;
		g = pow(g / 114.0f, 0.5f) * 264.0d;
		b = pow(b / 114.0f, 0.5f) * 264.0d;

		// PS2HLU
		// This code is present in PS2 HL, but it's
		// broken at an engine level
		// It actually uses the old WON cl_ prefixed cvars

		// sprintf(szColor, "%d", r);
		// CVAR_SET_STRING("sv_skycolor_r", szColor);
		// sprintf(szColor, "%d", g);
		// CVAR_SET_STRING("sv_skycolor_g", szColor);
		// sprintf(szColor, "%d", b);
		// CVAR_SET_STRING("sv_skycolor_b", szColor);

		return true;
	}

	return CLight::KeyValue(pkvd);
}


void CEnvLight::Spawn()
{
	// PS2HLU
	// This code is present in PS2 HL, but it's broken
	// at an engine level

	//char szVector[64];
	//UTIL_MakeAimVectors(pev->angles);

	//sprintf(szVector, "%f", gpGlobals->v_forward.x);
	//CVAR_SET_STRING("sv_skyvec_x", szVector);
	//sprintf(szVector, "%f", gpGlobals->v_forward.y);
	//CVAR_SET_STRING("sv_skyvec_y", szVector);
	//sprintf(szVector, "%f", gpGlobals->v_forward.z);
	//CVAR_SET_STRING("sv_skyvec_z", szVector);

	CLight::Spawn();
}
