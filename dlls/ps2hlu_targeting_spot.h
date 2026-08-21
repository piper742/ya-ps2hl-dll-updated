#ifndef PS2HLU_TARGETING_SPOT_H
#define PS2HLU_TARGETING_SPOT_H

// Header files
#include "extdll.h"		// Required for KeyValueData
#include "util.h"		// Required Consts & Macros
#include "cbase.h"		// Required for CPointEntity
#include "player.h"
#include "gamerules.h"	// Required for getting "game rules" values
#include "ps2hl_dbg.h"	// BBox render func

enum class AimTargetColor : unsigned char {
	GORDON = 0, // yellow
	GINA, // white
	COLETTE, // red
	ALIEN, // green
};

class CTargetingSpot : public CPointEntity
{
public:
	void Spawn(void) override;
	void Precache(void) override;
	void EXPORT ScaleThink(void);

	// Do we need don't save?
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & (~FCAP_ACROSS_TRANSITION)) | FCAP_DONT_SAVE; }

	static CTargetingSpot* CreateTargetingSpot(AimTargetColor hudColor);
	void SetTarget(CBaseEntity* pTarget);
};

#endif
