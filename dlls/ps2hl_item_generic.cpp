// Header
#include "ps2hl_item_generic.h"

// Link entity
LINK_ENTITY_TO_CLASS(item_generic, CItemGeneric);


// Methods //

// Precache handler
void CItemGeneric::Precache(void)
{
	PRECACHE_MODEL((char *)STRING(pev->model));
}

// Spawn handler
void CItemGeneric::Spawn(void)
{
	// Precache model
	Precache();

	// Set the model
	SET_MODEL(ENT(pev), STRING(pev->model));

	// BBox
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	pev->solid = SOLID_NOT;

	// PS2HLU
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	
	if (m_iSequence || pev->spawnflags & SF_ITEM_GENERIC_DROP_TO_FLOOR)
	{
		SetThink(&CItemGeneric::InitThink);
		pev->nextthink = gpGlobals->time + ITGN_DELAY_THINK;
	}
}

// Parse keys
bool CItemGeneric::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "model"))
	{
		// Set model
		pev->model = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "body"))
	{
		// Set body
		pev->body = atoi(pkvd->szValue);
		return true;
	}
	// PS2HLU
	// set skin
	else if (FStrEq(pkvd->szKeyName, "skin"))
	{
		pev->skin = (int)atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "sequencename"))
	{
		// Set sequence
		m_iSequence = ALLOC_STRING(pkvd->szValue);
		return true;
	}

		return CBaseEntity::KeyValue(pkvd);
}

// Think handler
void CItemGeneric::AnimateThink(void)
{
	// Call animation handler
	DispatchAnimEvents(0.1f);
	StudioFrameAdvance(0);

	if (m_fSequenceFinished && (m_fSequenceLoops == 0))
	{
		pev->frame = 0;
		ResetSequenceInfo();
	}

	// Set delay
	pev->nextthink = gpGlobals->time + ITGN_DELAY_THINK;
}

// PS2HLU
// Later sequence init, fixes drop to floor
// causing items to phase through brush entities
void CItemGeneric::InitThink(void)
{
	// PS2HLU Drop to floor flag, required by Decay
	if (FBitSet(pev->spawnflags, SF_ITEM_GENERIC_DROP_TO_FLOOR))
	{
		if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
		{
			ALERT(at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
			UTIL_Remove( this );
		}
	}

	if (m_iSequence)
	{
		pev->effects = 0;
		pev->sequence = LookupSequence(STRING(m_iSequence));

		// Check if sequence is loaded
		if (pev->sequence == -1)
		{
			// Failed to load sequence
			ALERT(at_console, "item_generic: cant load animation sequence %s\n", STRING(m_iSequence));
			pev->sequence = 0;
		}

		// Prepare sequence
		pev->frame = 0;
		ResetSequenceInfo();

		SetThink(&CItemGeneric::AnimateThink);
		pev->nextthink = gpGlobals->time + ITGN_DELAY_THINK;
	}
}
