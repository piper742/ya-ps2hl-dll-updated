/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "plane.h"

//=========================================================
// Plane
//=========================================================
CPlane::CPlane()
{
	m_fInitialized = false;
}

//=========================================================
// InitializePlane - Takes a normal for the plane and a
// point on the plane and
//=========================================================
void CPlane::InitializePlane(const Vector& vecNormal, const Vector& vecPoint)
{
	m_vecNormal = vecNormal;
	m_flDist = DotProduct(m_vecNormal, vecPoint);
	m_fInitialized = true;
}


//=========================================================
// PointInFront - determines whether the given vector is
// in front of the plane.
//=========================================================
bool CPlane::PointInFront(const Vector& vecPoint)
{
	float flFace;

	if (!m_fInitialized)
	{
		return false;
	}

	flFace = DotProduct(m_vecNormal, vecPoint) - m_flDist;

	if (flFace >= 0)
	{
		return true;
	}

	return false;
}
