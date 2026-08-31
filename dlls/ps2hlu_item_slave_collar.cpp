#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "effects.h"
#include "soundent.h"

// This change doesn't make any sense compared to islave/vortigaunts
// There's always only a maximum of 6 beams present...
constexpr int COLLAR_MAX_BEAMS = 12;

class CISlaveCollar : public CBaseAnimating
{
public:
	void Spawn(void) override;
	void Precache(void) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;

	// Moved from ISlave
	void ClearAllBeams();
	void Clear8Beams(); // why???
	void ArmBeam(int side);
	//void WackBeam(int side, CBaseEntity* pEntity); // Dead code
	void ZapBeam(int side);
	void BeamGlow();

	// I'm sorry I couldn't bother naming this mess
	// This entire class is a rushed broken mess, it's a miracle
	// it even worked in retail!
	void EXPORT Think3(void);
	void EXPORT Think2(void);
	void EXPORT ZapThink();
	void ShootArmBeams();

private:
	int m_iBeams = 0;
	CBeam* m_pBeam[COLLAR_MAX_BEAMS];

	// This is part of the class on PS2
	// BUT THESE DO NOTHING!!!
	// m_flNextUse isn't set anywhere, and m_fCollarOn
	// is only set, but never read, instead spawnflag bit 1
	// is used as an indicator of this.
	//float m_fCollarOn = 0.0f;
	//float m_flNextUse = 0.0f;
};

LINK_ENTITY_TO_CLASS(item_slave_collar, CISlaveCollar);

void CISlaveCollar::Think3(void)
{
	ALERT(at_console, "collar test is thinking!");
	if ((pev->spawnflags & 1) == 0)
	{
		ClearAllBeams();
	}
	else
	{
		ShootArmBeams();
		SetThink(&CISlaveCollar::Think2);
		pev->nextthink = gpGlobals->time + 2.0f;
	}
}

void CISlaveCollar::Think2(void)
{
	// Only clear 8?
	Clear8Beams();
	//ClearAllBeams();

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG(130, 160));

	// PS2HLU
	// This isn't done here in retail, the m_iBeams indexing into the array is
	// somehow broken in ZapBeam so those beams never get properly cleared
	// The only side-effect of this is the beam end points changing
	ZapThink();

	SetThink(&CISlaveCollar::Think3);
	pev->nextthink = gpGlobals->time + 2.0f;
}


void CISlaveCollar::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// PS2HLU
	// A delay check is here in retail, but the delay never gets set...

	pev->spawnflags ^= 1;

	if (pev->spawnflags & 1)
	{
		ShootArmBeams();
		SetThink(&CISlaveCollar::ZapThink);
		pev->nextthink = gpGlobals->time + 2.0f;
	}
	else
	{
		ClearAllBeams();
		SetThink(NULL);
		pev->nextthink = gpGlobals->time;
	}
}

void CISlaveCollar::Spawn(void)
{
	Precache();

	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL(ENT(pev), "models/collar_test.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	DROP_TO_FLOOR(ENT(pev));

	// PS2HLU
	// This is 1 in retail, which spams a bunch of
	// DLIGHTs that last for 20 seconds...
	// Considering the button can be spammed and that I've got no clue
	// if exceeding MAX_DLIGHTs crashes the server, let's change this to 0.5
	// so that the DLIGHT lasts roughly 4 seconds which is an entire charge/fire cycle
	pev->framerate = 0.5;

	// This is unbelievable, this spawnflag needs to be flipped at spawn.
	// I don't think I've messed up the spawnflag check, so where does this
	// happen in retail?
	pev->spawnflags ^= 1;

	// On a side note, I hoped that there would be something referencing the
	// very cool firing animation in collar_test.mdl, but sadly no.
	// This entire class seems extremely rushed

	SetThink(&CISlaveCollar::Think3);
	pev->nextthink = gpGlobals->time + 2.0f;
}

void CISlaveCollar::Precache(void)
{
	PRECACHE_MODEL("models/collar_test.mdl");

	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("hassult/hw_shoot1.wav");

	// Why?
	UTIL_PrecacheOther("test_effect");
}

void CISlaveCollar::ArmBeam(int side)
{
	TraceResult tr;
	float flDist = 1.0;

	if (m_iBeams >= COLLAR_MAX_BEAMS)
		return;

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT(pev), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	Vector tmp, tmp2;
	GetAttachment(side-1, tmp, tmp2);

	//m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	//m_pBeam[m_iBeams]->SetEndAttachment(side);
	m_pBeam[m_iBeams]->PointsInit(tr.vecEndPos, tmp);
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetColor(96, 128, 16);
	m_pBeam[m_iBeams]->SetBrightness(64);
	m_pBeam[m_iBeams]->SetNoise(80);
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;
}

//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CISlaveCollar::BeamGlow()
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255)
		{
			m_pBeam[i]->SetBrightness(b);
		}
	}
}


//=========================================================
// WackBeam - regenerate dead colleagues
// DEAD CODE!!!
//=========================================================
/*
void CISlaveCollar::WackBeam(int side, CBaseEntity* pEntity)
{
	Vector vecDest;
	float flDist = 1.0;

	if (m_iBeams >= COLLAR_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(pEntity->Center(), entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	m_pBeam[m_iBeams]->SetColor(180, 255, 96);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(80);
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;
}
*/

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CISlaveCollar::ZapBeam(int side)
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity* pEntity;

	if (m_iBeams >= COLLAR_MAX_BEAMS)
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = gpGlobals->v_forward;
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT(0, deflection) + gpGlobals->v_up * RANDOM_FLOAT(-deflection, deflection);
	UTIL_TraceLine(vecSrc, vecSrc + vecAim * 1024, ignore_monsters, ENT(pev), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 50);
	if (!m_pBeam[m_iBeams])
		return;

	Vector tmp, tmp2;
	GetAttachment(side-1, tmp, tmp2);

	//m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	//m_pBeam[m_iBeams]->SetEndAttachment(side);
	m_pBeam[m_iBeams]->PointsInit(tr.vecEndPos, tmp);
	m_pBeam[m_iBeams]->SetColor(180, 255, 96);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(20);
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;

	// PS2HLU
	// m_iBeams isn't incremented here in retail, but doing so breaks this
	// This class is such a mess in retail, it's unbelievable

	/*
	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && 0 != pEntity->pev->takedamage)
	{
		pEntity->TraceAttack(pev, gSkillData.slaveDmgZap, vecAim, &tr, DMG_SHOCK);
	}
	*/

	UTIL_EmitAmbientSound(ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));
}

//=========================================================
// Clear8Beams - remove 8 beams
//=========================================================
void CISlaveCollar::Clear8Beams()
{
	// PS2HLU
	// ISLAVE_MAX_BEAMS, Weird...
	for (int i = 0; i < 8; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;

	STOP_SOUND(ENT(pev), CHAN_WEAPON, "debris/zap4.wav");
}

//=========================================================
// ClearAllBeams - remove all beams
//=========================================================
void CISlaveCollar::ClearAllBeams()
{
	for (int i = 0; i < COLLAR_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;

	STOP_SOUND(ENT(pev), CHAN_WEAPON, "debris/zap4.wav");
}

void CISlaveCollar::ShootArmBeams()
{
	UTIL_MakeAimVectors(pev->angles);

	// PS2HLU
	// This only happens when playing in one-player mode (numclients = 1).
	// m_iBeams is broken in retail! It doesn't count ZapBeams!!!
	if (/*m_iBeams == 0*/ true)
	{
		Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(vecSrc.x);			 // X
		WRITE_COORD(vecSrc.y);			 // Y
		WRITE_COORD(vecSrc.z);			 // Z
		WRITE_BYTE(12);					 // radius * 0.1
		WRITE_BYTE(255);				 // r
		WRITE_BYTE(180);				 // g
		WRITE_BYTE(96);					 // b
		WRITE_BYTE(20 / pev->framerate); // time * 10
		WRITE_BYTE(0);					 // decay * 0.1
		MESSAGE_END();
	}

		ArmBeam(1);
		ArmBeam(2);
		ArmBeam(3);
		BeamGlow();

	// PS2HLU
	// This sounds different than in retail due to ZapBeam properly incrementing m_iBeams
	// I'm not even going to bother recreating that incredibly busted logic here
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10);
}

void CISlaveCollar::ZapThink()
{
		ClearAllBeams();

		UTIL_MakeAimVectors(pev->angles);

		ZapBeam(1);
		ZapBeam(2);
		ZapBeam(3);

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG(130, 160));
		
		SetThink(&CISlaveCollar::Think3);
		pev->nextthink = gpGlobals->time + 2.0f;
}
