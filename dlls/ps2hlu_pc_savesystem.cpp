#include "ps2hlu_pc_savesystem.h"
#include "ps2hlu_pc_lighttracker.h"
#include <stdlib.h>
#include "effects.h"

static TYPEDESCRIPTION saveHeader[] = {
	DEFINE_FIELD( SAVE_HEADER, skillLevel, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, entityCount, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, connectionCount, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, lightStyleCount, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, time, FIELD_TIME ),
	DEFINE_ARRAY( SAVE_HEADER, mapName, FIELD_CHARACTER, 32 ),
	DEFINE_ARRAY( SAVE_HEADER, skyName, FIELD_CHARACTER, 32 ),
	DEFINE_FIELD( SAVE_HEADER, skyColor_r, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, skyColor_g, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, skyColor_b, FIELD_INTEGER ),
	DEFINE_FIELD( SAVE_HEADER, skyVec_x, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, skyVec_y, FIELD_FLOAT ),
	DEFINE_FIELD( SAVE_HEADER, skyVec_z, FIELD_FLOAT ),
};

static TYPEDESCRIPTION gEntityTable[] =
{
	DEFINE_FIELD( ENTITYTABLE, id, FIELD_INTEGER ),
	DEFINE_FIELD( ENTITYTABLE, location, FIELD_INTEGER ),
	DEFINE_FIELD( ENTITYTABLE, size, FIELD_INTEGER ),
	DEFINE_FIELD( ENTITYTABLE, flags, FIELD_INTEGER ),
	DEFINE_FIELD( ENTITYTABLE, classname, FIELD_STRING ),
};

constexpr int FOURMB = 4 * 1024 * 1024;
constexpr int TOKENTABLESIZE = 0xFFF;

CSaveSystem::CSaveSystem() {
	m_State = SaveState::NONE;
	m_SaveHeader = {0};

	ALERT(at_console, "initialized save system!!!!\n");

	m_SaveRestoreData = (SAVERESTOREDATA*)calloc(1, sizeof(SAVERESTOREDATA) + FOURMB );
	m_SaveRestoreData->bufferSize = FOURMB;
	
	m_SaveRestoreData->pTokens = (char**)calloc(1, sizeof(char*) * TOKENTABLESIZE);
	m_SaveRestoreData->tokenCount = TOKENTABLESIZE;

	m_SaveRestoreData->pTable = (ENTITYTABLE*)calloc(1, sizeof(ENTITYTABLE) * gpGlobals->maxEntities);
	m_SaveRestoreData->tableCount = gpGlobals->maxEntities;

	m_pBackupTable = (ENTITYTABLE*)calloc(1, sizeof(ENTITYTABLE) * gpGlobals->maxEntities);
}

CSaveSystem::~CSaveSystem() {

	if (m_SaveRestoreData->pTable)
	{
		free(m_SaveRestoreData->pTable);
		m_SaveRestoreData->pTable = nullptr;
		m_SaveRestoreData->tableCount = 0;
	}

	if (m_SaveRestoreData->pTokens)
	{
		free(m_SaveRestoreData->pTokens);
		m_SaveRestoreData->pTokens = nullptr;
	}

	if (m_SaveRestoreData)
	{
		free(m_SaveRestoreData);
		m_SaveRestoreData = nullptr;
	}

	if (m_pBackupTable)
	{
		free(m_pBackupTable);
		m_pBackupTable = nullptr;
	}
}

void CSaveSystem::ResetMem()
{
		if (m_SaveRestoreData->pTokens)
		{
		memset(m_SaveRestoreData->pTokens, 0, sizeof(char*) * TOKENTABLESIZE);
		m_SaveRestoreData->tokenCount = TOKENTABLESIZE;
		}

		// Reallocate memory in case we've persisted through a maxplayers change
		if (m_SaveRestoreData->pTable)
		{
			free(m_SaveRestoreData->pTable);
			m_SaveRestoreData->pTable = 0;
			m_SaveRestoreData->tableCount = 0;
		}

		m_SaveRestoreData->pTable = (ENTITYTABLE*)calloc(1, sizeof(ENTITYTABLE) * gpGlobals->maxEntities);
		m_SaveRestoreData->tableCount = gpGlobals->maxEntities;

		// backup table
		if (m_pBackupTable == nullptr)
			m_pBackupTable = (ENTITYTABLE*)calloc(1, sizeof(ENTITYTABLE) * gpGlobals->maxEntities);
}

void CSaveSystem::ResetPtr()
{
	m_SaveRestoreData->size = 0;
	m_SaveRestoreData->pBaseData = (char*)(m_SaveRestoreData + 1);
	m_SaveRestoreData->pCurrentData = m_SaveRestoreData->pBaseData;
	// TODO: Do we need to handle currentIndex?
}

void CSaveSystem::WriteHeader()
{
	m_SaveHeader.time = gpGlobals->time;
	m_SaveHeader.connectionCount = 0;
	m_SaveHeader.skillLevel = 1; // TODO: Save!!!
	m_SaveHeader.entityCount = m_SaveRestoreData->tableCount;
	m_SaveHeader.lightStyleCount = 0; // useless since we've got a seperate light tracker
	m_SaveRestoreData->time = 0.0f;
	SaveWriteFields(m_SaveRestoreData, "Save Header", &m_SaveHeader, saveHeader, ARRAYSIZE(saveHeader));
	m_SaveRestoreData->time = m_SaveHeader.time;

	// TODO: Save sky color & vectors since it can change
	// on maps with multiple light environments!

	SaveGlobalState(m_SaveRestoreData);
}

void CSaveSystem::ReadHeader()
{
			SaveReadFields(m_SaveRestoreData, "Save Header", &m_SaveHeader, saveHeader, ARRAYSIZE(saveHeader));
			RestoreGlobalState(m_SaveRestoreData);
}

void CSaveSystem::Save()
{
	// We're not idle, abort!
	if (m_State > SaveState::HAS_SAVE)
		return;

	m_State = SaveState::SAVING;

	ResetPtr();
	ResetMem();

	// backup & override engine provided pointer
	pBackupGlobalData = (SAVERESTOREDATA*)gpGlobals->pSaveData;
	gpGlobals->pSaveData = m_SaveRestoreData;

	// save edicts
	for (int i = 0; i < gpGlobals->maxEntities; i++)
	{
		ENTITYTABLE* pTemp = &m_SaveRestoreData->pTable[i];
		pTemp->pent = INDEXENT(i);
		pTemp->id = i;
	}

	// Save current light styles
	if (pLightTracker)
		pLightTracker->Save();

	// write save header in memory
	WriteHeader();

	// populate with member variables
	ENTITYTABLE* pTable = nullptr;
	edict_t* pFirst = INDEXENT(0);
	for (int i = 1; i < gpGlobals->maxEntities; i++)
	{
		pFirst++; // skip worldspawn

		pTable = &m_SaveRestoreData->pTable[i];
		pTable->location = m_SaveRestoreData->size;
		m_SaveRestoreData->currentIndex = i;
		pTable->size = 0;

		if (FNullEnt(pFirst))
			continue;

		DispatchSave(pFirst, m_SaveRestoreData);

		// not used anywhere but it's a decent idea to mark the data
		if (FBitSet(pFirst->v.flags, FL_CLIENT))
		{
			SetBits(pTable->flags, 0x80000000);
		}
	}

	// write entity data
	for (int i = 0; i < m_SaveRestoreData->tableCount; i++)
	{
		SaveWriteFields(m_SaveRestoreData, "ETABLE", &m_SaveRestoreData->pTable[i], gEntityTable, ARRAYSIZE(gEntityTable));
	}

	// copy to backup table
	memcpy(m_pBackupTable, m_SaveRestoreData->pTable, (sizeof(ENTITYTABLE) * gpGlobals->maxEntities));

	// save strings
	char* pTokenData = m_SaveRestoreData->pCurrentData;

	if (m_SaveRestoreData->pTokens)
	{
		for (int i = 0; i < m_SaveRestoreData->tokenCount; i++)
		{
			const char *pszToken = m_SaveRestoreData->pTokens[i] ? m_SaveRestoreData->pTokens[i] : "";

			// just copy the token byte-by-byte
			while( *pszToken )
				*m_SaveRestoreData->pCurrentData++ = *pszToken++;
			*m_SaveRestoreData->pCurrentData++ = 0; // Write the term
		}
	}

	m_SaveRestoreData->tokenSize = m_SaveRestoreData->pCurrentData - pTokenData;

	gpGlobals->pSaveData = pBackupGlobalData;
	m_State = SaveState::HAS_SAVE;
}

void CSaveSystem::Restore()
{
	if (m_State != SaveState::HAS_SAVE)
		return;

	m_State = SaveState::LOADING;

	// backup & override engine provided pointer
	pBackupGlobalData = (SAVERESTOREDATA*)gpGlobals->pSaveData;
	gpGlobals->pSaveData = m_SaveRestoreData;

	ResetPtr();
	
	// clean up entities that appeared since our save
	for (int i = gpGlobals->maxClients + 1; i < gpGlobals->maxEntities; i++)
	{
			ENTITYTABLE *pTable = &m_SaveRestoreData->pTable[i];
			edict_t* pEnt = INDEXENT(i);

			if (pEnt && FNullEnt(pEnt) == 0)
			{
				// applies hacks and skips removal in case a hack needs it
				if (PreHacks(pEnt))
					continue;

				// preserve matching entities (EHANDLE check + classname)
				if (pTable->pent && FClassnameIs(pEnt, STRING(pTable->classname)) &&
						pTable->pent->serialnumber == pEnt->serialnumber /*&&
						pEnt->v.origin == pTable->pent->v.origin*/) // TODO: Why did I have origin here?
					continue;

				UTIL_Remove(CBaseEntity::Instance(pEnt));
			}
	}

	// really bizarre that the savesystem has a delta time restore mechanism
	// since it's useless since the engine resets the time on a restore anyway
	m_SaveRestoreData->time = gpGlobals->time;

	// HACK!!! Lie that we used a landmark so that the player's
	// position doesn't get reset
	m_SaveRestoreData->fUseLandmark = 1;

	// Restore saved light styles
	if (pLightTracker)
		pLightTracker->Restore();

	ResetGlobalState();
	ReadHeader();

	// Danger zone ahead!!!
	// The following loops mutate m_SaveRestoreData->pTable based off of current game
	// state! If you need ANYTHING from there be sure to grab it right here!
	// m_SaveRestoreData->pTable will be reset at the end of this function!

	// First pass
	// Recreate missing entities & shuffle around entity pointers so that memeber variables
	// will work fine. EHANDLEs surprisingly seemingly stay valid after this step...
	for (int i = gpGlobals->maxClients; i < m_SaveRestoreData->tableCount; i++)
	{
		ENTITYTABLE *pTable = &m_SaveRestoreData->pTable[i];
		edict_t* pent = nullptr;

		// skip invalid entries
		if ( pTable->classname == 0 || pTable->size == 0)
			continue;

		// dont restore clients!
		//if (FBitSet(pTable->flags, 0x80000000))
		//	continue;

		m_SaveRestoreData->pCurrentData = m_SaveRestoreData->pBaseData + pTable->location;
		m_SaveRestoreData->size = pTable->location;
		m_SaveRestoreData->currentIndex = i;

		/* Sigh...
		 * This is an EXTREMELY UGLY HACK to get around being unable to do this properly (recreate edicts with matching IDs),
		 * since doing index shuffling with certain entities (eg. soundent) leaves the game in an unstable state due to it's
		 * internal memory management getting messed up.
		 * This approach leaves leftover unsaved member variables in classes which is a decent-ish compromise BUT leads to some
		 * nasty problems in cases where it's an entity pointer (potential instability due to a potentially invalid/wrong pointer).
		 * The most noticable issue so far has been unsaved CBeam* pBeams variables causing problems upon removal but a hack above
		 * to ignore FCAP_DONT_SAVE for beams works around it... Very ugly and WILL cause problems later down the line
		 */
		if (INDEXENT(i) == 0 || (INDEXENT(i) && (FStrEq(STRING(pTable->classname), STRING(INDEXENT(i)->v.classname)) == 0 || pTable->pent->serialnumber != INDEXENT(i)->serialnumber)))
			pent = CREATE_NAMED_ENTITY(pTable->classname);
		else
			pent = pTable->pent;

		pTable->pent = pent;
	}

	// second pass
	// Actually dispatch restore
	for (int i = 0; i < m_SaveRestoreData->tableCount; i++)
	{
		ENTITYTABLE *pTable = &m_SaveRestoreData->pTable[i];
		edict_t* pent = nullptr;

		// skip invalid entries
		if (i == 0 || pTable->classname == 0 || pTable->size == 0)
			continue;

		m_SaveRestoreData->pCurrentData = m_SaveRestoreData->pBaseData + pTable->location;
		m_SaveRestoreData->size = pTable->location;
		m_SaveRestoreData->currentIndex = i;
		pent = pTable->pent;

		if (FNullEnt(pent) == 0)
		{
			ALERT(at_console, "dispatching restore for: %i\n", i);
			if (DispatchRestore(pent, m_SaveRestoreData, 0) < 0)
			{
				SetBits(pent->v.flags, FL_KILLME);
				pTable->pent = nullptr;
			}
			else
			{
				// force relink
				UTIL_SetOrigin(&pent->v, pent->v.origin);
				
				// TODO: What do we do with the return value?
				(void)PostHacks(pent);
			}
		}
	}

	memcpy(m_SaveRestoreData->pTable, m_pBackupTable, ((sizeof(ENTITYTABLE) * gpGlobals->maxEntities)));

	gpGlobals->pSaveData = pBackupGlobalData;
	m_State = SaveState::HAS_SAVE;

}

// Runs hacks over entities before DispatchRestore,
// during the UTIL_Remove loop. Returning true will cancel
// the removal of the entity.
bool CSaveSystem::PreHacks(edict_t* pent)
{
	// Ensure monsters & doors go quite before a restore
	if (FClassnameIs(pent, "monster_barney") || FClassnameIs(pent, "monster_scientist") ||
			FClassnameIs(pent, "monster_wheelchair") || FClassnameIs(pent, "monster_rosenberg") ||
			FClassnameIs(pent, "func_door") || FClassnameIs(pent, "func_plat") || FClassnameIs(pent, "momentary_rot_door")) // doors
		((CBaseToggle*)CBaseEntity::Instance(pent))->SentenceStop();

	// ugly hack to fix leftover CBeam pointer bug
	// TODO: Do this properly! The current solution easily duplicates beams!!!
	if (FClassnameIs(pent, "beam") && pent->v.spawnflags & SF_BEAM_TEMPORARY)
		return true;

	return false;
}

// Runs hacks after DispatchRestore, return has no effect
bool CSaveSystem::PostHacks(edict_t* pent)
{
	return false;
}
