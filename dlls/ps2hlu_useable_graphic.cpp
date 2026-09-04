#include "ps2hlu_useable_graphic.h"
#include "gamerules.h"
#include "util.h"

LINK_ENTITY_TO_CLASS(useable_graphic, CUseableGraphic);

void CUseableGraphic::Spawn(void)
{
	Precache();

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->rendercolor = Vector(255, 255, 255);
	pev->renderamt = 255;
	pev->effects = 0;

	// PS2HLU
	// This is missing in PS2HL, but the sprite behaves as if this is set.
	// Oddly enough setting this disables the downscaling effect of kRenderGlow
	// despite that not being documented ANYWHERE, so we just have to make a guess
	// and downscale the sprite further to somewhat match the original intent
	pev->renderfx = kRenderFxNoDissipation;

	SET_MODEL(ENT(pev), "sprites/bracket_02.spr");
	UTIL_SetOrigin(pev, pev->origin);
}

void CUseableGraphic::Precache(void)
{
	PRECACHE_MODEL("sprites/bracket_02.spr");
}

CUseableGraphic* CUseableGraphic::CreateUseableGraphic(CBaseEntity* pTarget, UseableGraphicColor hudColor)
{
	if (pTarget == nullptr)
		return nullptr;

	CUseableGraphic* pGraphic = GetClassPtr((CUseableGraphic*)NULL);

	UTIL_SetOrigin(pGraphic->pev, pGraphic->pev->origin);
	pGraphic->pev->classname = MAKE_STRING("useable_graphic");
	pGraphic->Spawn();

	if (!FStringNull(pTarget->pev->model))
	{
		//PRECACHE_MODEL((char*)STRING(pTarget->pev->model));
		//SET_MODEL(pTarget->edict(), STRING(pTarget->pev->model));
	}

	// VecBModelOrigin run through abs (?)
	Vector vecCenter = Vector(abs((pTarget->pev->absmax.x - pTarget->pev->absmin.x) * 0.5f),
							abs((pTarget->pev->absmax.y - pTarget->pev->absmin.y) * 0.5f),
							abs((pTarget->pev->absmax.z - pTarget->pev->absmin.z) * 0.5f));

	pGraphic->pev->scale = vecCenter.z;

	// clamp
	if (pGraphic->pev->scale < vecCenter.x)
		pGraphic->pev->scale = vecCenter.x;
	if (pGraphic->pev->scale < vecCenter.y)
		pGraphic->pev->scale = vecCenter.y;
	if (pGraphic->pev->scale > 32.0f)
		pGraphic->pev->scale = 32.0f;

	Vector vecShiftedOrigin = pTarget->pev->origin;

	// Fix for BModels
	if (FStringNull(pTarget->pev->model) == 0)
		if (STRING(pTarget->pev->model)[0] == '*')
			vecShiftedOrigin = VecBModelOrigin(pTarget->pev);

	if (FClassnameIs(pTarget->pev, "item_eyescanner"))
	{
		UTIL_MakeVectors(pTarget->pev->angles);
		Vector shift(gpGlobals->v_right.x * -2.3f,
								 gpGlobals->v_right.y * -2.3f,
								 gpGlobals->v_right.z * -2.3f);

		vecShiftedOrigin = vecShiftedOrigin + shift;
		vecShiftedOrigin.z += 45.0f;

		// PS2HLU
		// Disabled due to kRenderFxNoDissipation
		//pGraphic->pev->scale *= 0.5f;
	}

	if (FClassnameIs(pTarget->pev, "item_healthcharger") ||
			FClassnameIs(pTarget->pev, "item_recharge"))
	{
		// PS2HLU
		// Changed due to kRenderFxNoDissipation
		//pGraphic->pev->scale = pGraphic->pev->scale * 0.25f;
		pGraphic->pev->scale *= 25.0f;

		vecShiftedOrigin.z += 15.0f;
	}

	// PS2HLU
	pGraphic->pev->scale = pGraphic->pev->scale * 0.05f;

	//pGraphic->pev->origin = vecShiftedOrigin; // redundant and wrong
	UTIL_SetOrigin(pGraphic->pev, vecShiftedOrigin);

	if (hudColor == UseableGraphicColor::ALIEN)
			pGraphic->pev->rendercolor = Vector(0, 160, 0);
	else if ( g_pGameRules->IsCoOp() )
	{
		switch (hudColor)
		{
			case UseableGraphicColor::COLETTE:
				pGraphic->pev->rendercolor = Vector(255, 128, 64);
			break;
			case UseableGraphicColor::GINA:
				pGraphic->pev->rendercolor = Vector(160, 160, 192);
			break;
			case UseableGraphicColor::GORDON:
			default:
				pGraphic->pev->rendercolor = Vector(255, 160, 0);
			break;
		}
	}
	else
			pGraphic->pev->rendercolor = Vector(255, 160, 0);

	return pGraphic;
}
