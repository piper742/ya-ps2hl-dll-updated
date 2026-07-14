#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"

#include	"skill.h"
#include	"game.h"
#include	"items.h"
#include	"voice_gamemgr.h"
#include	"hltv.h"
#include	"gamerules.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL bool	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgServerName;
extern bool g_teamplay;
extern int gmsgHudColor;
//static CMultiplayGameMgrHelper g_GameMgrHelper;

// TODO:
// Refactor code

CHalfLifeCoop::CHalfLifeCoop()
{
	//g_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);

	RefreshSkillData();
	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if (IS_DEDICATED_SERVER())
	{
		// dedicated server
		char *servercfgfile = (char *)CVAR_GET_STRING("servercfgfile");

		if (servercfgfile && servercfgfile[0])
		{
			char szCommand[256];

			ALERT(at_console, "Executing dedicated server config file\n");
			sprintf(szCommand, "exec %s\n", servercfgfile);
			SERVER_COMMAND(szCommand);
		}
	}
	else
	{
		// listen server
		char *lservercfgfile = (char *)CVAR_GET_STRING("lservercfgfile");

		if (lservercfgfile && lservercfgfile[0])
		{
			char szCommand[256];

			ALERT(at_console, "Executing listen server config file\n");
			sprintf(szCommand, "exec %s\n", lservercfgfile);
			SERVER_COMMAND(szCommand);
		}
	}
}

bool CHalfLifeCoop::ClientCommand(CBasePlayer *pPlayer, const char *pcmd)
{

	return CGameRules::ClientCommand(pPlayer, pcmd);
}

//=========================================================
//=========================================================
void CHalfLifeCoop::RefreshSkillData(void)
{
	// Set the skill level to normal like in
	// the ps2 version
	CVAR_SET_STRING("skill", "2");
	// load all default values
	CGameRules::RefreshSkillData();

	// override some values for multiplay.
	gSkillData.healthchargerCapacity = 40;
	gSkillData.suitchargerCapacity = 50;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = 20; // fewer pellets in deathmatch

	gSkillData.plrDmgHornet = 10;

	/*
		// suitcharger
	gSkillData.suitchargerCapacity = 30;

	// Crowbar whack
	gSkillData.plrDmgCrowbar = 25;

	// Glock Round
	gSkillData.plrDmg9MM = 12;

	// 357 Round
	gSkillData.plrDmg357 = 40;

	// MP5 Round
	gSkillData.plrDmgMP5 = 12;

	// M203 grenade
	gSkillData.plrDmgM203Grenade = 100;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = 20;// fewer pellets in deathmatch

	// Crossbow
	gSkillData.plrDmgCrossbowClient = 20;

	// RPG
	gSkillData.plrDmgRPG = 120;

	// Egon
	gSkillData.plrDmgEgonWide = 20;
	gSkillData.plrDmgEgonNarrow = 10;

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = 100;

	// Satchel Charge
	gSkillData.plrDmgSatchel = 120;

	// Tripmine
	gSkillData.plrDmgTripmine = 150;

	// hornet
	gSkillData.plrDmgHornet = 10;
	*/
}

extern cvar_t timeleft, fragsleft;

extern cvar_t mp_chattime;

//=========================================================
//=========================================================
void CHalfLifeCoop::Think(void)
{

	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if (g_fGameOver)   // someone else quit the game already
	{
		// bounds check
		int time = (int)CVAR_GET_FLOAT("mp_chattime");
		if (time < 1)
			CVAR_SET_STRING("mp_chattime", "1");

		return;
	}

	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;

	time_remaining = (int)(flTimeLimit ? (flTimeLimit - gpGlobals->time) : 0);

	if (flTimeLimit != 0 && gpGlobals->time >= flTimeLimit)
	{
		GoToIntermission();
		return;
	}

	if (flFragLimit)
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex(i);

			if (pPlayer && pPlayer->pev->frags >= flFragLimit)
			{
				GoToIntermission();
				return;
			}


			if (pPlayer)
			{
				remain = flFragLimit - pPlayer->pev->frags;
				if (remain < bestfrags)
				{
					bestfrags = remain;
				}
			}

		}
		frags_remaining = bestfrags;
	}

	// Updates when frags change
	if (frags_remaining != last_frags)
	{
		g_engfuncs.pfnCvar_DirectSet(&fragsleft, UTIL_VarArgs("%i", frags_remaining));
	}

	// Updates once per second
	if (timeleft.value != last_time)
	{
		g_engfuncs.pfnCvar_DirectSet(&timeleft, UTIL_VarArgs("%i", time_remaining));
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}


//=========================================================
//=========================================================
bool CHalfLifeCoop::IsMultiplayer(void)
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeCoop::IsDeathmatch(void)
{
	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeCoop::IsCoOp(void)
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeCoop::FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{
	if (!pWeapon->CanDeploy())
	{
		// that weapon can't deploy anyway.
		return false;
	}

	if (!pPlayer->m_pActiveItem)
	{
		// player doesn't have an active item!
		return true;
	}

	if (!pPlayer->m_pActiveItem->CanHolster())
	{
		// can't put away the active item.
		return false;
	}

	if (pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight())
	{
		return true;
	}

	return false;
}

bool CHalfLifeCoop::GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon)
{

	CBasePlayerItem *pCheck;
	CBasePlayerItem *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	if (!pCurrentWeapon->CanHolster())
	{
		// can't put this gun away right now, so can't switch.
		return false;
	}

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pCheck = pPlayer->m_rgpPlayerItems[i];

		while (pCheck)
		{
			if (pCheck->iWeight() > -1 && pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon)
			{
				// this weapon is from the same category. 
				if (pCheck->CanDeploy())
				{
					if (pPlayer->SwitchWeapon(pCheck))
					{
						return true;
					}
				}
			}
			else if (pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon)// don't reselect the weapon we're trying to get rid of
			{
				//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				if (pCheck->CanDeploy())
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}

			pCheck = pCheck->m_pNext;
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 

	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	if (!pBest)
	{
		return false;
	}

	pPlayer->SwitchWeapon(pBest);

	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeCoop::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	return true;
}

extern int gmsgSayText;
extern int gmsgGameMode;

void CHalfLifeCoop::UpdateGameMode(CBasePlayer *pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pPlayer->edict());
	WRITE_BYTE(0);  // game mode none
	MESSAGE_END();
}

void CHalfLifeCoop::InitHUD(CBasePlayer *pl)
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s has joined the game\n",
		(pl->pev->netname && STRING(pl->pev->netname)[0] != 0) ? STRING(pl->pev->netname) : "unconnected"));

	// team match?
	if (g_teamplay)
	{
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" entered the game\n",
			STRING(pl->pev->netname),
			GETPLAYERUSERID(pl->edict()),
			GETPLAYERAUTHID(pl->edict()),
			g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pl->edict()), "model"));
	}
	else
	{
		UTIL_LogPrintf("\"%s<%i><%s><%i>\" entered the game\n",
			STRING(pl->pev->netname),
			GETPLAYERUSERID(pl->edict()),
			GETPLAYERAUTHID(pl->edict()),
			GETPLAYERUSERID(pl->edict()));
	}

	UpdateGameMode(pl);

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pl->edict());
	WRITE_BYTE(ENTINDEX(pl->edict()));
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_BYTE(0);
	WRITE_SHORT(0);
	MESSAGE_END();

	SendMOTDToClient(pl->edict());

	// loop through all active players and send their score info to the new client
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (plr)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pl->edict());
			WRITE_BYTE(i);	// client number
			WRITE_SHORT(plr->pev->frags);
			WRITE_SHORT(plr->m_iDeaths);
			WRITE_SHORT(0);
			WRITE_SHORT(GetTeamIndex(plr->m_szTeamName) + 1);
			WRITE_BYTE(plr->wpn_accuracy);
			WRITE_SHORT(plr->p_dmgTaken);
			MESSAGE_END();
		}
	}

	if (g_fGameOver)
	{
		MESSAGE_BEGIN(MSG_ONE, SVC_INTERMISSION, NULL, pl->edict());
		MESSAGE_END();
	}

	// Set player hud color
	if (pl->m_decayIndex == 1)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHudColor, NULL, pl->edict());
		WRITE_BYTE(160);
		WRITE_BYTE(160);
		WRITE_BYTE(192);
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHudColor, NULL, pl->edict());
		WRITE_BYTE(255);
		WRITE_BYTE(128);
		WRITE_BYTE(64);
		MESSAGE_END();
	}
}

//=========================================================
//=========================================================
void CHalfLifeCoop::ClientDisconnected(edict_t *pClient)
{
	if (pClient)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pClient);

		if (pPlayer)
		{
			FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

			// team match?
			if (g_teamplay)
			{
				UTIL_LogPrintf("\"%s<%i><%s><%s>\" disconnected\n",
					STRING(pPlayer->pev->netname),
					GETPLAYERUSERID(pPlayer->edict()),
					GETPLAYERAUTHID(pPlayer->edict()),
					g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model"));
			}
			else
			{
				UTIL_LogPrintf("\"%s<%i><%s><%i>\" disconnected\n",
					STRING(pPlayer->pev->netname),
					GETPLAYERUSERID(pPlayer->edict()),
					GETPLAYERAUTHID(pPlayer->edict()),
					GETPLAYERUSERID(pPlayer->edict()));
			}

			pPlayer->RemoveAllItems(true);// destroy all of the players weapons and items
		}
	}
}

//=========================================================
//=========================================================
float CHalfLifeCoop::FlPlayerFallDamage(CBasePlayer *pPlayer)
{
	int iFallDamage = (int)falldamage.value;

	switch (iFallDamage)
	{
	case 1://progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:// fixed
		return 10;
		break;
	}
}

//=========================================================
//=========================================================
bool CHalfLifeCoop::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeCoop::PlayerThink(CBasePlayer *pPlayer)
{
	if (g_fGameOver)
	{
		// check for button presses
		if (pPlayer->m_afButtonPressed & (IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP))
			m_iEndIntermissionButtonHit = true;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

//=========================================================
//=========================================================
void CHalfLifeCoop::PlayerSpawn(CBasePlayer *pPlayer)
{
	bool addDefault;
	CBaseEntity* pWeaponEntity = NULL;

	// Ensure the player switches to the Glock on spawn regardless of setting
	const int originalAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;

	pPlayer->SetHasSuit(true);

	addDefault = false;

	while (pWeaponEntity = UTIL_FindEntityByClassname(pWeaponEntity, "game_player_equip"))
	{
		pWeaponEntity->Touch(pPlayer);
		addDefault = false;
	}

	if (addDefault)
	{
		pPlayer->GiveNamedItem("weapon_crowbar");
		pPlayer->GiveNamedItem("weapon_9mmhandgun");
		pPlayer->GiveAmmo(68, "9mm", _9MM_MAX_CARRY); // 4 full reloads
	}

	pPlayer->m_iAutoWepSwitch = originalAutoWepSwitch;
}

//=========================================================
//=========================================================
bool CHalfLifeCoop::FPlayerCanRespawn(CBasePlayer *pPlayer)
{
	return false;
}

//=========================================================
//=========================================================
float CHalfLifeCoop::FlPlayerSpawnTime(CBasePlayer *pPlayer)
{
	return gpGlobals->time;//now!
}

bool CHalfLifeCoop::AllowAutoTargetCrosshair(void)
{
	return true;
}

void CHalfLifeCoop::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	// TODO: Do we need the SetPlayerModel call here?
	pPlayer->SetPrefsFromUserinfo(infobuffer);
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeCoop::IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled)
{
	return 0;
}


//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeCoop::PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor)
{
	DeathNotice(pVictim, pKiller, pInflictor);

	pVictim->m_iDeaths += 1;


	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance(pKiller);
	if (ktmp && (ktmp->Classify() == CLASS_PLAYER))
		peKiller = (CBasePlayer*)ktmp;

	// PS2HLU
	// Dont lose a frag for killing yourself like in decay
/*
	if (pVictim->pev == pKiller)
	{  // killed self
		pKiller->frags -= 1;
	}
*/
	else if (ktmp && ktmp->IsPlayer())
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->frags += IPointsForKill(peKiller, pVictim);

		FireTargets("game_playerkill", ktmp, ktmp, USE_TOGGLE, 0);
	}
	else
	{  // killed by the world
		pKiller->frags -= 1;
	}

	// update the scores
	// killed scores
	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(pVictim->edict()));
	WRITE_SHORT(pVictim->pev->frags);
	WRITE_SHORT(pVictim->m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(GetTeamIndex(pVictim->m_szTeamName) + 1);
	WRITE_BYTE(pVictim->wpn_accuracy);
	WRITE_SHORT(pVictim->p_dmgTaken);
	MESSAGE_END();

	// killers score, if it's a player
	CBaseEntity *ep = CBaseEntity::Instance(pKiller);
	if (ep && ep->Classify() == CLASS_PLAYER)
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_BYTE(ENTINDEX(PK->edict()));
		WRITE_SHORT(PK->pev->frags);
		WRITE_SHORT(PK->m_iDeaths);
		WRITE_SHORT(0);
		WRITE_SHORT(GetTeamIndex(PK->m_szTeamName) + 1);
		WRITE_BYTE(PK->wpn_accuracy);
		WRITE_SHORT(PK->p_dmgTaken);
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}
#ifndef HLDEMO_BUILD
	if (pVictim->HasNamedPlayerItem("weapon_satchel"))
	{
		DeactivateSatchels(pVictim);
	}
#endif

	// PS2HLU
	// End mission target fire
	if (pVictim->m_decayIndex == 1)
		FireTargets("decay_player1_dead", pVictim, pVictim, USE_TOGGLE, 0);
	if (pVictim->m_decayIndex == 2)
		FireTargets("decay_player2_dead", pVictim, pVictim, USE_TOGGLE, 0);

}

//=========================================================
// Deathnotice. 
//=========================================================
void CHalfLifeCoop::DeathNotice(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor)
{
	// test
	return;

	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity *Killer = CBaseEntity::Instance(pKiller);

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_index = 0;

	// Hack to fix name change
	char *tau = "tau_cannon";
	char *gluon = "gluon gun";

	if (pKiller->flags & FL_CLIENT)
	{
		killer_index = ENTINDEX(ENT(pKiller));

		if (pevInflictor)
		{
			if (pevInflictor == pKiller)
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance(pKiller);

				if (pPlayer->m_pActiveItem)
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING(pevInflictor->classname);  // it's just that easy
			}
		}
	}
	else
	{
		killer_weapon_name = STRING(pevInflictor->classname);
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if (strncmp(killer_weapon_name, "weapon_", 7) == 0)
		killer_weapon_name += 7;
	else if (strncmp(killer_weapon_name, "monster_", 8) == 0)
		killer_weapon_name += 8;
	else if (strncmp(killer_weapon_name, "func_", 5) == 0)
		killer_weapon_name += 5;

	MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
	WRITE_BYTE(killer_index);						// the killer
	WRITE_BYTE(ENTINDEX(pVictim->edict()));		// the victim
	WRITE_STRING(killer_weapon_name);		// what they were killed by (should this be a string?)
	MESSAGE_END();

	// replace the code names with the 'real' names
	if (!strcmp(killer_weapon_name, "egon"))
		killer_weapon_name = gluon;
	else if (!strcmp(killer_weapon_name, "gauss"))
		killer_weapon_name = tau;

	if (pVictim->pev == pKiller)
	{
		// killed self

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" committed suicide with \"%s\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killer_weapon_name);
		}
	}
	else if (pKiller->flags & FL_CLIENT)
	{
		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
				STRING(pKiller->netname),
				GETPLAYERUSERID(ENT(pKiller)),
				GETPLAYERAUTHID(ENT(pKiller)),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(ENT(pKiller)), "model"),
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" killed \"%s<%i><%s><%i>\" with \"%s\"\n",
				STRING(pKiller->netname),
				GETPLAYERUSERID(ENT(pKiller)),
				GETPLAYERAUTHID(ENT(pKiller)),
				GETPLAYERUSERID(ENT(pKiller)),
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killer_weapon_name);
		}
	}
	else
	{
		// killed by the world

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" committed suicide with \"%s\" (world)\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killer_weapon_name);
		}
	}

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);	// command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);	// player killed
	WRITE_SHORT(ENTINDEX(pVictim->edict()));	// index number of primary entity
	if (pevInflictor)
		WRITE_SHORT(ENTINDEX(ENT(pevInflictor)));	// index number of secondary entity
	else
		WRITE_SHORT(ENTINDEX(ENT(pKiller)));	// index number of secondary entity
	WRITE_LONG(7 | DRC_FLAG_DRAMATIC);   // eventflags (priority and flags)
	MESSAGE_END();

	//  Print a standard message
		// TODO: make this go direct to console
	return; // just remove for now
/*
	char	szText[ 128 ];

	if ( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was killed by a monster.\n" );
		return;
	}

	if ( pKiller == pVictim->pev )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " commited suicide.\n" );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		strcpy ( szText, STRING( pKiller->netname ) );

		strcat( szText, " : " );
		strcat( szText, killer_weapon_name );
		strcat( szText, " : " );

		strcat ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, "\n" );
	}
	else if ( FClassnameIs ( pKiller, "worldspawn" ) )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " fell or drowned or something.\n" );
	}
	else if ( pKiller->solid == SOLID_BSP )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was mooshed.\n" );
	}
	else
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " died mysteriously.\n" );
	}

	UTIL_ClientPrintAll( szText );
*/
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeCoop::PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeCoop::FlWeaponRespawnTime(CBasePlayerItem *pWeapon)
{
	if (weaponstay.value > 0)
	{
		// make sure it's only certain weapons
		if (!(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD))
		{
			return gpGlobals->time + 0;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->time + 30;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeCoop::FlWeaponTryRespawn(CBasePlayerItem *pWeapon)
{
	if (pWeapon && pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD))
	{
		if (NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE))
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime(pWeapon);
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeCoop::VecWeaponRespawnSpot(CBasePlayerItem *pWeapon)
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeCoop::WeaponShouldRespawn(CBasePlayerItem *pWeapon)
{

	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CHalfLifeCoop::CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem)
{
	if (weaponstay.value > 0)
	{
		if (pItem->iFlags() & ITEM_FLAG_LIMITINWORLD)
			return CGameRules::CanHavePlayerItem(pPlayer, pItem);

		// check if the player already has this weapon
		for (int i = 0; i < MAX_ITEM_TYPES; i++)
		{
			CBasePlayerItem *it = pPlayer->m_rgpPlayerItems[i];

			while (it != NULL)
			{
				if (it->m_iId == pItem->m_iId)
				{
					return false;
				}

				it = it->m_pNext;
			}
		}
	}

	return CGameRules::CanHavePlayerItem(pPlayer, pItem);
}

//=========================================================
//=========================================================
bool CHalfLifeCoop::CanHaveItem(CBasePlayer *pPlayer, CItem *pItem)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeCoop::PlayerGotItem(CBasePlayer *pPlayer, CItem *pItem)
{
}

//=========================================================
//=========================================================
int CHalfLifeCoop::ItemShouldRespawn(CItem *pItem)
{
		return GR_ITEM_RESPAWN_NO;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeCoop::FlItemRespawnTime(CItem *pItem)
{
	return gpGlobals->time + 30;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeCoop::VecItemRespawnSpot(CItem *pItem)
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeCoop::PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, int iCount)
{
}

//=========================================================
//=========================================================
bool CHalfLifeCoop::IsAllowedToSpawn(CBaseEntity *pEntity)
{
	//	if ( pEntity->pev->flags & FL_MONSTER )
	//		return false;

	return true;
}

//=========================================================
//=========================================================
int CHalfLifeCoop::AmmoShouldRespawn(CBasePlayerAmmo *pAmmo)
{
	if (pAmmo->pev->spawnflags & SF_NORESPAWN)
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeCoop::FlAmmoRespawnTime(CBasePlayerAmmo *pAmmo)
{
	return -1;
}

//=========================================================
//=========================================================
Vector CHalfLifeCoop::VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo)
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeCoop::FlHealthChargerRechargeTime(void)
{
	return -1;
}


float CHalfLifeCoop::FlHEVChargerRechargeTime(void)
{
	return -1;
}

//=========================================================
//=========================================================
int CHalfLifeCoop::DeadPlayerWeapons(CBasePlayer *pPlayer)
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CHalfLifeCoop::DeadPlayerAmmo(CBasePlayer *pPlayer)
{
	return GR_PLR_DROP_AMMO_NO;
}

edict_t *CHalfLifeCoop::GetPlayerSpawnSpot(CBasePlayer *pPlayer)
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot(pPlayer);
	if (IsMultiplayer() && pentSpawnSpot->v.target)
	{
		FireTargets(STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0);
	}

	return pentSpawnSpot;
}


//=========================================================
//=========================================================
int CHalfLifeCoop::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
	// half life deathmatch has only enemies
	return GR_TEAMMATE;
}

bool CHalfLifeCoop::PlayFootstepSounds(CBasePlayer *pl, float fvol)
{
	if (g_footsteps && g_footsteps->value == 0)
		return false;

	if (pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220)
		return true;  // only make step sounds in multiplayer if the player is moving fast enough

	return false;
}

bool CHalfLifeCoop::FAllowFlashlight(void)
{
	return 0;
}

//=========================================================
//=========================================================
bool CHalfLifeCoop::FAllowMonsters(void)
{
	return true;
}

//=========================================================
//======== CHalfLifeCoop private functions ===========
#define INTERMISSION_TIME		6

void CHalfLifeCoop::GoToIntermission(void)
{
	if (g_fGameOver)
		return;  // intermission has already been triggered, so ignore.

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	// bounds check
	int time = (int)CVAR_GET_FLOAT("mp_chattime");
	if (time < 1)
		CVAR_SET_STRING("mp_chattime", "1");

	g_fGameOver = true;
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[32];
	int  minplayers, maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeCoop::ChangeLevel(void)
{
	g_fGameOver = true;

	CHANGE_LEVEL("decaymapselect", NULL);
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeCoop::SendMOTDToClient(edict_t *client)
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME((char *)CVAR_GET_STRING("motdfile"), &length);

	// send the server name
	MESSAGE_BEGIN(MSG_ONE, gmsgServerName, NULL, client);
	WRITE_STRING(CVAR_GET_STRING("hostname"));
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while (pFileList && *pFileList && char_count < MAX_MOTD_LENGTH)
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		if (strlen(pFileList) < MAX_MOTD_CHUNK)
		{
			strcpy(chunk, pFileList);
		}
		else
		{
			strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}

		char_count += strlen(chunk);
		if (char_count < MAX_MOTD_LENGTH)
			pFileList = aFileList + char_count;
		else
			*pFileList = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, NULL, client);
		WRITE_BYTE(*pFileList ? false : true);	// false means there is still more message to come
		WRITE_STRING(chunk);
		MESSAGE_END();
	}

	FREE_FILE(aFileList);
}
