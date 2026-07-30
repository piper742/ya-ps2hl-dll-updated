/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// barnacle - stationary ceiling mounted 'fishing' monster
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"

#define BARNACLE_BODY_HEIGHT 44 // how 'tall' the barnacle's model is.
#define BARNACLE_PULL_SPEED 8
#define BARNACLE_KILL_VICTIM_DELAY 5 // how many seconds after pulling prey in to gib them.

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define BARNACLE_AE_PUKEGIB 2

// PS2HLU
// Invisible hull to get the engine to do some calculations for us
class CBarnacleVisHack : public CPointEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_NONE; /* CLASS_BARNACLE */ }
};

void CBarnacleVisHack::Spawn()
{
	Precache();

	// Based off of how CXenHull does something very similar
	pev->classname = MAKE_STRING("pc_barnaclevishack");

	SET_MODEL(ENT(pev), "models/barnacle.mdl");
	// PS2HLU value grabbed from PS2 HL (1024 was subtracted in StudioCheckBBox)
	UTIL_SetSize(pev, Vector(-16, -16, -1056), Vector(16, 16, 0));

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;

	// Match CXenHull behaviour
	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;

	// Can't we do this sooner?
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBarnacleVisHack::Precache()
{
	// This is probably redundant, but account for
	// someone creating a save then this entity getting
	// processed first rather than a barnacle
	PRECACHE_MODEL("models/barnacle.mdl");

	// Reset effects to prevent fireflies, in case someone
	// manages to create a save during our lifetime
	pev->effects = 0;
}

class CBarnacle : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	CBaseEntity* TongueTouchEnt(float* pflLength);
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void EXPORT BarnacleThink();
	void EXPORT WaitTillDead();
	void Killed(entvars_t* pevAttacker, int iGib) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
	void EXPORT ApplyVisHack();

	float m_flAltitude;
	float m_flKillVictimTime;
	int m_cGibs; // barnacle loads up on gibs each time it kills something.
	bool m_fTongueExtended;
	bool m_fLiftingPrey;
	float m_flTongueAdj;

	// PS2HLU flag so we can reapply the hack after level changes and saveloads
	bool appliedVisHack = false;
};
LINK_ENTITY_TO_CLASS(monster_barnacle, CBarnacle);

TYPEDESCRIPTION CBarnacle::m_SaveData[] =
	{
		DEFINE_FIELD(CBarnacle, m_flAltitude, FIELD_FLOAT),
		DEFINE_FIELD(CBarnacle, m_flKillVictimTime, FIELD_TIME),
		DEFINE_FIELD(CBarnacle, m_cGibs, FIELD_INTEGER), // barnacle loads up on gibs each time it kills something.
		DEFINE_FIELD(CBarnacle, m_fTongueExtended, FIELD_BOOLEAN),
		DEFINE_FIELD(CBarnacle, m_fLiftingPrey, FIELD_BOOLEAN),
		DEFINE_FIELD(CBarnacle, m_flTongueAdj, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CBarnacle, CBaseMonster);


//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CBarnacle::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBarnacle::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BARNACLE_AE_PUKEGIB:
		CGib::SpawnRandomGibs(pev, 1, true);
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarnacle::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/barnacle.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, -32), Vector(16, 16, 0));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_AIM;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = EF_INVLIGHT; // take light from the ceiling
	pev->health = 25;
	m_flFieldOfView = 0.5; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flKillVictimTime = 0;
	m_cGibs = 0;
	m_fLiftingPrey = false;
	m_flTongueAdj = -100;

	InitBoneControllers();

	SetActivity(ACT_IDLE);

	SetThink(&CBarnacle::BarnacleThink);
	pev->nextthink = gpGlobals->time + 0.5;

	UTIL_SetOrigin(pev, pev->origin);
}

bool CBarnacle::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if ((bitsDamageType & DMG_CLUB) != 0)
	{
		flDamage = pev->health;
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
//=========================================================
void CBarnacle::BarnacleThink()
{
	CBaseEntity* pTouchEnt;
	CBaseMonster* pVictim;
	float flLength;

	pev->nextthink = gpGlobals->time + 0.1;

	// PS2HLU
	// The hack seems to be getting applied in a pretty timely manner,
	// so just leave here for now
	if (appliedVisHack == 0)
		ApplyVisHack();

	if (m_hEnemy != NULL)
	{
		// barnacle has prey.

		if (!m_hEnemy->IsAlive())
		{
			// someone (maybe even the barnacle) killed the prey. Reset barnacle.
			m_fLiftingPrey = false; // indicate that we're not lifting prey.
			m_hEnemy = NULL;
			return;
		}

		if (m_fLiftingPrey)
		{
			if (m_hEnemy != NULL && m_hEnemy->pev->deadflag != DEAD_NO)
			{
				// crap, someone killed the prey on the way up.
				m_hEnemy = NULL;
				m_fLiftingPrey = false;
				return;
			}

			// still pulling prey.
			Vector vecNewEnemyOrigin = m_hEnemy->pev->origin;
			vecNewEnemyOrigin.x = pev->origin.x;
			vecNewEnemyOrigin.y = pev->origin.y;

			// guess as to where their neck is
			vecNewEnemyOrigin.x -= 6 * cos(m_hEnemy->pev->angles.y * M_PI / 180.0);
			vecNewEnemyOrigin.y -= 6 * sin(m_hEnemy->pev->angles.y * M_PI / 180.0);

			m_flAltitude -= BARNACLE_PULL_SPEED;
			vecNewEnemyOrigin.z += BARNACLE_PULL_SPEED;

			if (fabs(pev->origin.z - (vecNewEnemyOrigin.z + m_hEnemy->pev->view_ofs.z - 8)) < BARNACLE_BODY_HEIGHT)
			{
				// prey has just been lifted into position ( if the victim origin + eye height + 8 is higher than the bottom of the barnacle, it is assumed that the head is within barnacle's body )
				m_fLiftingPrey = false;

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_bite3.wav", 1, ATTN_NORM);

				pVictim = m_hEnemy->MyMonsterPointer();

				m_flKillVictimTime = gpGlobals->time + 10; // now that the victim is in place, the killing bite will be administered in 10 seconds.

				if (pVictim)
				{
					pVictim->BarnacleVictimBitten(pev);
					SetActivity(ACT_EAT);
				}
			}

			UTIL_SetOrigin(m_hEnemy->pev, vecNewEnemyOrigin);
		}
		else
		{
			// prey is lifted fully into feeding position and is dangling there.

			pVictim = m_hEnemy->MyMonsterPointer();

			if (m_flKillVictimTime != -1 && gpGlobals->time > m_flKillVictimTime)
			{
				// kill!
				if (pVictim)
				{
					pVictim->TakeDamage(pev, pev, pVictim->pev->health, DMG_SLASH | DMG_ALWAYSGIB);
					m_cGibs = 3;
				}

				return;
			}

			// bite prey every once in a while
			if (pVictim && (RANDOM_LONG(0, 49) == 0))
			{
				switch (RANDOM_LONG(0, 2))
				{
				case 0:
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_chew1.wav", 1, ATTN_NORM);
					break;
				case 1:
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_chew2.wav", 1, ATTN_NORM);
					break;
				case 2:
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_chew3.wav", 1, ATTN_NORM);
					break;
				}

				pVictim->BarnacleVictimBitten(pev);
			}
		}
	}
	else
	{
		// barnacle has no prey right now, so just idle and check to see if anything is touching the tongue.

		// If idle and no nearby client, don't think so often
		if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT(1, 1.5); // Stagger a bit to keep barnacles from thinking on the same frame

		if (m_fSequenceFinished)
		{ // this is done so barnacle will fidget.
			SetActivity(ACT_IDLE);
			m_flTongueAdj = -100;
		}

		if (0 != m_cGibs && RANDOM_LONG(0, 99) == 1)
		{
			// cough up a gib.
			CGib::SpawnRandomGibs(pev, 1, true);
			m_cGibs--;

			switch (RANDOM_LONG(0, 2))
			{
			case 0:
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_chew1.wav", 1, ATTN_NORM);
				break;
			case 1:
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_chew2.wav", 1, ATTN_NORM);
				break;
			case 2:
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_chew3.wav", 1, ATTN_NORM);
				break;
			}
		}

		pTouchEnt = TongueTouchEnt(&flLength);

		if (pTouchEnt != NULL && m_fTongueExtended)
		{
			// tongue is fully extended, and is touching someone.
			if (pTouchEnt->FBecomeProne())
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_alert2.wav", 1, ATTN_NORM);

				SetSequenceByName("attack1");
				m_flTongueAdj = -20;

				m_hEnemy = pTouchEnt;

				pTouchEnt->pev->movetype = MOVETYPE_FLY;
				pTouchEnt->pev->velocity = g_vecZero;
				pTouchEnt->pev->basevelocity = g_vecZero;
				pTouchEnt->pev->origin.x = pev->origin.x;
				pTouchEnt->pev->origin.y = pev->origin.y;

				m_fLiftingPrey = true;	 // indicate that we should be lifting prey.
				m_flKillVictimTime = -1; // set this to a bogus time while the victim is lifted.

				m_flAltitude = (pev->origin.z - pTouchEnt->EyePosition().z);
			}
		}
		else
		{
			// calculate a new length for the tongue to be clear of anything else that moves under it.
			if (m_flAltitude < flLength)
			{
				// if tongue is higher than is should be, lower it kind of slowly.
				m_flAltitude += BARNACLE_PULL_SPEED;
				m_fTongueExtended = false;
			}
			else
			{
				m_flAltitude = flLength;
				m_fTongueExtended = true;
			}
		}
	}

	// ALERT( at_console, "tounge %f\n", m_flAltitude + m_flTongueAdj );
	SetBoneController(0, -(m_flAltitude + m_flTongueAdj));
	StudioFrameAdvance(0.1);
}

//=========================================================
// Killed.
//=========================================================
void CBarnacle::Killed(entvars_t* pevAttacker, int iGib)
{
	CBaseMonster* pVictim;

	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	if (m_hEnemy != NULL)
	{
		pVictim = m_hEnemy->MyMonsterPointer();

		if (pVictim)
		{
			pVictim->BarnacleVictimReleased();
		}
	}

	//	CGib::SpawnRandomGibs( pev, 4, 1 );

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_die1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barnacle/bcl_die3.wav", 1, ATTN_NORM);
		break;
	}

	SetActivity(ACT_DIESIMPLE);
	SetBoneController(0, 0);

	StudioFrameAdvance(0.1);

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CBarnacle::WaitTillDead);
}

//=========================================================
//=========================================================
void CBarnacle::WaitTillDead()
{
	pev->nextthink = gpGlobals->time + 0.1;

	float flInterval = StudioFrameAdvance(0.1);
	DispatchAnimEvents(flInterval);

	if (m_fSequenceFinished)
	{
		// death anim finished.
		StopAnimation();
		SetThink(NULL);
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarnacle::Precache()
{
	// PS2HLU
	// Reset hack in precache, is otherwise reset to false
	// by not being saved in Save/Restore
	appliedVisHack = false;

	PRECACHE_MODEL("models/barnacle.mdl");

	PRECACHE_SOUND("barnacle/bcl_alert2.wav"); //happy, lifting food up
	PRECACHE_SOUND("barnacle/bcl_bite3.wav");  //just got food to mouth
	PRECACHE_SOUND("barnacle/bcl_chew1.wav");
	PRECACHE_SOUND("barnacle/bcl_chew2.wav");
	PRECACHE_SOUND("barnacle/bcl_chew3.wav");
	PRECACHE_SOUND("barnacle/bcl_die1.wav");
	PRECACHE_SOUND("barnacle/bcl_die3.wav");
}

//=========================================================
// TongueTouchEnt - does a trace along the barnacle's tongue
// to see if any entity is touching it. Also stores the length
// of the trace in the int pointer provided.
//=========================================================
#define BARNACLE_CHECK_SPACING 8
CBaseEntity* CBarnacle::TongueTouchEnt(float* pflLength)
{
	TraceResult tr;
	float length;

	// trace once to hit architecture and see if the tongue needs to change position.
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 2048), ignore_monsters, ENT(pev), &tr);
	length = fabs(pev->origin.z - tr.vecEndPos.z);
	if (pflLength)
	{
		*pflLength = length;
	}

	Vector delta = Vector(BARNACLE_CHECK_SPACING, BARNACLE_CHECK_SPACING, 0);
	Vector mins = pev->origin - delta;
	Vector maxs = pev->origin + delta;
	maxs.z = pev->origin.z;
	mins.z -= length;

	CBaseEntity* pList[10];
	int count = UTIL_EntitiesInBox(pList, 10, mins, maxs, (FL_CLIENT | FL_MONSTER));
	if (0 != count)
	{
		for (int i = 0; i < count; i++)
		{
			// only clients and monsters
			if (pList[i] != this && IRelationship(pList[i]) > R_NO && pList[i]->pev->deadflag == DEAD_NO) // this ent is one of our enemies. Barnacle tries to eat it.
			{
				return pList[i];
			}
		}
	}

	return NULL;
}

// PS2HLU
void CBarnacle::ApplyVisHack()
{
	// A fix for barnacles getting prematurely culled was attempted in PS2 HL,
	// but was done completely in the wrong place (engine's StudioCheckBBox).
	// The real fix is to ensure it has correct leaf visibility data for it's
	// expected size + the max potential length of it's tongue.
	// This data gets recalculated by the engine every time an edict gets relinked
	// which occurs during movement, size or angle change. The fact that the barnacle
	// is completely stationary is what allows changing this value without it getting
	// overwritten by the engine.
	// Have the engine calculate this for another entity, copy the result, then destroy
	// the dummy entity.
	// This is the best cross-engine solution I found for invoking the engine's
	// SV_FindTouchedLeafs function
	CBarnacleVisHack* pHack = GetClassPtr((CBarnacleVisHack*)NULL);
	pHack->Spawn();
	UTIL_SetOrigin(pHack->pev, pev->origin);
	pHack->pev->angles = pev->angles;

	edict_t* ownEdict = edict();
	edict_t* hackEdict = ENT(pHack->pev);

	// Copy everything because we've got no idea of the amount of visleafs
	ownEdict->headnode = hackEdict->headnode;
	ownEdict->num_leafs = hackEdict->num_leafs;
	memcpy(ownEdict->leafnums, hackEdict->leafnums, sizeof(ownEdict->leafnums));

	//ALERT(at_console, "Applied barnacle vis hack!\n");

	appliedVisHack = true;
}
