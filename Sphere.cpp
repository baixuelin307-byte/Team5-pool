////////////////////////////////////////////////////////////////////////////////
//
// File: Sphere.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include "Sphere.h"
#include <cmath>

CPocketListener* CSphere::s_listener = NULL;

CSphere::CSphere(void)
{
	D3DXMatrixIdentity(&m_mLocal);
	ZeroMemory(&m_mtrl, sizeof(m_mtrl));
	ZeroMemory(&m_bandMtrl, sizeof(m_bandMtrl));
	center_x = center_y = center_z = 0;
	m_radius = 0;
	m_velocity_x = 0;
	m_velocity_z = 0;
	m_number = 0;
	m_state = BALL_ON_TABLE;
	m_pSphereMesh = NULL;
	m_pBandMesh = NULL;
}

bool CSphere::create(IDirect3DDevice9* pDevice, int number, D3DXCOLOR color)
{
	if (NULL == pDevice)
		return false;

	m_number = number;
	bool isStripe = (groupOf(number) == GROUP_STRIPE);

	// a stripe ball is a white ball with a colored band around it
	setMaterial(m_mtrl, isStripe ? d3d::WHITE : color);
	setMaterial(m_bandMtrl, color);

	if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
		return false;

	if (isStripe) {
		// a short cylinder a little bigger than the ball = the colored band
		if (FAILED(D3DXCreateCylinder(pDevice, getRadius() * 1.02f, getRadius() * 1.02f,
			getRadius() * 0.9f, 50, 1, &m_pBandMesh, NULL)))
			return false;
	}
	return true;
}

void CSphere::destroy(void)
{
	if (m_pSphereMesh != NULL) {
		m_pSphereMesh->Release();
		m_pSphereMesh = NULL;
	}
	if (m_pBandMesh != NULL) {
		m_pBandMesh->Release();
		m_pBandMesh = NULL;
	}
}

void CSphere::draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
{
	if (NULL == pDevice || m_state == BALL_HIDDEN)
		return;
	pDevice->SetTransform(D3DTS_WORLD, &mWorld);
	pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
	pDevice->SetMaterial(&m_mtrl);
	m_pSphereMesh->DrawSubset(0);

	if (m_pBandMesh != NULL) {
		// the cylinder is made along the z axis, so stand it up first
		D3DXMATRIX mRot;
		D3DXMatrixRotationX(&mRot, PI / 2);
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->MultiplyTransform(D3DTS_WORLD, &mRot);
		pDevice->SetMaterial(&m_bandMtrl);
		m_pBandMesh->DrawSubset(0);
	}
}

// Two balls touch when the distance between their centers (on the x-z plane)
// is not larger than the sum of their radii.
bool CSphere::hasIntersected(CSphere& ball)
{
	if (&ball == this || !isOnTable() || !ball.isOnTable()) return false;
	float dx = ball.center_x - center_x;
	float dz = ball.center_z - center_z;
	float rsum = getRadius() + ball.getRadius();
	return (dx * dx + dz * dz) <= (rsum * rsum);
}

// Collision of two balls with the same mass:
// the velocity components along the line joining the centers are exchanged
// (with a little energy loss), the other components are kept.
void CSphere::hitBy(CSphere& ball)
{
	if (!hasIntersected(ball)) return;

	float dx = ball.center_x - center_x;
	float dz = ball.center_z - center_z;
	float dist = sqrtf(dx * dx + dz * dz);

	// unit normal from this ball to the other ball
	float nx, nz;
	if (dist < 1e-6f) { nx = 1.0f; nz = 0.0f; dist = 0.0f; }
	else              { nx = dx / dist; nz = dz / dist; }

	// 1) push the balls apart so that they no longer overlap
	float overlap = (getRadius() + ball.getRadius()) - dist;
	if (overlap > 0) {
		float half = overlap / 2.0f;
		setCenter(center_x - nx * half, center_y, center_z - nz * half);
		ball.setCenter(ball.center_x + nx * half, ball.center_y, ball.center_z + nz * half);
	}

	// 2) velocity components along the normal
	float v1n = m_velocity_x * nx + m_velocity_z * nz;
	float v2n = ball.m_velocity_x * nx + ball.m_velocity_z * nz;

	// already moving apart: nothing more to do
	if (v1n - v2n <= 0) return;

	// 3) exchange the normal components (BALL_BOUNCE < 1 loses a little energy)
	float avg  = (v1n + v2n) / 2.0f;
	float diff = (v1n - v2n) / 2.0f * BALL_BOUNCE;
	float new1 = avg - diff;
	float new2 = avg + diff;
	setPower(m_velocity_x + (new1 - v1n) * nx, m_velocity_z + (new1 - v1n) * nz);
	ball.setPower(ball.m_velocity_x + (new2 - v2n) * nx, ball.m_velocity_z + (new2 - v2n) * nz);
}

// move the ball. the distance is proportional to timeDiff,
// so the speed does not depend on the speed of the computer.
void CSphere::ballUpdate(float timeDiff)
{
	if (!isOnTable()) return;

	if (getSpeed() > STOP_SPEED)
	{
		float tX = center_x + TIME_SCALE * timeDiff * m_velocity_x;
		float tZ = center_z + TIME_SCALE * timeDiff * m_velocity_z;
		// (the position correction of the sample code is not used any more,
		//  because balls must be able to leave the table through the pockets.
		//  the cushions push the balls back instead, see CWall::hitBy)
		setCenter(tX, center_y, tZ);
	}
	else { setPower(0, 0); }

	// friction (same formula as the sample code)
	double rate = 1 - (1 - DECREASE_RATE) * timeDiff * 400;
	if (rate < 0)
		rate = 0;
	setPower((float)(m_velocity_x * rate), (float)(m_velocity_z * rate));
}

// an object ball goes to the tray next to the table
void CSphere::fallIntoPocket(void)
{
	setPower(0, 0);
	m_state = BALL_IN_TRAY;
	if (s_listener != NULL) s_listener->onBallPocketed(this);
}

float CSphere::getSpeed(void) const
{
	return sqrtf(m_velocity_x * m_velocity_x + m_velocity_z * m_velocity_z);
}

void CSphere::setPower(double vx, double vz)
{
	this->m_velocity_x = (float)vx;
	this->m_velocity_z = (float)vz;
}

void CSphere::setCenter(float x, float y, float z)
{
	D3DXMATRIX m;
	center_x = x;	center_y = y;	center_z = z;
	D3DXMatrixTranslation(&m, x, y, z);
	setLocalTransform(m);
}

// -----------------------------------------------------------------------------
// CCueBall
// -----------------------------------------------------------------------------
void CCueBall::fallIntoPocket(void)
{
	setPower(0, 0);
	m_state = BALL_HIDDEN;
	if (s_listener != NULL) s_listener->onBallPocketed(this);
}
