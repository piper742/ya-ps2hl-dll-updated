#ifndef PS2HLU_USEABLE_GRAPHIC
#define PS2HLU_USEABLE_GRAPHIC

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"

// TODO: Merge this with targeting_spot's to prevent duplicates!
enum class UseableGraphicColor : unsigned char {
	GORDON = 0, // yellow
	GINA, // white
	COLETTE, // red
	ALIEN, // green
};

class CUseableGraphic : public CPointEntity
{
public:
	void Spawn(void) override;
	void Precache(void) override;

	// Do we need don't save?
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & (~FCAP_ACROSS_TRANSITION)) | FCAP_DONT_SAVE; }

	static CUseableGraphic* CreateUseableGraphic(CBaseEntity* pTarget, UseableGraphicColor hudColor);
};



#endif
