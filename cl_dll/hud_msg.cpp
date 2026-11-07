/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "event_api.h"

#include "particleman.h"
extern IParticleMan* g_pParticleMan;

extern BEAM* pBeam;
extern BEAM* pBeam2;


/// USER-DEFINED SERVER MESSAGE HANDLERS

bool CHud::MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	ASSERT(iSize == 0);

	// clear all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->Reset();
		pList = pList->pNext;
	}

	//Reset weapon bits.
	m_iWeaponBits = 0ULL;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	// PS2HLU
	gHUD.HudColor = 0x00FFA000;

	return true;
}

void CAM_ToFirstPerson();

void CHud::MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	// prepare all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->InitHUDData();
		pList = pList->pNext;
	}


	//TODO: needs to be called on every map change, not just when starting a new game
	if (g_pParticleMan)
		g_pParticleMan->ResetParticles();

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
}


bool CHud::MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	//Note: this user message could be updated to include multiple gamemodes, so make sure this checks for game mode 1
	//See CHalfLifeTeamplay::UpdateGameMode
	//TODO: define game mode constants
	m_Teamplay = READ_BYTE() == 1;

#ifdef STEAM_RICH_PRESENCE
	if (m_Teamplay)
		gEngfuncs.pfnClientCmd("richpresence_gamemode Teamplay\n");
	else
		gEngfuncs.pfnClientCmd("richpresence_gamemode\n"); // reset

	gEngfuncs.pfnClientCmd("richpresence_update\n");
#endif

	return true;
}


bool CHud::MsgFunc_Damage(const char* pszName, int iSize, void* pbuf)
{
	int armor, blood;
	Vector from;
	int i;
	float count;

	BEGIN_READ(pbuf, iSize);
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return true;
}

bool CHud::MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iConcussionEffect = READ_BYTE();
	if (0 != m_iConcussionEffect)
	{
		int r, g, b;
		UnpackRGB(r, g, b, gHUD.HudColor);
		this->m_StatusIcons.EnableIcon("dmg_concuss", r, g, b);
	}
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return true;
}

bool CHud::MsgFunc_Weapons(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const std::uint64_t lowerBits = READ_LONG();
	const std::uint64_t upperBits = READ_LONG();

	m_iWeaponBits = (lowerBits & 0XFFFFFFFF) | ((upperBits & 0XFFFFFFFF) << 32ULL);

	return true;
}

// PS2HLU
bool CHud::MsgFunc_HudColor(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	// This is possibly the stupidest solution I could come
	// up with to avoid rewriting all of the FillRGBA code in every HUD element

	gHUD.HudColor = ((READ_BYTE() & 0xFF) << 16) + ((READ_BYTE() & 0xFF) << 8) + (READ_BYTE() & 0xFF);

	return 1;
}

// !!! Update when number of decals changes!!!
constexpr int NUM_DECALS = 41;

bool CHud::MsgFunc_PCHTRestore(const char* pszName, int iSize, void* pbuf)
{
	//gEngfuncs.Con_Printf("Client recieved PC HT Restore message!!!\n");

	int i = 0;
	while (true)
	{
		i++; // start AFTER worldspawn

		cl_entity_t* pEnt = gEngfuncs.GetEntityByIndex(i);

		// TODO: Does this work with empty player slots in multiplayer?
		if (pEnt == nullptr)
			break;

		// Skip sounds with no model origin, as they're
		// ambient_generics and looping ones can't quite
		// easily get restored.
		if (pEnt->model == 0)
			continue;

		// stop all sounds
		// exclude CHAN_AUTO, CHAN_STREAM, networked voice & CHAN_BOT
		for (int j = 1; j < 7; j++)
		{
			// skip CHAN_STREAM
			if (j == 5)
				continue;

			gEngfuncs.pEventAPI->EV_StopAllSounds(i, j);
		}
	}

	// Only removes client side decals (?).
	// Server-side ones (eg. infodecal) stays behind, this only
	// becomes a problem on maps that use trigger-able decals, but
	// there's nothing that can be done about that.
	// TODO: consider creating a decal tracker for client-side decals
	for (int j = 0; j < NUM_DECALS; j++)
		gEngfuncs.pEfxAPI->R_DecalRemoveAll(j); // really just an index into decal list
		//gEngfuncs.pEfxAPI->R_DecalRemoveAll(gEngfuncs.pEfxAPI->Draw_DecalIndex(j));


	return true;
}
