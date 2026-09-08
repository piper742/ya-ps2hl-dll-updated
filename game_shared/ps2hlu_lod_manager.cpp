#include "filesystem_utils.h"
#include "ps2hlu_lod_manager.h"
#include <istream>
#include <charconv>
#include <utility>

// Used only for Con_DPrintf
#ifdef CLIENT_DLL
#include "hud.h"
#endif

// TEMP

// TODO: Move this to filesystem_utils in case we need to convert something else
// to a stream too!
// Based off of the class from here: https://stackoverflow.com/a/67457982
class vectorbuf : public std::streambuf {
public:
    vectorbuf(std::vector<std::byte> &v){
        setg((char*)v.data(), (char*)v.data(), (char*)(v.data() + v.size()));
    }
    ~vectorbuf() {}
};

/// TEMP END

std::string CLodManager::ConvertFilename(std::string_view filename)
{
	if (filename.empty())
		return "";

	const int len = filename.length();

	//ALERT(at_console, "convert: %s, len: %i\n", filename.data(), len);
	// pretty ugly but I kinda don't care
	if (filename[len-3] == 'm' && filename[len-2] == 'd' && filename[len-1] == 'l')
	{
		std::string copy{filename};
		copy[len-3] = 'i';
		copy[len-2] = 'n';
		copy[len-1] = 'f';

		//g_engfuncs.Con_DPrintf("copy: %s\n", copy.c_str());
		//ALERT(at_console, "copy %s\n", copy.data());
		return copy;
	}

	return "";
}

bool CLodManager::LoadLODData(const char* filename, LodData_s& result)
{
	// TODO: Do we need to check g_pFileSystemModule too?
	// Realistically this check should never fail unless we
	// aren't on proprietary/steam engine (Xash does implement the
	// filesystem library but from my tests at least with my setup it
	// failed to load anything. Maybe testing from a read-only FS was the problem)
	if (g_pFileSystem == nullptr)
		return false;

	const auto buffer = FileSystem_LoadFileIntoBuffer(filename, FileContentFormat::Text, "GAMECONFIG", true);
	
	if (buffer.empty())
		return false;

	// real ugly
	vectorbuf vbuf(const_cast<std::vector<std::byte>&>(buffer));
	std::istream is(&vbuf);

	// 2 passes like in supadupaplex's version so that we can grab
	// groups & maxparts even if they're at the end of the file
	// TODO: Consider verifying that the number of entires add up to
	// the expected size? Seems unnecessary for parts considering blank
	// part entires exist, otherwise should be easy for groups.
	int numParsed = 0; // only tracks groups and maxparts!
	int numGroups = 0;
	int numParts = 0;
	int fades[2]{0};
	bool bInsideGroup = false;
	const char* pattern = "1234567890";

	// Used for emplacing to LodData_s
	LodBodyGroup_s tmpGroup{};

	for (int j = 0; j < 2; j++)
	{
		// Strip whitespace and go line by line
		for (std::string line; std::getline(is >> std::ws, line, '\n');)
		{
			// Skip empty lines
			if (line[0] == '\r' || line[0] == '\0')
				continue;

			// Skip comments
			if (line[0] == '\\' && line[1] == '\\' ||
					line[0] == '/' && line[1] == '/')
				continue;

			// Most important values to parse!
			if (line.compare(0, strlen("groups["), "groups[") == 0)
			{
				if (int end = line.find("]", strlen("groups[")); end != std::string::npos)
				{
					std::from_chars(line.data() + strlen("groups["), line.data() + end, numGroups);

					numParsed++;
				}
				continue;
			}

			if (line.compare(0, strlen("maxparts["), "maxparts[") == 0)
			{
				if (int end = line.find("]", strlen("maxparts[")); end != std::string::npos)
				{
					std::from_chars(line.data() + strlen("maxparts["), line.data() + end, numParts);
					// Reserve so we don't unnecessarily fragment
					//tmpGroup.table.reserve(numParts);
					numParsed++;
				}
				continue;
			}

			// These ones are completely optional, only item pickups seem to utilize them
			if (line.compare(0, strlen("fadestart["), "fadestart[") == 0)
			{
				if (int end = line.find("]", strlen("fadestart[")); end != std::string::npos)
				{
					std::from_chars(line.data() + strlen("fadestart["), line.data() + end, fades[0]);
				}
				continue;
			}

			if (line.compare(0, strlen("fadeend["), "fadeend[") == 0)
			{
				if (int end = line.find("]", strlen("fadeend[")); end != std::string::npos)
				{
					std::from_chars(line.data() + strlen("fadeend["), line.data() + end, fades[1]);
				}
				continue;
			}

			// Only parse these if we've already finished our first pass
			if (numParsed >= 2)
			{
				// Allow group decleration without group keyword
				// Maintain parity with supadupaplex's mdltool
				if (line[0] == '{' && line[1] != '}')
				{
					bInsideGroup = true;
					continue;
				}

				if (line[0] == '}')
				{
					bInsideGroup = false;
					result.groups.push_back(tmpGroup);
					tmpGroup.table.clear();
					continue;
				}

				if (bInsideGroup)
				{
					// Skip empty lines
					if (line == "blank")
						continue;

					if (line.compare(0, strlen("part["), "part[") == 0)
					{
						// Fill LodEntry
						LodEntry_s entry;
						int dist = 0;
						int idx = 0;
						// We add one in the loop
						std::size_t lastpos = strlen("part[");
						for (; idx < 4; idx++)
						{
							int end = line.find_first_not_of(pattern, lastpos);
							if (end == std::string::npos)
									break;

							std::from_chars(line.data() + lastpos, line.data() + end, dist);

							entry.dist[idx] = dist;

							if (line[end] == ']' || line[end] == '\0')
								break;

							lastpos = end + 1;
						}
						entry.LodCount = idx + 1;
						tmpGroup.table.push_back(entry);
					}
				}
			}
		}
	}

	// Parse was a success!
	if (numParsed >= 2)
	{
		result.MaxBodyParts = numParts;
		result.NumBodyGroups = numGroups;

		result.FadeStart = fades[0];
		result.FadeEnd = fades[1];

		// Doing this might be a bad thing since we're forcing a
		// reallocation, but really I'll just leave this be
		// I shouldn't be worrying about this (class) too much as long as it
		// doesn't tank performance, hog or corrupt memory it's fine...
		result.groups.resize(numGroups);
		return true;
	}

	return false;
}

void CLodManager::FlushCache()
{
	cache.clear();
}

const LodData_s* CLodManager::GetLODData(int modelindex, const char* modelname)
{
	// TODO: Prevent known nonexistent models from issuing filesystem calls
	if (auto found = cache.find( modelindex ); found != cache.end())
	{
		return found->second.get();
	}
	else
	{
		std::unique_ptr<LodData_s> temp = std::make_unique<LodData_s>();
		std::string filename = ConvertFilename(modelname);

		if (filename.empty())
		{
			// assume this was a fluke and someone passed us a mangled
			// modelname, let's not rule out this modelindex just yet...
			//cache[modelindex] = nullptr;
			return nullptr;
		}

		if (LoadLODData(filename.c_str(), *temp))
		{
#ifdef CLIENT_DLL
		gEngfuncs.Con_DPrintf("loading %s\n", filename.c_str());
#endif

			auto it = cache.emplace(modelindex, std::move(temp));
			return it.first->second.get();
		}
		else
		{
			// No more lookups! This doesn't exist!
			cache[modelindex] = nullptr;
			return nullptr;
		}
	}
}
