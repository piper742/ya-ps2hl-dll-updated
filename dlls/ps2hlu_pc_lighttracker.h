#ifndef PS2HLU_LIGHTTRACKER_H
#define PS2HLU_LIGHTTRACKER_H

/*************************
 * PS2HLU
 * Light style tracker
 * Tracks the server's light styles
 * ***********************/

// Goldsrc has a max of 64 light styles
constexpr int MAX_LIGHTSTYLES = 64;

class CLightTracker {
public:
	CLightTracker();
	void Reset();
	void SetLightStyle(int style, const char* val);
	void Save();
	void Restore();

private:
	// track live data and copy for saves
	const char* m_currentStyles[MAX_LIGHTSTYLES];
	const char* m_savedStyles[MAX_LIGHTSTYLES];
};

// same as in soundent.h, provides global pointer
inline CLightTracker* pLightTracker;

#endif
