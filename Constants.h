////////////////////////////////////////////////////////////////////////////////
//
// File: Constants.h
//
// Constants (table size, physics values, ball colors) and small helper
// functions used by all classes.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ConstantsH__
#define __ConstantsH__

#include "d3dUtility.h"

#define M_RADIUS 0.21f      // ball radius
#define PI 3.14159265f
#define M_HEIGHT 0.01f
#define DECREASE_RATE 0.9982

// table size (the inner side of the cushions)
const float TABLE_HALF_X = 4.5f;
const float TABLE_HALF_Z = 2.25f;
const float WALL_THICK   = 0.12f;

// pockets
const float CORNER_GAP    = 0.40f;   // a cushion stops this far from a corner
const float SIDE_GAP      = 0.30f;   // half width of the side pocket opening
const float CORNER_POCKET = 0.32f;   // radius of a corner pocket
const float SIDE_POCKET   = 0.28f;   // radius of a side pocket

// physics
const float TIME_SCALE  = 3.3f;      // same value as the sample code
const float BALL_BOUNCE = 0.95f;     // ball - ball   (1.0 = perfectly elastic)
const float WALL_BOUNCE = 0.80f;     // ball - cushion
const float MAX_POWER   = 6.0f;      // maximum shot speed
const float STOP_SPEED  = 0.06f;     // a ball slower than this stops

// ball groups
const int GROUP_NONE   = 0;
const int GROUP_SOLID  = 1;
const int GROUP_STRIPE = 2;

// state of a ball
const int BALL_ON_TABLE = 0;
const int BALL_IN_TRAY  = 1;   // pocketed object ball, shown next to the table
const int BALL_HIDDEN   = 2;   // pocketed cue ball, waiting to be put back

// colors of balls 1~8 (balls 9~15 use the colors of 1~7)
const D3DXCOLOR BALL_COLOR[9] = {
	D3DXCOLOR(1.00f, 1.00f, 1.00f, 1.0f),   // 0 : cue ball (white)
	D3DXCOLOR(1.00f, 0.85f, 0.00f, 1.0f),   // 1 : yellow
	D3DXCOLOR(0.10f, 0.25f, 0.90f, 1.0f),   // 2 : blue
	D3DXCOLOR(0.90f, 0.10f, 0.10f, 1.0f),   // 3 : red
	D3DXCOLOR(0.45f, 0.15f, 0.65f, 1.0f),   // 4 : purple
	D3DXCOLOR(1.00f, 0.50f, 0.00f, 1.0f),   // 5 : orange
	D3DXCOLOR(0.00f, 0.55f, 0.25f, 1.0f),   // 6 : green
	D3DXCOLOR(0.55f, 0.10f, 0.10f, 1.0f),   // 7 : maroon
	D3DXCOLOR(0.05f, 0.05f, 0.05f, 1.0f)    // 8 : black
};

// clamp v into [lo, hi]
inline float clampf(float v, float lo, float hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

// fill a material with one color (same style as the sample code)
inline void setMaterial(D3DMATERIAL9& mtrl, D3DXCOLOR color)
{
	ZeroMemory(&mtrl, sizeof(mtrl));
	mtrl.Ambient  = color;
	mtrl.Diffuse  = color;
	mtrl.Specular = color;
	mtrl.Emissive = d3d::BLACK;
	mtrl.Power    = 5.0f;
}

// group of a ball number: 1~7 solid, 9~15 stripe, 0 and 8 none
inline int groupOf(int number)
{
	if (number >= 1 && number <= 7)  return GROUP_SOLID;
	if (number >= 9 && number <= 15) return GROUP_STRIPE;
	return GROUP_NONE;
}

#endif // __ConstantsH__
