////////////////////////////////////////////////////////////////////////////////
//
// File: Hole.h
//
// CHole : a pocket (the same idea as THole in Chapter 7 of the textbook)
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __HoleH__
#define __HoleH__

#include "GameObject.h"
#include "Constants.h"

class CSphere;

class CHole : public CGameObject {
private:
	float           m_x;
	float           m_z;
	float           m_radius;
	D3DXMATRIX      m_mLocal;
	D3DMATERIAL9    m_mtrl;
	ID3DXMesh*      m_pMesh;

public:
	CHole(void);
	virtual ~CHole(void) {}

	bool create(IDirect3DDevice9* pDevice, float x, float z, float radius);
	virtual void destroy(void);
	virtual void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld);

	bool hasIntersected(CSphere& ball);   // is the ball center inside the pocket?
	void hitBy(CSphere& ball);            // the ball falls in
};

#endif // __HoleH__
