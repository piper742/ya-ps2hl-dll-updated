#include "ps2hlu_pc_lighttracker.h"
#include "extdll.h"
#include "util.h"

CLightTracker::CLightTracker() {
	// Initialize arrays
	for (int i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		m_currentStyles[i] = "";
		m_savedStyles[i] = "";
	}
}

void CLightTracker::Reset()
{
	// very ugly
	CLightTracker();
}

void CLightTracker::SetLightStyle(int style, const char* val)
{
	if (pLightTracker == 0)
		return;

	if (style >= 0 && style < MAX_LIGHTSTYLES)
	{
		m_currentStyles[style] = val;
	}
}

void CLightTracker::Save()
{
	memcpy(&m_savedStyles, &m_currentStyles, sizeof(const char*) * MAX_LIGHTSTYLES);
}

void CLightTracker::Restore()
{
	for (int i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		LIGHT_STYLE_ENGINE(i, m_savedStyles[i]);
	}

	memcpy(&m_currentStyles, &m_savedStyles, sizeof(const char*) * MAX_LIGHTSTYLES);
}
