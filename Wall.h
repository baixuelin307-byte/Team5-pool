////////////////////////////////////////////////////////////////////////////////
//
// File: Wall.h
//
// CWall : a box. Used for the table cloth, the cushions (balls bounce on
//         them), the wooden frame and the tray.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __WallH__
#define __WallH__

#include "GameObject.h"
#include "Constants.h"

class CSphere;

class CWall : public CGameObject {
private:
	float					m_x;
	float					m_z;
	float                   m_width;
	float                   m_depth;
	float					m_height;

public:
	CWall(void);
	virtual ~CWall(void) {}

	bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE);
	virtual void destroy(void);
	virtual void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld);

	bool hasIntersected(CSphere& ball);   // does the ball touch this wall?
	void hitBy(CSphere& ball);            // bounce the ball

	void setPosition(float x, float y, float z);
	float getHeight(void) const { return M_HEIGHT; }

private :
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh*              m_pBoundMesh;
};

#endif // __WallH__
