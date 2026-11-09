////////////////////////////////////////////////////////////////////////////////
//
// File: GameObject.h
//
// CGameObject     : abstract base class of everything drawn on the screen
// CPocketListener : interface. A ball uses it to tell the game
//                   "I fell into a pocket" (CGameManager implements it).
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __GameObjectH__
#define __GameObjectH__

#include "d3dUtility.h"

class CSphere;

class CPocketListener {
public:
	virtual ~CPocketListener(void) {}
	virtual void onBallPocketed(CSphere* ball) = 0;
};

class CGameObject {
public:
	virtual ~CGameObject(void) {}
	virtual void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld) = 0;
	virtual void destroy(void) = 0;
};

#endif // __GameObjectH__
