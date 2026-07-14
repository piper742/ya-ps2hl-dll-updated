/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

#include "demo.h"
#include "demo_api.h"
#include "vgui_ScorePanel.h"

// PS2HLU
#include "event_api.h"
#include "particleman.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;


hud_player_info_t g_PlayerInfoList[MAX_PLAYERS_HUD + 1];	// player info from the engine
extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS_HUD + 1]; // additional player info sent directly to the client dll

class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	void GetPlayerTextColor(int entindex, int color[3]) override
	{
		color[0] = color[1] = color[2] = 255;

		if (entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo) / sizeof(g_PlayerExtraInfo[0]))
		{
			int iTeam = g_PlayerExtraInfo[entindex].teamnumber;

			if (iTeam < 0)
			{
				iTeam = 0;
			}

			iTeam = iTeam % iNumberOfTeamColors;

			color[0] = iTeamColors[iTeam][0];
			color[1] = iTeamColors[iTeam][1];
			color[2] = iTeamColors[iTeam][2];
		}
	}

	void UpdateCursorState() override
	{
		gViewPort->UpdateCursorState();
	}

	int GetAckIconHeight() override
	{
		return ScreenHeight - gHUD.m_iFontHeight * 3 - 6;
	}

	bool CanShowSpeakerLabels() override
	{
		if (gViewPort && gViewPort->m_pScoreBoard)
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;


extern client_sprite_t* GetSpriteList(client_sprite_t* pList, const char* psz, int iRes, int iCount);

extern float IN_GetMouseSensitivity();
cvar_t* cl_lw = NULL;
cvar_t* cl_rollangle = nullptr;
cvar_t* cl_rollspeed = nullptr;
cvar_t* cl_bobtilt = nullptr;
cvar_t* r_decals = nullptr;

// PS2HLU
cvar_t* cl_cjump_style = nullptr;

void ShutdownInput();

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_Logo(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Logo(pszName, iSize, pbuf));
}

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf));
}

int __MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_InitHUD(pszName, iSize, pbuf);
	return 1;
}

int __MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_ViewMode(pszName, iSize, pbuf);
	return 1;
}

int __MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_SetFOV(pszName, iSize, pbuf));
}

int __MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Concuss(pszName, iSize, pbuf));
}

int __MsgFunc_Weapons(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Weapons(pszName, iSize, pbuf));
}

int __MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_GameMode(pszName, iSize, pbuf));
}

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->ShowCommandMenu(gViewPort->m_StandardMenu);
	}
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial()
{
	if (gViewPort)
	{
		gViewPort->InputPlayerSpecial();
	}
}

void __CmdFunc_CloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->InputSignalHideCommandMenu();
	}
}

void __CmdFunc_ForceCloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->HideCommandMenu();
	}
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ValClass(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamNames(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamNames(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Feign(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Feign(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Detpack(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Detpack(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_VGUIMenu(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_VGUIMenu(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_MOTD(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_MOTD(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_BuildSt(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_BuildSt(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_RandomPC(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_RandomPC(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ServerName(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ServerName(pszName, iSize, pbuf));
	return 0;
}
/*
int __MsgFunc_ScoreInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ScoreInfo(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamScore(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamScore(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamInfo(pszName, iSize, pbuf));
	return 0;
}
*/
int __MsgFunc_Spectator(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Spectator(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_SpecFade(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_SpecFade(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ResetFade(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ResetFade(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_AllowSpec(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_AllowSpec(pszName, iSize, pbuf));
	return 0;
}

// PS2HLU
// Rough reimplementation of
// the extremefunnel effect
// Call to it in PS2 HL looks like this:
// 
// Write a tempentity with byte 128
// coord coord coord (origin coords)
// coord coord coord (???)
// short (sprite index)
// short (reverse flag)
// short (maybe another sprite index, not too sure)

// Implementation of client-side particles I used as reference:
// https://github.com/FreeSlave/halflife-featureful/blob/216e030866809350c5e3b2de19dbc48f39283e46/cl_dll/cl_msg_fx.cpp#L497-L567

#define PARTICLE_MINPOS -50
#define PARTICLE_MAXPOS 50

int __MsgFunc_BigFunnel(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const float x = READ_COORD();
	const float y = READ_COORD();
	const float z = READ_COORD();

	// Not sure what these do
	const float velX = READ_COORD();
	const float velY = READ_COORD();
	const float velZ = READ_COORD();

	const int modelIndex = READ_SHORT();

	// Not sure about what this one does either
	// Looks like the part of the code, that wouldve
	// changed whether the funnel was reverse or not
	// Currently going to use it for the last stage of
	// the extremefunnel aka a particle burst
	const int mysteryflag = READ_SHORT();

	// always 1
	const int floatymodelIndex = READ_SHORT(); // ???
	//gEngfuncs.Con_Printf("Particle code thingy ran!\n");

	if (g_pParticleMan)
	{
		// gEngfuncs.Con_Printf("Particle code thingy ran!\n");
		const float clTime = gEngfuncs.GetClientTime();

		model_s* sprite = IEngineStudio.GetModelByIndex(modelIndex);
		model_s* floatything = IEngineStudio.GetModelByIndex(floatymodelIndex);

		if (sprite)
		{
			if (mysteryflag == 1)
			{
				for (int i = 0; i < 250; i++)
				{

					CBaseParticle* particle = g_pParticleMan->CreateParticle(Vector(x, y, z), Vector(0.0f, 0.0f, 0.0f), sprite, 1.0f, 220, "particle");
					if (particle)
					{
						particle->SetLightFlag(LIGHT_NONE);
						particle->SetCullFlag(CULL_PVS);
						particle->SetRenderFlag(RENDER_FACEPLAYER);

						Vector randomparticledir = {0, 0, 0};

						randomparticledir.x = gEngfuncs.pfnRandomLong(0, 100);
						randomparticledir.y = gEngfuncs.pfnRandomLong(-100, 100);
						randomparticledir.z = gEngfuncs.pfnRandomLong(-100, 100);


						particle->m_vVelocity = randomparticledir;
						particle->m_iRendermode = kRenderTransAdd;
						particle->m_vColor = Vector(225, 225, 225);
						particle->m_flGravity = 0;
						particle->m_flSize = 32.0;
						particle->m_flFadeSpeed = 2.0f;
						// the fading in the PS2 version starts 1 seconds
						// before the sprite dies, how do I make it match?
						// maybe fade out manually?
						particle->m_flScaleSpeed = 0;
						// if (flags & 16)
						//{
						/*
						int frameRate = 10;
							particle->m_iFramerate = frameRate > 0 ? frameRate : 10;
							particle->m_iNumFrames = 20;
						*/
						//}

						particle->m_flDieTime = clTime + 4;
					}
				}
			}
			else
			{
				CBaseParticle* particle = g_pParticleMan->CreateParticle(Vector(x, y, z), Vector(0.0f, 0.0f, 0.0f), sprite, 1.0f, 220, "particle");
				if (particle)
				{
					particle->SetLightFlag(LIGHT_NONE);
					particle->SetCullFlag(CULL_PVS);
					particle->SetRenderFlag(RENDER_FACEPLAYER);

					Vector randomparticledir = {0, 0, 0};

					randomparticledir.x = gEngfuncs.pfnRandomLong(0, 100);
					randomparticledir.y = gEngfuncs.pfnRandomLong(-100, 100);
					randomparticledir.z = gEngfuncs.pfnRandomLong(-100, 100);


					particle->m_vVelocity = randomparticledir;
					particle->m_iRendermode = kRenderTransAdd;
					particle->m_flBrightness = 200;
					//particle->m_vColor = Vector(225, 225, 225);
					particle->m_flGravity = 0;
					particle->m_flSize = 32.0;
					particle->m_flFadeSpeed = 2.0f;
					// the fading in the PS2 version starts 1 seconds
					// before the sprite dies, how do I make it match?
					// maybe fade out manually?
					particle->m_flScaleSpeed = 0;
					// if (flags & 16)
					//{
					/*
					int frameRate = 10;
						particle->m_iFramerate = frameRate > 0 ? frameRate : 10;
						particle->m_iNumFrames = 20;
					*/
					//}

					particle->m_flDieTime = clTime + 4;
				}
			}
		}
		
		if (floatything)
		{
			for (int j = 0; j < 8; j++)
			{

				Vector particlepos = {x + gEngfuncs.pfnRandomLong(PARTICLE_MINPOS, PARTICLE_MAXPOS),
					y + gEngfuncs.pfnRandomLong(PARTICLE_MINPOS, PARTICLE_MAXPOS),
					z + gEngfuncs.pfnRandomLong(PARTICLE_MINPOS, 0)};

				CBaseParticle* floatyparticle = g_pParticleMan->CreateParticle(particlepos, Vector(0.0f, 0.0f, 0.0f), floatything, 1.0f, 220, "particle");
				if (floatyparticle)
				{
					floatyparticle->SetLightFlag(LIGHT_NONE);
					floatyparticle->SetCullFlag(CULL_PVS);
					floatyparticle->SetRenderFlag(RENDER_FACEPLAYER);

					Vector randomparticledir = {0, 0, 0};

					// There seems to be a minimum speed limit on these
					// in PS2 HL
					randomparticledir.x = gEngfuncs.pfnRandomLong(0, 100);
					randomparticledir.y = gEngfuncs.pfnRandomLong(-100, 100);
					randomparticledir.z = gEngfuncs.pfnRandomLong(-100, 100);


					floatyparticle->m_vVelocity = randomparticledir;
					floatyparticle->m_iRendermode = kRenderTransAdd;
					floatyparticle->m_vColor = Vector(0, 244, 0); // these are pure green
					floatyparticle->m_flGravity = 0;
					floatyparticle->m_flSize = 1.0;
					floatyparticle->m_flScaleSpeed = 0;

					randomparticledir = {0, 0, 0};
					floatyparticle->m_flDieTime = clTime + 2;
				}
			}
		}
	}
	
	// Do we return 1 or 0?
	// TODO: Figure this out
	return 1;
}

// PS2HLU
int __MsgFunc_HudColor(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_HudColor(pszName, iSize, pbuf));
}

// This is called every time the DLL is loaded
void CHud::Init()
{
	HOOK_MESSAGE(Logo);
	HOOK_MESSAGE(ResetHUD);
	HOOK_MESSAGE(GameMode);
	HOOK_MESSAGE(InitHUD);
	HOOK_MESSAGE(ViewMode);
	HOOK_MESSAGE(SetFOV);
	HOOK_MESSAGE(Concuss);
	HOOK_MESSAGE(Weapons);

	// TFFree CommandMenu
	HOOK_COMMAND("+commandmenu", OpenCommandMenu);
	HOOK_COMMAND("-commandmenu", CloseCommandMenu);
	HOOK_COMMAND("ForceCloseCommandMenu", ForceCloseCommandMenu);
	HOOK_COMMAND("special", InputPlayerSpecial);

	HOOK_MESSAGE(ValClass);
	HOOK_MESSAGE(TeamNames);
	HOOK_MESSAGE(Feign);
	HOOK_MESSAGE(Detpack);
	HOOK_MESSAGE(MOTD);
	HOOK_MESSAGE(BuildSt);
	HOOK_MESSAGE(RandomPC);
	HOOK_MESSAGE(ServerName);
	/*
	HOOK_MESSAGE(ScoreInfo);
	HOOK_MESSAGE(TeamScore);
	HOOK_MESSAGE(TeamInfo);
	*/

	// PS2HLU
	// Removed from here,
	// beacuse scoreboard.cpp hooks the messages
	/*
	HOOK_MESSAGE( ScoreInfo );
	HOOK_MESSAGE( TeamScore );
	HOOK_MESSAGE( TeamInfo );
	*/
	//HOOK_MESSAGE( Accuracy );
	HOOK_MESSAGE(BigFunnel);
	HOOK_MESSAGE(HudColor);

	HOOK_MESSAGE(Spectator);
	HOOK_MESSAGE(AllowSpec);

	HOOK_MESSAGE(SpecFade);
	HOOK_MESSAGE(ResetFade);

	// VGUI Menus
	HOOK_MESSAGE(VGUIMenu);

	CVAR_CREATE("hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO); // controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE("hud_takesshots", "0", FCVAR_ARCHIVE);					   // controls whether or not to automatically take screenshots at the end of a round


	m_iLogo = 0;
	m_iFOV = 0;

	CVAR_CREATE("zoom_sensitivity_ratio", "1.2", FCVAR_ARCHIVE);
	CVAR_CREATE("cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO);
	default_fov = CVAR_CREATE("default_fov", "90", FCVAR_ARCHIVE);
	m_pCvarStealMouse = CVAR_CREATE("hud_capturemouse", "1", FCVAR_ARCHIVE);
	m_pCvarDraw = CVAR_CREATE("hud_draw", "1", FCVAR_ARCHIVE);
	cl_lw = gEngfuncs.pfnGetCvarPointer("cl_lw");
	cl_rollangle = CVAR_CREATE("cl_rollangle", "2.0", FCVAR_ARCHIVE);
	cl_rollspeed = CVAR_CREATE("cl_rollspeed", "200", FCVAR_ARCHIVE);
	cl_bobtilt = CVAR_CREATE("cl_bobtilt", "0", FCVAR_ARCHIVE);
	r_decals = gEngfuncs.pfnGetCvarPointer("r_decals");

	// PS2HLU
	m_pCvarCrosshair = gEngfuncs.pfnGetCvarPointer("crosshair");
	cl_cjump_style = CVAR_CREATE("cl_cjump_style", "1", FCVAR_ARCHIVE | FCVAR_USERINFO);
	//CVAR_CREATE("cl_crouch_toggle", "0", FCVAR_ARCHIVE | FCVAR_USERINFO); // TODO

	m_pSpriteList = NULL;

	// Clear any old HUD list
	if (m_pHudList)
	{
		HUDLIST* pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);

	// PS2HL
	m_HudMode.Init();
	m_HudLock.Init();
	
	// PS2HLU
	// WON/Opfor/PS2 Scoreboard
	m_Scoreboard.Init();


	m_Menu.Init();

	MsgFunc_ResetHUD(0, 0, NULL);

#ifdef STEAM_RICH_PRESENCE
	gEngfuncs.pfnClientCmd("richpresence_gamemode\n"); // reset
	gEngfuncs.pfnClientCmd("richpresence_update\n");
#endif
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud::~CHud()
{
	delete[] m_rghSprites;
	delete[] m_rgrcRects;
	delete[] m_rgszSpriteNames;

	if (m_pHudList)
	{
		HUDLIST* pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rghSprites[] array
// returns 0 if sprite not found
int CHud::GetSpriteIndex(const char* SpriteName)
{
	// look through the loaded sprite name list for SpriteName
	for (int i = 0; i < m_iSpriteCount; i++)
	{
		if (strncmp(SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH) == 0)
			return i;
	}

	return -1; // invalid sprite
}

void CHud::VidInit()
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	// ----------
	// Load Sprites
	// ---------
	//	m_hsprFont = LoadSprite("sprites/%d_font.spr");

	m_hsprLogo = 0;
	m_hsprCursor = 0;

	if (ScreenWidth > 2560 && ScreenHeight > 1600)
		m_iRes = 2560;
	else if (ScreenWidth >= 1280 && ScreenHeight > 720)
		m_iRes = 1280;
	else if (ScreenWidth >= 640)
		m_iRes = 640;
	else
		m_iRes = 320;

	// Only load this once
	if (!m_pSpriteList)
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("sprites/hud.txt", &m_iSpriteCountAllRes);

		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t* p = m_pSpriteList;
			int j;
			for (j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
			m_rghSprites = new HSPRITE[m_iSpriteCount];
			m_rgrcRects = new Rect[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for (j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
				{
					char sz[256];
					sprintf(sz, "sprites/%s.spr", p->szSprite);
					m_rghSprites[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					strncpy(&m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH);

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t* p = m_pSpriteList;
		int index = 0;
		for (int j = 0; j < m_iSpriteCountAllRes; j++)
		{
			if (p->iRes == m_iRes)
			{
				char sz[256];
				sprintf(sz, "sprites/%s.spr", p->szSprite);
				m_rghSprites[index] = SPR_Load(sz);
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex("number_0");

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();
	GetClientVoiceMgr()->VidInit();

	// PS2HL
	m_HudMode.VidInit();
	m_HudLock.VidInit();

	// PS2HLU
	// WON/Opfor/PS2 Scoreboard
	m_Scoreboard.VidInit();
}

bool CHud::MsgFunc_Logo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	// update Train data
	m_iLogo = READ_BYTE();

	return true;
}

float g_lastFOV = 0.0;

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase(const char* in, char* out)
{
	int len, start, end;

	len = strlen(in);

	// scan backward for '.'
	end = len - 1;
	while (0 != end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.') // no '.', copy to end
		end = len - 1;
	else
		end--; // Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len - 1;
	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (in[start] != '/' && in[start] != '\\')
		start = 0;
	else
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy(out, &in[start], len);
	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
bool HUD_IsGame(const char* game)
{
	const char* gamedir;
	char gd[1024];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if (gamedir && '\0' != gamedir[0])
	{
		COM_FileBase(gamedir, gd);
		if (!stricmp(gd, game))
			return true;
	}
	return false;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV()
{
	if (0 != gEngfuncs.pDemoAPI->IsRecording())
	{
		// Write it
		int i = 0;
		unsigned char buf[100];

		// Active
		*(float*)&buf[i] = g_lastFOV;
		i += sizeof(float);

		Demo_WriteBuffer(TYPE_ZOOM, i, buf);
	}

	if (0 != gEngfuncs.pDemoAPI->IsPlayingback())
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

bool CHud::MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT("default_fov");

	//Weapon prediction already takes care of changing the fog. ( g_lastFOV ).
	//But it doesn't restore correctly so this still needs to be used
	/*
	if ( cl_lw && cl_lw->value )
		return 1;
		*/

	g_lastFOV = newfov;

	if (newfov == 0)
	{
		m_iFOV = def_fov;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if (m_iFOV == def_fov)
	{
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = IN_GetMouseSensitivity() * ((float)newfov / (float)def_fov) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	return true;
}


void CHud::AddHudElem(CHudBase* phudelem)
{
	HUDLIST *pdl, *ptemp;

	//phudelem->Think();

	if (!phudelem)
		return;

	pdl = (HUDLIST*)malloc(sizeof(HUDLIST));
	if (!pdl)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity()
{
	return m_flMouseSensitivity;
}
