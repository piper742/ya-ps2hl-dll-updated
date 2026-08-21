// PS2HLU
// Bot code based on Botmans code

#include "extdll.h"
#include "util.h"
#include "ps2hlu_bot_decay.h"
#include "ps2hl_dbg.h"

extern int gmsgHealth;
extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgHudColor;
extern DLL_GLOBAL unsigned int g_ulModelIndexPlayer;
// Set in combat.cpp.  Used to pass the damage inflictor for death messages.
extern entvars_t *g_pevLastInflictor;

inline edict_t *CREATE_FAKE_CLIENT(const char *netname)
{
	return (*g_engfuncs.pfnCreateFakeClient)(netname);
}

void BotCreate()
{
	CDecayBot myDecayBot;
	myDecayBot.CreateBot();
}

void DoBotSwap()
{
	CDecayBot myDecayBot;
	myDecayBot.SwapBotWithPlayer();
}

void CDecayBot::CreateBot(void)
{
		edict_t *BotEnt = CREATE_FAKE_CLIENT("Colette");

	if (FNullEnt(BotEnt)) {
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "Max. Players reached.  Can't create AI Player!\n");

		if (IS_DEDICATED_SERVER())
			printf("Max. Players reached.  Can't create AI Player!\n");
		return;
	}
	else
	{
		char ptr[128];  // allocate space for message from ClientConnect
		CDecayBot *BotClass;
		char *infobuffer;
		int clientIndex;

		BotClass = GetClassPtr((CDecayBot *)VARS(BotEnt));
		infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(BotClass->edict());
		clientIndex = BotClass->entindex();

		g_engfuncs.pfnSetClientKeyValue(clientIndex, infobuffer, "model", "ginacol");

		ClientConnect(BotClass->edict(), "Colette", "127.0.0.1", ptr);
		DispatchSpawn(BotClass->edict());

		return;
	}
}

//START BOT
CBasePlayer *CBasePlayerByIndex(int playerIndex)
{
	CBasePlayer *pPlayer = NULL;
	entvars_t *pev;

	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		edict_t *pPlayerEdict = INDEXENT(playerIndex);
		if (pPlayerEdict && !pPlayerEdict->free &&
			(pPlayerEdict->v.flags & FL_FAKECLIENT || pPlayerEdict->v.flags & FL_CLIENT)) //fake
		{
			pev = &pPlayerEdict->v;
			pPlayer = GetClassPtr((CBasePlayer *)pev);
		}
	}

	return pPlayer;
}
//END BOT


void EXPORT CDecayBot::SwapBotWithPlayer()
{

	CBaseEntity *pPlayer = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(1));
	//Vector BotOrigin;
	//Vector PlayerOrigin;
	Vector BotAngles;
	Vector PlayerAngles;
	Vector BotMins;
	Vector PlayerMins;
	Vector BotMaxs;
	Vector PlayerMaxs;
	Vector BotVAngles;
	Vector PlayerVAngles;
	Vector BotVelocity;
	Vector PlayerVelocity;
	int PlayerSkin=0;
	int BotSkin=0;
	int PlayerBody = 0;
	int BotBody = 0;
	/*
	int PlayerButton = 0;
	int BotButton = 0;
	*/


	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer2 = CBasePlayerByIndex(i);

		if (!pPlayer2)  // if invalid then continue with next index...
			continue;

		// check if this is a FAKECLIENT (i.e. is it a bot?)
		if (FBitSet(pPlayer2->pev->flags, FL_FAKECLIENT))
		{
			//pPlayer->m_decayIndex = (pPlayer->m_decayIndex == 1) ? 2 : 1;
			//pPlayer2->m_decayIndex = (pPlayer2->m_decayIndex == 1) ? 2 : 1;
			//ALERT(at_console, "Player 1 index changed to %d\n", pPlayer->m_decayIndex);
			//ALERT(at_console, "Player 2 index changed to %d\n", pPlayer2->m_decayIndex);
			// Set Data
			//BotOrigin = pPlayer2->pev->origin;
			//PlayerOrigin = pPlayer->pev->origin;
			BotAngles = pPlayer2->pev->angles;
			PlayerAngles = pPlayer->pev->angles;
			PlayerMins = pPlayer->pev->mins;
			BotMins = pPlayer2->pev->mins;
			PlayerMaxs = pPlayer->pev->maxs;
			BotMaxs = pPlayer2->pev->maxs;
			BotVAngles = pPlayer2->pev->v_angle;
			PlayerVAngles = pPlayer->pev->v_angle;
			PlayerSkin = pPlayer->pev->skin;
			BotSkin = pPlayer2->pev->skin;
			PlayerBody = pPlayer->pev->body;
			BotBody = pPlayer2->pev->body;
			/*
			PlayerButton = pPlayer->pev->button;
			BotButton = pPlayer2->pev->button;
			*/
			PlayerVelocity = pPlayer->pev->velocity;
			BotVelocity = pPlayer2->pev->velocity;
			
			Vector tmp = pPlayer->pev->origin;
			Vector tmp2 = pPlayer2->pev->origin;

			//pPlayer->pev->flags &= ~FL_ONGROUND;
			//pPlayer2->pev->flags &= ~FL_ONGROUND;

			// Do the actual switch
			EMIT_SOUND_DYN(ENT(pPlayer->pev), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM);
			UTIL_SetOrigin(pPlayer->pev, tmp2);
			UTIL_SetOrigin(pPlayer2->pev, tmp);
			pPlayer->pev->angles = BotAngles;
			pPlayer2->pev->angles = PlayerAngles;
			pPlayer->pev->v_angle = BotVAngles;
			pPlayer2->pev->v_angle = PlayerVAngles;
			//UTIL_SetSize(pPlayer->pev, BotMins, BotMaxs);
			//UTIL_SetSize(pPlayer2->pev, PlayerMins, PlayerMaxs);
			//pPlayer2->pev->absmax = pPlayer->pev->absmax;
			//pPlayer2->pev->absmin = pPlayer->pev->absmax;
			pPlayer->pev->skin = BotSkin;
			pPlayer2->pev->skin = PlayerSkin;
			pPlayer->pev->body = BotBody;
			pPlayer2->pev->body = PlayerBody;
			pPlayer->pev->velocity = BotVelocity;
			pPlayer2->pev->velocity = PlayerVelocity;

			

			if (pPlayer->m_decayIndex == 1)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgHudColor, NULL, pPlayer->edict());
				WRITE_BYTE(160);
				WRITE_BYTE(160);
				WRITE_BYTE(192);
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgHudColor, NULL, pPlayer->edict());
				WRITE_BYTE(255);
				WRITE_BYTE(128);
				WRITE_BYTE(64);
				MESSAGE_END();
			}
			/*
			pPlayer2->pev->avelocity = pPlayer->pev->avelocity;
			pPlayer2->pev->basevelocity = pPlayer->pev->basevelocity;
			pPlayer2->pev->flFallVelocity = pPlayer->pev->flFallVelocity;
			pPlayer2->pev->clbasevelocity = pPlayer->pev->clbasevelocity;
			*/
			//pPlayer->m_bIsInTrigger = FALSE;
			//pPlayer2->m_bIsInTrigger = FALSE;

			// This doesnt help in getting the bot unstuck from the ground

			//pPlayer->pev->button = pPlayer2->pev->button = 0;
			// fix bot getting stuck in floor
			//if (FBitSet(pPlayer->pev->flags, FL_DUCKING))
			//	pPlayer2->Crouch();
			/*
			if (FBitSet(pPlayer->pev->flags, FL_DUCKING))
			{
				ALERT(at_console, "player is crouched!\n");
				CDecayBot *bot = static_cast<CDecayBot *>(pPlayer2);
				bot->Crouch();
			}*/

			// This is important!!!
			// Make sure bot & player face in the right direction
			// BUGBUG: The engine corrects angles or something
			// Its always weird
			pPlayer2->pev->fixangle = 1;
			pPlayer->pev->fixangle = 1;

			//pPlayer->pev->flags &= ~FL_ONGROUND;
			//pPlayer2->pev->flags &= ~FL_ONGROUND;

			// TODO: Send over health, armor, ammo, and weapons
		}
	}
}

void CDecayBot::Spawn()
{

	CBasePlayer::Spawn();

	pev->flags = FL_CLIENT | FL_FAKECLIENT;

	// DEBUG DEBUG
	pev->flags |= FL_GODMODE;

	SetThink(&CDecayBot::BotThink);

	m_flNextBotThink = gpGlobals->time + g_flBotCommandInterval;
	m_flNextFullBotThink = gpGlobals->time + g_flBotFullThinkInterval;
	m_flPreviousCommandTime = gpGlobals->time;

	m_fIsCrouching = false;
	m_forwardSpeed = 0.0f;
	m_buttonFlags = 0;

	pev->nextthink = gpGlobals->time + 0.1;
}

bool CDecayBot::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	CBaseEntity *attacker = GetClassPtr((CBaseEntity *)pevInflictor);
	if(IsEnemy(attacker))
		m_attacker = static_cast<CBaseMonster *>(attacker);

	return CBasePlayer::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void EXPORT CDecayBot::PlayerDeathThink(void)
{
	CBasePlayer::PlayerDeathThink();
}

void CDecayBot::Killed(entvars_t *pevAttacker, int iGib)
{
	CBasePlayer::Killed(pevAttacker, iGib);
}

void EXPORT CDecayBot::BotThink(void)
{
	/*
	if (!IsInWorld())
	{
		SetTouch(NULL);
		UTIL_Remove(this);
		return;
	}

	if (!IsAlive())
	{
		// if bot is dead, don't take up any space in world (set size to 0)
		UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
		pev->solid = SOLID_NOT;

		return;
	}*/

	//m_forwardSpeed = 0;
	//pev->button = 0;
	//pev->angles.z = 0;
	//pev->angles.y = pev->v_angle.y;
	/*
	StudioFrameAdvance();
	ItemPostFrame();
	*/
	
	// this causes an access violation for some reason, ill just disable it for now
#if 0
	if (/*GetAttacker() != NULL && GetAttacker() != this->GetEnemy() && */m_attacker != NULL)
	{
		//GetAttacker();
		CBaseMonster *m_attacker2 = GetAttacker();

		if (!m_attacker2)
			return;

		pev->angles.x = 0;
		
		Vector enemyorigin = ((m_attacker2->pev->origin + m_attacker2->pev->view_ofs));
		enemyorigin.z -= m_attacker2->pev->size.z * 0.1f;
		enemyorigin = (enemyorigin + ((m_attacker2->pev->size / 2 + m_attacker2->pev->velocity) / 3)) - GetGunPosition();
		pev->v_angle = UTIL_VecToAngles(enemyorigin);
		pev->v_angle.z = 0;
		pev->fixangle = 1;
		PrimaryAttack();
	}
	else
		ClearPrimaryAttack();
#endif

	Vector autoaim = GetAutoaimVector(3.1256671980042f);
	if (m_iOnTarget == 1)
	{
		auto desiredangle = UTIL_VecToAngles(autoaim);

		//auto botPitch = UTIL_AngleDiff(desiredangle.x, pev->angles.x);
		//pev->angles.x = UTIL_AngleMod(pev->angles.x + botPitch);
		pev->angles.x = UTIL_AngleMod(360.0f - desiredangle.x);
		pev->v_angle.y = pev->angles.y = desiredangle.y;

		/*
		if (pev->v_angle.x < -89.0f)
			pev->v_angle.x = -89.0f;
		else if (pev->v_angle.x > 89.0f)
			pev->v_angle.x = 89.0f;
		*/

		pev->angles.z = pev->v_angle.z = 0;

		pev->fixangle = 1;
		if (m_iOnTarget == 1)
			PrimaryAttack();
		else
			ClearPrimaryAttack();
	}
	//pev->angles.x = 0;

	/*
	// DEBUG
	// This code visualizes the bounding box
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer2 = CBasePlayerByIndex(i);

		if (!pPlayer2)  // if invalid then continue with next index...
			continue;

		// check if this is a FAKECLIENT (i.e. is it a bot?)
		if (FBitSet(pPlayer2->pev->flags, FL_FAKECLIENT))
			DBG_RenderBBox(pPlayer2->pev->origin, pPlayer2->pev->mins, pPlayer2->pev->maxs, 10, 10, 10);
	}
	*/

	if (gpGlobals->time >= m_flNextBotThink)
	{
		m_flNextBotThink = gpGlobals->time + g_flBotCommandInterval;

		if (gpGlobals->time >= m_flNextFullBotThink)
		{
			m_flNextFullBotThink = gpGlobals->time + g_flBotFullThinkInterval;
			m_fIsCrouching = false;
			m_buttonFlags = 0;
			m_forwardSpeed = 0.0f;
		}

		//m_forwardSpeed = GetMoveSpeed();
		ExecuteCommand();
		//ALERT(at_console, "bot is thinking! \n");
	}
	pev->nextthink = gpGlobals->time + 0.1;
}


void CDecayBot::ExecuteCommand(void)
{
	byte adjustedMSec;

	// Adjust msec to command time interval
	adjustedMSec = ThrottledMsec();

	// player model is "munged"
	pev->angles = pev->v_angle;
	pev->angles.x /= -3.0;

	// save the command time
	m_flPreviousCommandTime = gpGlobals->time;


	// this doesnt work for some reason, the bot gets stuck in the ground when
	// swapping players when crouched
	if (m_fIsCrouching)
		SetBits(m_buttonFlags, IN_DUCK);

	// Run the command
	(*g_engfuncs.pfnRunPlayerMove)(edict(), pev->v_angle, m_forwardSpeed, 0, 0,
		m_buttonFlags, 0, adjustedMSec);
}

byte CDecayBot::ThrottledMsec(void) const
{
	int iNewMsec;

	// Estimate Msec to use for this command based on time passed from the previous command
	iNewMsec = (int)((gpGlobals->time - m_flPreviousCommandTime) * 1000);
	if (iNewMsec > 255)  // Doh, bots are going to be slower than they should if this happens.
		iNewMsec = 255;		 // Upgrade that CPU or use less bots!

	return (byte)iNewMsec;
}

/*
Vector CDecayBot::GetAutoaimVector(float flDelta)
{
	UTIL_MakeVectors(pev->v_angle + pev->punchangle);

	return gpGlobals->v_forward;
}
*/

void CDecayBot::Crouch(void)
{
	m_fIsCrouching = true;
}

void CDecayBot::PrimaryAttack(void)
{
	SetBits(m_buttonFlags, IN_ATTACK);
}

//--------------------------------------------------------------------------------------------------------------
void CDecayBot::ClearPrimaryAttack(void)
{
	ClearBits(m_buttonFlags, IN_ATTACK);
}

CBaseMonster *CDecayBot::GetAttacker() const
{
	if (m_attacker != NULL && m_attacker->IsAlive())
		return m_attacker;

	return NULL;
}

bool CDecayBot::IsEnemy(CBaseEntity *enemy)
{
	// TODO: better check if it classifies as hostile against the player
	if (enemy != NULL && enemy->IsAlive() /* && enemy->pev->flags & FL_MONSTER *//* && enemy->Classify() == CLASS_ALIEN_MILITARY*/)
	return true;

	return false;
}


