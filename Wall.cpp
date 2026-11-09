////////////////////////////////////////////////////////////////////////////////
//
// File: Wall.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include "Wall.h"
#include "Sphere.h"
#include <cmath>

CWall::CWall(void)
{
	D3DXMatrixIdentity(&m_mLocal);
	ZeroMemory(&m_mtrl, sizeof(m_mtrl));
	m_x = 0;
	m_z = 0;
	m_width = 0;
	m_depth = 0;
	m_height = 0;
	m_pBoundMesh = NULL;
}

bool CWall::create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color)
{
	if (NULL == pDevice)
		return false;

	setMaterial(m_mtrl, color);

	m_width = iwidth;
	m_depth = idepth;
	m_height = iheight;

	if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
		return false;
	return true;
}

void CWall::destroy(void)
{
	if (m_pBoundMesh != NULL) {
		m_pBoundMesh->Release();
		m_pBoundMesh = NULL;
	}
}

void CWall::draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
{
	if (NULL == pDevice)
		return;
	pDevice->SetTransform(D3DTS_WORLD, &mWorld);
	pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
	pDevice->SetMaterial(&m_mtrl);
	m_pBoundMesh->DrawSubset(0);
}

// A wall is a box centered at (m_x, m_z) with size m_width (x) by m_depth (z).
// The ball touches the wall when the point of the box closest to the ball
// center is within one radius of the center.
bool CWall::hasIntersected(CSphere& ball)
{
	if (!ball.isOnTable()) return false;
	D3DXVECTOR3 c = ball.getCenter();
	float cx = clampf(c.x, m_x - m_width / 2.0f, m_x + m_width / 2.0f);   // closest point
	float cz = clampf(c.z, m_z - m_depth / 2.0f, m_z + m_depth / 2.0f);
	float dx = c.x - cx;
	float dz = c.z - cz;
	float r = ball.getRadius();
	return (dx * dx + dz * dz) < (r * r);
}

// Bounce: push the ball out of the wall and reflect the velocity
// component that goes into the wall. Using the closest point also makes
// the ends of a cushion (next to a pocket) work correctly.
void CWall::hitBy(CSphere& ball)
{
	if (!hasIntersected(ball)) return;

	D3DXVECTOR3 c = ball.getCenter();
	float cx = clampf(c.x, m_x - m_width / 2.0f, m_x + m_width / 2.0f);
	float cz = clampf(c.z, m_z - m_depth / 2.0f, m_z + m_depth / 2.0f);
	float dx = c.x - cx;
	float dz = c.z - cz;
	float dist = sqrtf(dx * dx + dz * dz);
	if (dist < 1e-5f) return;   // should not happen (balls move in small steps)

	// normal of the wall surface at the contact point
	float nx = dx / dist;
	float nz = dz / dist;

	// push the ball out
	float push = ball.getRadius() - dist;
	ball.setCenter(c.x + nx * push, c.y, c.z + nz * push);

	// reflect only if the ball is moving into the wall
	float vx = (float)ball.getVelocity_X();
	float vz = (float)ball.getVelocity_Z();
	float vn = vx * nx + vz * nz;
	if (vn < 0) {
		float k = (1.0f + WALL_BOUNCE) * vn;
		ball.setPower(vx - k * nx, vz - k * nz);
	}
}

void CWall::setPosition(float x, float y, float z)
{
	D3DXMATRIX m;
	this->m_x = x;
	this->m_z = z;

	D3DXMatrixTranslation(&m, x, y, z);
	setLocalTransform(m);
}
