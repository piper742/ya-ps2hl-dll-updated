#ifndef PS2HLU_SAVESYSTEM_H
#define PS2HLU_SAVESYSTEM_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"

/**************************
 * PS2HLU
 * Checkpoint system for multiplayer HEAVILY based off of Xash3D-FWGS's
 * save/restore code, with a bunch of hacks piled on top to make it semi-stable
 * for same session multiplayer. Currently still work in progress.
 * Due to the way edicts are handled on restore much data is leftover from the
 * previous game state, which leads to softlocks in many missions.
 *
 * References:
 * sv_save.c from: https://github.com/FWGS/xash3d-fwgs/blob/3e24d5a347a4f0417292cbc75ee4a628f2091f6c/engine/server/sv_save.c
 * sv_game.c from: https://github.com/FWGS/xash3d-fwgs/blob/3e24d5a347a4f0417292cbc75ee4a628f2091f6c/engine/server/sv_game.c
 *************************/

// TODO: LIGHT_STYLE tracker to fix big visual bugs
// TODO: Decal tracker if I have the time, for players who join the session after
// infodecal's have sent their messages.

typedef struct {
	int skillLevel;
	int entityCount;
	int connectionCount; // always zero
	int lightStyleCount; // never restored
	float time; // NEVER OVERRIDE WITH VALUE! Only use to correct thinking for entities!
	char mapName[32];
	char skyName[32];
	int skyColor_r;
	int skyColor_g;
	int skyColor_b;
	float skyVec_x;
	float skyVec_y;
	float skyVec_z;
} SAVE_HEADER;

enum class SaveState : unsigned char
{
	NONE = 0, // we're idle
	HAS_SAVE, // we've got a save ready to go
	SAVING, // we're making a checkpoint
	LOADING, // we're loading said checkpoint
};

class CSaveSystem {
public:
		// Very un-C++-y solution with memory management here,
		// maybe will fix later.
		CSaveSystem();
		~CSaveSystem();

		void Save();
		void Restore();
	
private:
		void ResetMem();
		void ResetPtr();
		bool PreHacks(edict_t* pent);
		bool PostHacks(edict_t* pent);
		void WriteHeader();
		void ReadHeader();

		SAVE_HEADER m_SaveHeader;
		SaveState m_State = SaveState::NONE;
		SAVERESTOREDATA* m_SaveRestoreData;
		SAVERESTOREDATA* pBackupGlobalData;
		ENTITYTABLE* m_pBackupTable;
};

#endif
