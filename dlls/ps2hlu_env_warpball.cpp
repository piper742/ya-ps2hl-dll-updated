#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "customentity.h"
#include "gamerules.h"
#include "ps2hlu_env_warpball.h"
#include "ps2hl_dbg.h"

LINK_ENTITY_TO_CLASS(env_warpball, CWarpBall);

TYPEDESCRIPTION CWarpBall::m_SaveData[] =
	{
		DEFINE_FIELD(CWarpBall, m_iBeams, FIELD_INTEGER),
		DEFINE_FIELD(CWarpBall, m_flLastTime, FIELD_FLOAT),
		DEFINE_FIELD(CWarpBall, m_flMaxFrame, FIELD_FLOAT),
		DEFINE_FIELD(CWarpBall, m_flBeamRadius, FIELD_FLOAT),
		DEFINE_FIELD(CWarpBall, m_iszWarpTarget, FIELD_STRING),
		DEFINE_FIELD(CWarpBall, m_flWarpStart, FIELD_FLOAT),
		DEFINE_FIELD(CWarpBall, m_flDamageDelay, FIELD_FLOAT),
		DEFINE_FIELD(CWarpBall, m_flTargetDelay, FIELD_FLOAT),
		DEFINE_FIELD(CWarpBall, m_fPlaying, FIELD_BOOLEAN),
		DEFINE_FIELD(CWarpBall, m_fDamageApplied, FIELD_BOOLEAN),
		DEFINE_FIELD(CWarpBall, m_fBeamsCleared, FIELD_BOOLEAN),
		DEFINE_FIELD(CWarpBall, m_pBeams, FIELD_CLASSPTR),
		// PS2HLU
		DEFINE_FIELD(CWarpBall, m_flGround, FIELD_FLOAT),
		DEFINE_FIELD(CWarpBall, m_iszMonsterClassname, FIELD_STRING),
		DEFINE_FIELD(CWarpBall, m_iSpawnFlags, FIELD_INTEGER),
		DEFINE_FIELD(CWarpBall, m_fMonsterCreated, FIELD_BOOLEAN),
		DEFINE_FIELD(CWarpBall, m_iMaxLiveChildren, FIELD_INTEGER),
		DEFINE_FIELD(CWarpBall, m_cLiveChildren, FIELD_INTEGER),
		DEFINE_FIELD(CWarpBall, m_fWarpRefire, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CWarpBall, CBaseEntity);

bool CWarpBall::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("radius", pkvd->szKeyName))
	{
		m_flBeamRadius = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq("warp_target", pkvd->szKeyName))
	{
		m_iszWarpTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq("damage_delay", pkvd->szKeyName))
	{
		m_flDamageDelay = atof(pkvd->szValue);
		return true;
	}
	// PS2HLU
	// PS2 additions
	else if (FStrEq(pkvd->szKeyName, "maxlivechildren"))
	{
		m_iMaxLiveChildren = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "monstertype"))
	{
		m_iszMonsterClassname = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "monsterspawnflags"))
	{
		m_iSpawnFlags = atoi(pkvd->szValue);
		return true;
	}

	return false;
}

void CWarpBall::Precache()
{
	PRECACHE_MODEL("sprites/warpball.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/alien_teleport.wav");

	if (FStringNull(m_iszMonsterClassname) == 0)
		UTIL_PrecacheOther(STRING(m_iszMonsterClassname));
}

void CWarpBall::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->rendermode = kRenderGlow;
	pev->renderamt = 255;
	pev->renderfx = kRenderFxNoDissipation;
	pev->framerate = 10;

	m_cLiveChildren = 0;

	// PS2HLU
	// warpball.spr contains combined sprites!
	//m_pSprite = CSprite::SpriteCreate("sprites/Fexplo1.spr", pev->origin, true);
	//m_pSprite->TurnOff();

	// PS2HLU
	// Default to 1 second of damage time, pretty pointless considering
	// that time is always clamped to 1.0
	if (pev->spawnflags & SF_WARPBALL_DELAYED_DAMAGE && m_flDamageDelay == 0.0f)
		m_flDamageDelay = 1.0f;
}


void CWarpBall::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	TryWarpThink();
}

void CWarpBall::TryWarpThink()
{
	if (!m_fPlaying)
	{
		// PS2HLU
		// Don't needlessly spam FX if we cannot have more children
		if (m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren)
		{ // not allowed to make a new one yet. Too many live ones out right now.
			return;
		}

		if (!FStringNull(m_iszWarpTarget))
		{
			auto targetEntity = g_engfuncs.pfnFindEntityByString(0, "targetname", STRING(m_iszWarpTarget));
			if (targetEntity)
				UTIL_SetOrigin(pev, targetEntity->v.origin);
		}

		if ((pev->spawnflags & SF_WARPBALL_DELAYED_DAMAGE) == 0)
		{
			if (0 == m_flGround)
			{
				// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
				TraceResult tr;

				UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 2048), ignore_monsters, ENT(pev), &tr);
				m_flGround = tr.vecEndPos.z;
			}

			Vector mins = pev->origin - Vector(34, 34, 0);
			Vector maxs = pev->origin + Vector(34, 34, 0);
			maxs.z = pev->origin.z;
			mins.z = m_flGround;

			CBaseEntity* pList[2];
			int count = UTIL_EntitiesInBox(pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER);
			if (0 != count)
			{
				// don't build a stack of monsters!
				SetThink(&CWarpBall::TryWarpThink);
				pev->nextthink = gpGlobals->time + RANDOM_FLOAT(2.0f, 4.0f);
				return;
			}
		}
		m_fWarpRefire = false;

		SET_MODEL(pev->pContainingEntity, "sprites/warpball.spr");

		m_flMaxFrame = MODEL_FRAMES(pev->modelindex) - 1;

		pev->rendercolor.x = 77;
		pev->rendercolor.y = 210;
		pev->rendercolor.z = 130;
		pev->scale = 1.2;
		pev->frame = 0;

		if (!m_pBeams)
		{
			m_pBeams = CLightning::LightningCreate("sprites/lgtning.spr", 18);

			m_pBeams->m_iszSpriteName = MAKE_STRING("sprites/lgtning.spr");

			m_pBeams->pev->origin = pev->origin;
			UTIL_SetOrigin(m_pBeams->pev, pev->origin);

			// PS2HLU
			// Less beams in Decay
			if (g_pGameRules->IsCoOp())
				m_pBeams->m_restrike = -0.3f;
			else
				m_pBeams->m_restrike = -0.5f;

			m_pBeams->m_noiseAmplitude = 65;
			m_pBeams->m_boltWidth = 18;
			m_pBeams->m_life = 0.5;

			m_pBeams->pev->rendercolor.x = 0;
			m_pBeams->pev->rendercolor.y = 255;
			m_pBeams->pev->rendercolor.z = 0;

			m_pBeams->pev->spawnflags |= 0x20u;
			m_pBeams->pev->spawnflags |= 2u;

			m_pBeams->m_radius = m_flBeamRadius;
			m_pBeams->m_iszStartEntity = pev->targetname;

			m_pBeams->BeamUpdateVars();
		}

		if (m_pBeams)
		{
			m_pBeams->pev->solid = 0;
			m_pBeams->Precache();
			m_pBeams->SetThink(&CLightning::StrikeThink);
			m_pBeams->pev->nextthink = gpGlobals->time + 0.1;
		}

		SetThink(&CWarpBall::BallThink);
		pev->nextthink = gpGlobals->time + 0.1;

		m_flLastTime = gpGlobals->time;
		m_fBeamsCleared = false;
		m_fPlaying = true;

		if (pev->spawnflags & SF_WARPBALL_DELAYED_DAMAGE && m_flDamageDelay == 0.0f)
		{
			::RadiusDamage(pev->origin, pev, pev, 300, 48, CLASS_NONE, DMG_SHOCK);
			m_fDamageApplied = true;
		}
		else
		{
			m_fDamageApplied = false;
		}

		SUB_UseTargets(this, USE_TOGGLE, 0);

		UTIL_ScreenShake(pev->origin, 4, 100, 2, 1000);

		m_flWarpStart = gpGlobals->time;

		EMIT_SOUND(edict(), CHAN_WEAPON, "debris/alien_teleport.wav", VOL_NORM, ATTN_NORM);

		m_fMonsterCreated = false;
	}
}

void CWarpBall::BallThink()
{
	pev->frame = ((gpGlobals->time - m_flLastTime) * pev->framerate) + pev->frame;

	if (pev->frame > m_flMaxFrame)
	{
		SET_MODEL(edict(), "");

		SetThink(nullptr);

		if ((pev->spawnflags & SF_WARPBALL_FIRE_ONCE) != 0)
			UTIL_Remove(this);

		m_fPlaying = false;

		// PS2HLU
		// Retry spawning warpball
		if (m_fWarpRefire)
		{
			SetThink(&CWarpBall::TryWarpThink);
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT(2.0f, 4.0f);
		}
	}
	else
	{
		// PS2HLU
		// Clamp damage delay to 1.0 at least
		// It's also the monster spawning delay!
		if (m_iszMonsterClassname &&
				m_fMonsterCreated == 0 &&
				(gpGlobals->time - m_flWarpStart) >= V_max(m_flDamageDelay + 0.2f, 1.0f))
		{
			MakeMonster();
			m_fMonsterCreated = true;
		}

		if (m_pBeams)
		{
			if (pev->frame >= (m_flMaxFrame - 4.0))
			{
				m_pBeams->SetThink(nullptr);
				m_pBeams->pev->nextthink = gpGlobals->time;
			}
		}

		pev->nextthink = gpGlobals->time + 0.1;
		m_flLastTime = gpGlobals->time;
	}
}

// PS2HLU
// Moved from monstermaker
void CWarpBall::MakeMonster()
{
	edict_t* pent;
	entvars_t* pevCreate;

	if (m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren)
	{ // not allowed to make a new one yet. Too many live ones out right now.
		return;
	}

	if (0 == m_flGround)
	{
		// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
		TraceResult tr;

		UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 2048), ignore_monsters, ENT(pev), &tr);
		m_flGround = tr.vecEndPos.z;
	}

	Vector mins = pev->origin - Vector(34, 34, 0);
	Vector maxs = pev->origin + Vector(34, 34, 0);
	maxs.z = pev->origin.z;
	mins.z = m_flGround;

	CBaseEntity* pList[2];
	int count = UTIL_EntitiesInBox(pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER);
	if (0 != count)
	{
		// PS2HLU
		// Try again if we only failed here!
		if ((pev->spawnflags & SF_WARPBALL_DELAYED_DAMAGE) == 0)
			m_fWarpRefire = true;

		// don't build a stack of monsters!
		return;
	}

	pent = CREATE_NAMED_ENTITY(m_iszMonsterClassname);

	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in MonsterMaker!\n");
		return;
	}

	// PS2HLU
	// happens in warpballthink
	/*
	// If I have a target, fire!
	if (!FStringNull(pev->target))
	{
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
	}
	*/

	pevCreate = VARS(pent);
	pevCreate->origin = pev->origin;
	pevCreate->angles = pev->angles;

	// PS2HLU
	// Set spawnflags here!
	SetBits(pevCreate->spawnflags, (SF_MONSTER_FALL_TO_GROUND | m_iSpawnFlags));

	DispatchSpawn(ENT(pevCreate));
	pevCreate->owner = edict();

	if (!FStringNull(pev->netname))
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pevCreate->targetname = pev->netname;
	}

	m_cLiveChildren++; // count this monster
	
	/*
	m_cNumMonsters--;

	if (m_cNumMonsters == 0)
	{
		// Disable this forever.  Don't kill it because it still gets death notices
		SetThink(NULL);
		SetUse(NULL);
	}
	*/
}

// PS2HLU
// Borrowed from monstermaker
void CWarpBall::DeathNotice(entvars_t* pevChild)
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_cLiveChildren--;

	// PS2HLU
	if ((pevChild->spawnflags & SF_MONSTER_FADECORPSE) == 0)
	{
		pevChild->owner = NULL;
	}
}
