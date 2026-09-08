#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

#include "Platform.h"

struct LodEntry_s {
	// Number of LODs for this submodel excluding the full quality mesh (LOD0)
	unsigned long LodCount;

	unsigned long dist[4];

	LodEntry_s() : LodCount(0) {};
};

struct LodBodyGroup_s {
	std::vector<LodEntry_s> table;
};

struct LodData_s {
	// These have nothing to do with the ones in
	// mstudiomodel_t, they're for LOD only!!!
	unsigned char MaxBodyParts;
	unsigned char NumBodyGroups;

	// TODO: Do we want this?
	// This effect is seemingly extremely rarely used
	// (only for pickups) so it might just be hindering
	// multiplayer matches on large open maps (eg. xen_dm) by hiding them
	unsigned long FadeStart;
	unsigned long FadeEnd;

	std::vector<LodBodyGroup_s> groups;

	LodData_s() : MaxBodyParts(0), NumBodyGroups(0), FadeStart(0), FadeEnd(0), groups(0) {};
};

class CLodManager
{
public:
	CLodManager(const CLodManager&) = delete;
	CLodManager& operator=(const CLodManager&) = delete;

	static CLodManager& getInstance() {
		static CLodManager _instance;
		return _instance;
	};

	// Retrieves LOD data from cache, if that fails,
	// then it is loaded and parsed from disk then added to the cache
	// failiure returns nullptr
	const LodData_s* GetLODData(int modelindex, const char* modelname);

	// Flushes the cache. Needs to be done on client on every gamemode
	// switch due to Decay/custom server's potentially having custom model overrides
	// and those potentially conflicting with the modelindex indexed cache
	// Can also be manually done with the "flushlodcache" client command
	void FlushCache();

private:
	CLodManager() {}

	// verifies and converts .mdl to .inf ending
	// returns empty string on fail
	std::string ConvertFilename(std::string_view filename);
	// parses supadupaplex's mdltool's .inf output and returns
	// a filled in LodData_s.
	bool LoadLODData(const char* filename, LodData_s& result);

	// Cache
	std::unordered_map<int, std::unique_ptr<LodData_s>> cache;
};
