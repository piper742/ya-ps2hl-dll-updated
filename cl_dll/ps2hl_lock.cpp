/*

20.10.18

PS2HL weapon lock sprite

Created by supadupaplex

*/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "triangleapi.h"

#include <string.h>
#include <stdio.h>
#include <cmath>
#include <iostream>

#include "parsemsg.h"

DECLARE_MESSAGE(m_HudLock, LockOffs);

extern Vector v_origin;
extern Vector v_angles;

// cvar reference
extern cvar_t * cl_ps2hl_oldsights;

bool CHudLock::Init(void)
{
	//m_fPrevX = m_fPrevY = m_iOffX = m_iOffY = m_iActive = 0;
	Reset();
	HOOK_MESSAGE(LockOffs);

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return true;
};

void CHudLock::Reset(void)
{
	m_fX = (ScreenWidth / 2) - ((m_rcCrosshair.right - m_rcCrosshair.left) / 2.0f);
	m_fY = (ScreenHeight / 2) + ((m_rcCrosshair.bottom - m_rcCrosshair.top) / 2.0f);
	m_iActive = 0;
	framesRemaining = 1;
}

void CHudLock::SetSprite(HSPRITE hspr, Rect rc)
{
	m_hLock = hspr;
	m_rcLock = rc;
}

void CHudLock::SetState(int iState)
{
	m_iActive = iState;
}

// PS2HLU
// Crosshair drawing was moved here
void CHudLock::SetCrosshair(HSPRITE sprite, Rect rect, Vector color)
{
	m_hCrosshair = sprite;
	m_rcCrosshair = rect;
	vecCrosshairColor = color;
}

bool CHudLock::MsgFunc_LockOffs(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	autoaimAngle.x = READ_COORD();
	autoaimAngle.y = READ_COORD();

	return true;
}

bool CHudLock::VidInit(void)
{
	m_hLock = 0;
	m_rcLock.top = 0;
	m_rcLock.bottom = 0;
	m_rcLock.left = 0;
	m_rcLock.right = 0;

	return 1;
};

bool CHudLock::Draw(float flTime)
{
	static Rect nullrc;
	
	// Do not draw if HUD is disabled
	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_ALL | HIDEHUD_WEAPONS))
	{
		SetCrosshair(0, nullrc, {0, 0, 0}); // Ghost crosshair bug fix
	}
	
	// Do not draw if suit isn't equipped
	if (!(gHUD.m_iWeaponBits & (1 << WEAPON_SUIT)))
	{
		SetCrosshair(0, nullrc, {0, 0, 0}); // Ghost crosshair bug fix
		//return 1; // This is commented to match the PS2 version
	}

	// Do not draw if there are no weapons
	if (!(gHUD.m_iWeaponBits & ~(1 << (WEAPON_SUIT))))
	{
		SetCrosshair(0, nullrc, {0, 0, 0}); // Ghost crosshair bug fix;
	}
	
	// Do not draw if old crosshairs are enabled
	if (cl_ps2hl_oldsights->value != 0)
		return 1;

	int r, g, b, a;

	const int staticCrosshairWidth = (m_rcCrosshair.right - m_rcCrosshair.left) / 2;
	const int staticCrosshairHeight = (m_rcCrosshair.bottom - m_rcCrosshair.top) / 2;

	const int staticCrosshairX = ScreenWidth / 2 - staticCrosshairWidth;
	const int staticCrosshairY = ScreenHeight / 2 - staticCrosshairHeight;

	// PS2HLU
	// the crosshair in PS2HL is actually the autoaim "lock", the "real" crosshair is always drawn
	if (m_hCrosshair)
	{
		SPR_Set(m_hCrosshair, vecCrosshairColor.x, vecCrosshairColor.y, vecCrosshairColor.z);
		gEngfuncs.pfnSPR_DrawHoles(0, staticCrosshairX, staticCrosshairY, &m_rcCrosshair);
	}

	if (gHUD.m_pCvarCrosshair->value && gHUD.HasAnyWeapons())
	{
		// Set coordinates
		const int Width = (m_rcLock.right - m_rcLock.left);
		const int Height = (m_rcLock.bottom - m_rcLock.top);

		const Vector angles = v_angles + autoaimAngle;
		Vector forward;
		AngleVectors(angles, forward, nullptr, nullptr);

		Vector point = v_origin + forward;
		Vector screen;
		gEngfuncs.pTriAPI->WorldToScreen(point, screen);

		// Round this way to prevent jittering, but dont remove autoaim angle
		screen = screen * 1000;
		screen[0] = std::round(screen[0]);
		screen[1] = std::round(screen[1]);
		screen = screen / 1000;

		int adjustedX = XPROJECT(screen[0]);
		int adjustedY = YPROJECT(screen[1]);

		adjustedX -= (m_rcLock.right - m_rcLock.left) / 2;
		adjustedY -= (m_rcLock.bottom - m_rcLock.top) / 2;

		if (adjustedX != m_fTargetX || adjustedY != m_fTargetY)
		{
			framesRemaining = 5;
		}

		const float xStep = (adjustedX - m_fX) / 5.0f;
		const float yStep = (adjustedY - m_fY) / 5.0f;

		m_fTargetX = adjustedX;
		m_fTargetY = adjustedY;

		if (framesRemaining > 0)
		{
			m_fX += xStep;
			m_fY += yStep;

			framesRemaining--;

			if (framesRemaining == 0)
			{
				m_fX = adjustedX;
				m_fY = adjustedY;
			}
		}

		// Do not draw empty sprite
		if (Width == 0 || Height == 0)
			return 1;

		// Set color
		a = 225;
		if (m_iActive >= 64)
		{
			if (m_iActive == 64)
				UnpackRGB(r, g, b, RGB_REDISH);
			else
				UnpackRGB(r, g, b, RGB_GREENISH);
		}
		else
			UnpackRGB(r, g, b, gHUD.HudColor);
		ScaleColors(r, g, b, a);

		// Draw
		SPR_Set(m_hLock, r, g, b);
		// SPR_DrawAdditive(0, adjustedX, adjustedY, &m_rcLock);
		SPR_DrawAdditive(0, m_fX, m_fY, &m_rcLock);
	}

	return 1;
}
