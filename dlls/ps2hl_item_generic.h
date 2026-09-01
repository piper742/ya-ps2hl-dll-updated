/*

22.08.18

"item_generic" entity for ps2hl. Acts like env_model from SoHL

Created by supadupaplex
Based on this article https://www.moddb.com/games/half-life/tutorials/where-is-poppy-your-first-custom-entity-part-2

*/

// Include guard: start
#ifndef PS2_ITEM_GENERIC_H
#define PS2_ITEM_GENERIC_H

// Header files
#include "extdll.h"		// Required for KeyValueData
#include "util.h"		// Required Consts & Macros
#include "cbase.h"		// Required for CPointEntity

#define SF_ITEM_GENERIC_DROP_TO_FLOOR 1 // PS2HLU New drop to floor flag
#define ITGN_DELAY_THINK 0.1f

class CItemGeneric : public CBaseAnimating
{
private:
	// Methods
	void Spawn(void);						// Spawn handler
	void Precache(void);					// Precache handler
	bool KeyValue(KeyValueData *pkvd);		// Parse keys
	void EXPORT AnimateThink(void);						// Think handler
	
	// PS2HLU
	void EXPORT InitThink(void);

	int m_iSequence;
};

// Include guard: end
#endif
