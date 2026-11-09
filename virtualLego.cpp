///////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: Chang-hyeon Park,
// Modified by Bong-Soo Sohn and Dong-Jun Kim
//
// Originally programmed for Virtual LEGO.
// Modified later to program for Virtual Billiard.
//
// Team 5 : extended the sample code into a simple 8-ball pool game
//
// This file : global objects, Setup / Display / Cleanup, input (WndProc), WinMain.
// Classes are in their own files:
//   Constants.h      constants and helper functions
//   GameObject.h     CGameObject (abstract base class), CPocketListener (interface)
//   Sphere.h/.cpp    CSphere (ball), CCueBall (white ball)
//   Wall.h/.cpp      CWall (cloth, cushions, frame)
//   Hole.h/.cpp      CHole (pocket)
//   Light.h/.cpp     CLight
//   GameManager.h/.cpp  CGameManager (rules, turns, text on the screen)
//   - 6 pockets, 1 cue ball + 15 object balls (solids 1~7, 8-ball, stripes 9~15)
//   - two players take turns, SOLID / STRIPE groups, fouls, win / lose
//
// Controls
//   right mouse drag : move the blue ball (aiming target)
//   left mouse drag  : rotate the table
//   hold SPACE       : charge shot power (2 seconds to 100%)
//   release SPACE    : shoot toward the blue ball using charged power
//                      (blue ball controls direction only)
//   V                : change view (perspective / top view)
//   R                : restart the game
//   ENTER            : wireframe on / off,   ESC : quit
//
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include "Constants.h"
#include "Sphere.h"
#include "Wall.h"
#include "Hole.h"
#include "Light.h"
#include "GameManager.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cmath>

IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
const int NUM_BALLS = 16;

CWall       g_legoPlane;          // green cloth
CWall       g_cushion[6];         // cushions (balls bounce on these)
CWall       g_frame[4];           // wooden frame (only for drawing)
CWall       g_tray;               // tray for pocketed balls
CHole       g_hole[6];            // pockets
CCueBall    g_cueBall;
CSphere     g_objectBall[15];
CSphere* g_balls[NUM_BALLS];   // g_balls[n] = ball number n
CSphere     g_target_blueball;
CLight      g_light;
CGameManager g_game;

// every object that is drawn every frame. Because they all inherit
// CGameObject, one loop can draw all of them (polymorphism).
std::vector<CGameObject*> g_drawList;

bool g_topView = false;

// NEW: hold SPACE to charge, release SPACE to shoot.
const DWORD SHOT_CHARGE_TIME_MS = 2000;
bool g_spaceCharging = false;
DWORD g_chargeStartMs = 0;

float currentShotCharge(void)
{
	if (!g_spaceCharging) return 0.0f;

	// DWORD subtraction remains valid if GetTickCount wraps around.
	DWORD elapsedMs = GetTickCount() - g_chargeStartMs;
	float fraction = (float)elapsedMs / (float)SHOT_CHARGE_TIME_MS;
	return (fraction > 1.0f) ? 1.0f : fraction;
}

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

void setCamera(void)
{
	// top view : look straight down, the far side of the table (+z) is up on the screen
	D3DXVECTOR3 pos = g_topView ? D3DXVECTOR3(0.0f, 13.0f, 0.0f) : D3DXVECTOR3(0.0f, 8.0f, -10.5f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up = g_topView ? D3DXVECTOR3(0.0f, 0.0f, 1.0f) : D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);
}

void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup()
{
	int i;

	D3DXMatrixIdentity(&g_mWorld);
	D3DXMatrixIdentity(&g_mView);
	D3DXMatrixIdentity(&g_mProj);

	const D3DXCOLOR CLOTH(0.0f, 0.38f, 0.16f, 1.0f);
	const D3DXCOLOR CUSHION(0.0f, 0.28f, 0.12f, 1.0f);
	const D3DXCOLOR WOOD(0.32f, 0.15f, 0.05f, 1.0f);

	const float HX = TABLE_HALF_X;
	const float HZ = TABLE_HALF_Z;
	const float T = WALL_THICK;

	// create plane and set the position
	if (false == g_legoPlane.create(Device, -1, -1, 2 * (HX + T), 0.03f, 2 * (HZ + T), CLOTH)) return false;
	g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

	// six cushions with openings for the pockets
	float longLen = HX - CORNER_GAP - SIDE_GAP;       // one half of a long cushion
	float longMidX = (HX - CORNER_GAP + SIDE_GAP) / 2;  // its center
	float shortLen = 2 * (HZ - CORNER_GAP);

	if (false == g_cushion[0].create(Device, -1, -1, longLen, 0.3f, T, CUSHION)) return false;
	g_cushion[0].setPosition(-longMidX, 0.12f, HZ + T / 2);
	if (false == g_cushion[1].create(Device, -1, -1, longLen, 0.3f, T, CUSHION)) return false;
	g_cushion[1].setPosition(longMidX, 0.12f, HZ + T / 2);
	if (false == g_cushion[2].create(Device, -1, -1, longLen, 0.3f, T, CUSHION)) return false;
	g_cushion[2].setPosition(-longMidX, 0.12f, -(HZ + T / 2));
	if (false == g_cushion[3].create(Device, -1, -1, longLen, 0.3f, T, CUSHION)) return false;
	g_cushion[3].setPosition(longMidX, 0.12f, -(HZ + T / 2));
	if (false == g_cushion[4].create(Device, -1, -1, T, 0.3f, shortLen, CUSHION)) return false;
	g_cushion[4].setPosition(HX + T / 2, 0.12f, 0.0f);
	if (false == g_cushion[5].create(Device, -1, -1, T, 0.3f, shortLen, CUSHION)) return false;
	g_cushion[5].setPosition(-(HX + T / 2), 0.12f, 0.0f);

	// wooden frame around the table
	const float F = 0.35f;   // frame thickness
	if (false == g_frame[0].create(Device, -1, -1, 2 * (HX + T + F), 0.32f, F, WOOD)) return false;
	g_frame[0].setPosition(0.0f, 0.1f, HZ + T + F / 2);
	if (false == g_frame[1].create(Device, -1, -1, 2 * (HX + T + F), 0.32f, F, WOOD)) return false;
	g_frame[1].setPosition(0.0f, 0.1f, -(HZ + T + F / 2));
	if (false == g_frame[2].create(Device, -1, -1, F, 0.32f, 2 * (HZ + T), WOOD)) return false;
	g_frame[2].setPosition(HX + T + F / 2, 0.1f, 0.0f);
	if (false == g_frame[3].create(Device, -1, -1, F, 0.32f, 2 * (HZ + T), WOOD)) return false;
	g_frame[3].setPosition(-(HX + T + F / 2), 0.1f, 0.0f);

	// tray for the pocketed balls (in front of the table)
	if (false == g_tray.create(Device, -1, -1, 8.0f, 0.05f, 0.6f, WOOD)) return false;
	g_tray.setPosition(0.0f, -0.03f, -(HZ + 1.0f));

	// six pockets : 4 corners + 2 in the middle of the long sides
	const float pocketPos[6][2] = {
		{ -HX - 0.05f,  HZ + 0.05f }, { HX + 0.05f,  HZ + 0.05f },
		{ -HX - 0.05f, -HZ - 0.05f }, { HX + 0.05f, -HZ - 0.05f },
		{ 0.0f,  HZ + 0.08f }, { 0.0f, -HZ - 0.08f } };
	for (i = 0; i < 6; i++) {
		float r = (i < 4) ? CORNER_POCKET : SIDE_POCKET;
		if (false == g_hole[i].create(Device, pocketPos[i][0], pocketPos[i][1], r)) return false;
	}

	// create the balls (ball number n is g_balls[n])
	g_balls[0] = &g_cueBall;
	if (false == g_cueBall.create(Device, 0, BALL_COLOR[0])) return false;
	for (i = 1; i < NUM_BALLS; i++) {
		g_balls[i] = &g_objectBall[i - 1];
		int colorIndex = (i <= 8) ? i : i - 8;    // 9~15 use the colors of 1~7
		if (false == g_balls[i]->create(Device, i, BALL_COLOR[colorIndex])) return false;
	}

	// create blue ball for set direction
	if (false == g_target_blueball.create(Device, 0, d3d::BLUE)) return false;
	g_target_blueball.setCenter(0.0f, (float)M_RADIUS, 0.0f);

	// game rules and text
	if (false == g_game.create(Device, g_balls, NUM_BALLS)) return false;
	g_game.reset();

	// draw list
	g_drawList.push_back(&g_legoPlane);
	for (i = 0; i < 6; i++) g_drawList.push_back(&g_cushion[i]);
	for (i = 0; i < 4; i++) g_drawList.push_back(&g_frame[i]);
	g_drawList.push_back(&g_tray);
	for (i = 0; i < 6; i++) g_drawList.push_back(&g_hole[i]);
	for (i = 0; i < NUM_BALLS; i++) g_drawList.push_back(g_balls[i]);

	// light setting
	D3DLIGHT9 lit;
	::ZeroMemory(&lit, sizeof(lit));
	lit.Type = D3DLIGHT_POINT;
	lit.Diffuse = d3d::WHITE;
	lit.Specular = d3d::WHITE * 0.9f;
	lit.Ambient = d3d::WHITE * 0.9f;
	lit.Position = D3DXVECTOR3(0.0f, 5.0f, 0.0f);
	lit.Range = 100.0f;
	lit.Attenuation0 = 1.1f;    // the table is bigger than in the sample,
	lit.Attenuation1 = 0.04f;   // so the light should not fade so fast
	lit.Attenuation2 = 0.0f;
	if (false == g_light.create(Device, lit))
		return false;

	// Position and aim the camera.
	setCamera();

	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
		(float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

	// Set render states.
	Device->SetRenderState(D3DRS_LIGHTING, TRUE);
	Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	Device->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(35, 35, 35));

	g_light.setLight(Device, g_mWorld);
	return true;
}

void Cleanup(void)
{
	for (size_t i = 0; i < g_drawList.size(); i++)
		g_drawList[i]->destroy();
	g_drawList.clear();
	g_target_blueball.destroy();
	destroyAllLegoBlock();
	g_light.destroy();
	g_game.destroy();
}

// move all balls by a small time step and handle all collisions
void physicsStep(float timeDelta)
{
	int i, j;

	for (i = 0; i < NUM_BALLS; i++)
		g_balls[i]->ballUpdate(timeDelta);

	// ball - ball
	for (i = 0; i < NUM_BALLS; i++) {
		for (j = i + 1; j < NUM_BALLS; j++) {
			if (g_balls[i]->hasIntersected(*g_balls[j])) {
				g_game.recordContact(i, j);
				g_balls[i]->hitBy(*g_balls[j]);
			}
		}
	}

	// ball - pocket (checked before the cushions)
	for (i = 0; i < 6; i++)
		for (j = 0; j < NUM_BALLS; j++)
			g_hole[i].hitBy(*g_balls[j]);

	// ball - cushion
	for (i = 0; i < 6; i++)
		for (j = 0; j < NUM_BALLS; j++)
			g_cushion[i].hitBy(*g_balls[j]);

	// safety : a ball that went through an opening but missed the pocket
	for (j = 0; j < NUM_BALLS; j++) {
		if (!g_balls[j]->isOnTable()) continue;
		D3DXVECTOR3 c = g_balls[j]->getCenter();
		if (fabsf(c.x) > TABLE_HALF_X + 0.25f || fabsf(c.z) > TABLE_HALF_Z + 0.25f)
			g_balls[j]->fallIntoPocket();
	}
}

// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
	int i = 0;

	if (Device)
	{
		// NEW: refresh power bar on every frame while SPACE is held.
		if (g_spaceCharging && g_game.canShoot())
			g_game.setShotPower(currentShotCharge());

		// update the balls only while a shot is in progress
		if (g_game.isRolling()) {
			// a fast ball could jump over another ball in one frame,
			// so one frame is divided into several small steps
			float maxSpeed = 0;
			for (i = 0; i < NUM_BALLS; i++)
				if (g_balls[i]->isOnTable() && g_balls[i]->getSpeed() > maxSpeed)
					maxSpeed = g_balls[i]->getSpeed();
			int steps = 1 + (int)(maxSpeed * TIME_SCALE * timeDelta / (M_RADIUS * 0.5f));
			if (steps > 30) steps = 30;
			for (i = 0; i < steps; i++)
				physicsStep(timeDelta / steps);

			g_game.checkShotEnd();
		}

		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();

		// draw everything with one loop (each object knows how to draw itself)
		for (i = 0; i < (int)g_drawList.size(); i++)
			g_drawList[i]->draw(Device, g_mWorld);

		// the blue ball is shown only while the player is aiming
		if (g_game.canShoot())
			g_target_blueball.draw(Device, g_mWorld);
		g_light.draw(Device);

		g_game.drawHUD(Device);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture(0, NULL);
	}
	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
	static int old_x = 0;
	static int old_y = 0;
	static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

	switch (msg) {
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam) {
		case VK_ESCAPE:
			::DestroyWindow(hwnd);
			break;
		case VK_RETURN:
			if (NULL != Device) {
				wire = !wire;
				Device->SetRenderState(D3DRS_FILLMODE,
					(wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
			}
			break;
		case 'R':
			g_spaceCharging = false;
			g_game.reset();
			break;
		case 'V':
			g_topView = !g_topView;
			D3DXMatrixIdentity(&g_mWorld);   // also undo the rotation of the table
			setCamera();
			break;
		case VK_SPACE:
			// Only start once; ignore Windows key-repeat messages.
			if (g_game.canShoot() && !g_spaceCharging) {
				g_spaceCharging = true;
				g_chargeStartMs = GetTickCount();
				g_game.setShotPower(0.0f);
			}
			break;
		}
		return 0;
	}

	case WM_KEYUP:
		if (wParam == VK_SPACE) {
			if (g_spaceCharging) {
				// Capture charge before clearing the key-held state.
				float charge = currentShotCharge();
				g_spaceCharging = false;

				if (g_game.canShoot()) {
					D3DXVECTOR3 targetpos = g_target_blueball.getCenter();
					D3DXVECTOR3 whitepos = g_cueBall.getCenter();
					double dx = targetpos.x - whitepos.x;
					double dz = targetpos.z - whitepos.z;
					double distance = sqrt(dx * dx + dz * dz);

					if (distance > 0.01 && charge > 0.0f) {
						// Blue target controls direction; held SPACE controls speed.
						double power = (double)MAX_POWER * (double)charge;
						g_cueBall.setPower(dx / distance * power,
							dz / distance * power);
						g_game.startShot(); // also resets the HUD power to 0
					}
					else {
						g_game.setShotPower(0.0f);
					}
				}
				else {
					g_game.setShotPower(0.0f);
				}
			}
			return 0;
		}
		break;

	case WM_KILLFOCUS:
		// Never leave the power meter charging after switching windows.
		g_spaceCharging = false;
		g_game.setShotPower(0.0f);
		break;

	case WM_MOUSEMOVE:
	{
		int new_x = LOWORD(lParam);
		int new_y = HIWORD(lParam);
		float dx;
		float dy;

		if (LOWORD(wParam) & MK_LBUTTON) {

			if (isReset) {
				isReset = false;
			}
			else {
				D3DXMATRIX mX;
				D3DXMATRIX mY;

				switch (move) {
				case WORLD_MOVE:
					dx = (old_x - new_x) * 0.01f;
					dy = (old_y - new_y) * 0.01f;
					D3DXMatrixRotationY(&mX, dx);
					D3DXMatrixRotationX(&mY, dy);
					g_mWorld = g_mWorld * mX * mY;
					break;
				default:
					break;
				}
			}

			old_x = new_x;
			old_y = new_y;

		}
		else {
			isReset = true;

			if (LOWORD(wParam) & MK_RBUTTON) {
				dx = (float)(old_x - new_x);
				dy = (float)(old_y - new_y);

				// move the blue ball, but keep it on the table
				D3DXVECTOR3 coord3d = g_target_blueball.getCenter();
				float nx = clampf(coord3d.x + dx * (-0.007f), -TABLE_HALF_X, TABLE_HALF_X);
				float nz = clampf(coord3d.z + dy * 0.007f, -TABLE_HALF_Z, TABLE_HALF_Z);
				if (g_topView) nz = clampf(coord3d.z + dy * 0.012f, -TABLE_HALF_Z, TABLE_HALF_Z);
				g_target_blueball.setCenter(nx, coord3d.y, nz);
			}
			old_x = new_x;
			old_y = new_y;

			move = WORLD_MOVE;
		}
		break;
	}
	}

	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd)
{
	srand(static_cast<unsigned int>(time(NULL)));

	if (!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}