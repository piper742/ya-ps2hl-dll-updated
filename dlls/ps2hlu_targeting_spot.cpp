#include "ps2hlu_targeting_spot.h"
#include <gamerules.h>

LINK_ENTITY_TO_CLASS(targeting_spot, CTargetingSpot);

CTargetingSpot* CTargetingSpot::CreateTargetingSpot(AimTargetColor hudColor)
{
	CTargetingSpot* pSpot = GetClassPtr((CTargetingSpot*)NULL);

	pSpot->pev->classname = MAKE_STRING("targeting_spot");
	pSpot->pev->movetype = MOVETYPE_NONE;
	pSpot->pev->solid = SOLID_NOT;
	pSpot->pev->rendermode = kRenderTransAdd;

	if (hudColor == AimTargetColor::ALIEN)
			pSpot->pev->rendercolor = Vector(0, 160, 0);
	else if ( g_pGameRules->IsCoOp() )
	{
		switch (hudColor)
		{
			case AimTargetColor::COLETTE:
				pSpot->pev->rendercolor = Vector(255, 128, 64);
			break;
			case AimTargetColor::GINA:
				pSpot->pev->rendercolor = Vector(160, 160, 192);
			break;
			case AimTargetColor::GORDON:
			default:
				pSpot->pev->rendercolor = Vector(255, 160, 0);
			break;
		}
	}
	else
			pSpot->pev->rendercolor = Vector(255, 160, 0);

	pSpot->pev->renderamt = 255;
	SET_MODEL(pSpot->edict(), "sprites/bracket.spr");
	UTIL_SetOrigin(pSpot->pev, pSpot->pev->origin);

	return pSpot;
}

void CTargetingSpot::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor = Vector(255, 255, 255);
	pev->renderamt = 255;
	SET_MODEL(ENT(pev), "sprites/bracket.spr");
	UTIL_SetOrigin(pev, pev->origin);
}

void CTargetingSpot::Precache(void)
{
	PRECACHE_MODEL("sprites/bracket.spr");
}

void CTargetingSpot::ScaleThink(void)
{
	if (pev->scale > 1.0f)
	{
		pev->scale -= 0.5f;
		pev->nextthink = gpGlobals->time + 0.1f;

		if (pev->scale == 1.0f && pev->rendercolor.z == 64.0f)
			pev->scale = 0.9f;
	}
	else
		pev->origin = g_vecZero; // What's the purpose of this?
}

void CTargetingSpot::SetTarget(CBaseEntity* pTarget)
{
	if (pTarget == nullptr)
		return;

	UTIL_SetOrigin(pev, pTarget->pev->origin);
	pev->skin = ENTINDEX(pTarget->edict());
	pev->body = 0;

	// Determine if we are a monster or not (the very ugly way)
	if (MODEL_FRAMES(pTarget->pev->modelindex) && pTarget->pev->yaw_speed)
	{
		const int numAttachments = static_cast<CBaseMonster*>(pTarget)->GetNumAttachments();

		if (numAttachments > 0)
			pev->body = numAttachments;
	}

	pev->aiment = ENT(pTarget->pev);
	pev->movetype = MOVETYPE_FOLLOW;
	pev->scale = 3.0f;

	SetThink(&CTargetingSpot::ScaleThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}
