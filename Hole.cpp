////////////////////////////////////////////////////////////////////////////////
//
// File: Hole.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include "Hole.h"
#include "Sphere.h"

CHole::CHole(void)
{
	D3DXMatrixIdentity(&m_mLocal);
	ZeroMemory(&m_mtrl, sizeof(m_mtrl));
	m_x = 0;
	m_z = 0;
	m_radius = 0;
	m_pMesh = NULL;
}

bool CHole::create(IDirect3DDevice9* pDevice, float x, float z, float radius)
{
	if (NULL == pDevice)
		return false;

	m_x = x;
	m_z = z;
	m_radius = radius;
	setMaterial(m_mtrl, d3d::BLACK);

	// a flat black disk on the cloth
	if (FAILED(D3DXCreateCylinder(pDevice, radius, radius, 0.04f, 30, 1, &m_pMesh, NULL)))
		return false;

	D3DXMATRIX mRot, mTrans;
	D3DXMatrixRotationX(&mRot, PI / 2);          // z axis -> y axis
	D3DXMatrixTranslation(&mTrans, x, 0.0f, z);
	m_mLocal = mRot * mTrans;
	return true;
}

void CHole::destroy(void)
{
	if (m_pMesh != NULL) {
		m_pMesh->Release();
		m_pMesh = NULL;
	}
}

void CHole::draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
{
	if (NULL == pDevice)
		return;
	pDevice->SetTransform(D3DTS_WORLD, &mWorld);
	pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
	pDevice->SetMaterial(&m_mtrl);
	m_pMesh->DrawSubset(0);
}

// the ball falls in when its center is inside the pocket circle
bool CHole::hasIntersected(CSphere& ball)
{
	if (!ball.isOnTable()) return false;
	D3DXVECTOR3 c = ball.getCenter();
	float dx = c.x - m_x;
	float dz = c.z - m_z;
	return (dx * dx + dz * dz) < (m_radius * m_radius);
}

void CHole::hitBy(CSphere& ball)
{
	if (hasIntersected(ball))
		ball.fallIntoPocket();   // virtual : CSphere or CCueBall
}
