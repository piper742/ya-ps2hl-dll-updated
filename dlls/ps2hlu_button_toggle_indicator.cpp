#include "extdll.h"
#include "util.h"
#include "cbase.h"

constexpr int SF_NO_OBJECT_HIGHLIGHT = 0x200;

// Toggles the object highlight around the button/interactable object
class CButtonToggleIndicator : public CPointEntity
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

LINK_ENTITY_TO_CLASS(button_toggle_indicator, CButtonToggleIndicator);

void CButtonToggleIndicator::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pEntity;

	pEntity = UTIL_FindEntityByTargetname(0, STRING(pev->target));

	if (pEntity)
	{
		if (FBitSet(pEntity->pev->spawnflags, SF_NO_OBJECT_HIGHLIGHT))
			pEntity->pev->spawnflags &= ~SF_NO_OBJECT_HIGHLIGHT;
		else
			pEntity->pev->spawnflags |= SF_NO_OBJECT_HIGHLIGHT;
	}
}
