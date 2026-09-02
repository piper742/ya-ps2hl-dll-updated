#pragma once

/****
* PS2HLU - env_warpball
* Based off of code from Blue Shift Updated:
* https://github.com/twhl-community/halflife-bs-updated/blob/d53e3a4aa6d3d9c2bc5c299c2f2499f57c9f68ab/dlls/effects.cpp#L2278-L2535
*****/

#include "effects.h"

const int SF_WARPBALL_FIRE_ONCE = 1 << 0;
const int SF_WARPBALL_DELAYED_DAMAGE = 1 << 1;

/**
*	@brief Alien teleportation effect
*/
class CWarpBall : public CBaseEntity
{
public:
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int Classify() override { return CLASS_NONE; }

	bool KeyValue(KeyValueData* pkvd) override;

	void Precache() override;
	void Spawn() override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT BallThink();

	// PS2HLU
	void MakeMonster();
	void EXPORT TryWarpThink();
	void DeathNotice(entvars_t* pevChild) override;

	static CWarpBall* CreateWarpBall(Vector vecOrigin)
	{
		auto warpBall = GetClassPtr<CWarpBall>(nullptr);

		UTIL_SetOrigin(warpBall->pev, vecOrigin);

		warpBall->pev->classname = MAKE_STRING("env_warpball");
		warpBall->Spawn();

		return warpBall;
	}

	CLightning* m_pBeams;
	int m_iBeams;
	float m_flLastTime;
	float m_flMaxFrame;
	float m_flBeamRadius;
	string_t m_iszWarpTarget;
	float m_flWarpStart;
	float m_flDamageDelay;
	float m_flTargetDelay;
	bool m_fPlaying;
	bool m_fDamageApplied;
	bool m_fBeamsCleared;

	// PS2HLU
	float m_flGround = 0.0f;
	string_t m_iszMonsterClassname;
	int m_iSpawnFlags = 0;
	bool m_fMonsterCreated = false;
	int m_iMaxLiveChildren = 0;
	int m_cLiveChildren = 0;
	bool m_fWarpRefire = false;
};
