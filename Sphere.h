////////////////////////////////////////////////////////////////////////////////
//
// File: Sphere.h
//
// CSphere  : a ball (moving, collision with other balls, falling into a pocket)
// CCueBall : the white ball. It behaves differently when it is pocketed.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __SphereH__
#define __SphereH__

#include "GameObject.h"
#include "Constants.h"

class CSphere : public CGameObject {
protected:
	float					center_x, center_y, center_z;
	float                   m_radius;
	float					m_velocity_x;
	float					m_velocity_z;
	int                     m_number;   // 0 = cue ball, 1~15 = object balls
	int                     m_state;    // BALL_ON_TABLE, BALL_IN_TRAY, BALL_HIDDEN

	static CPocketListener* s_listener;

public:
	CSphere(void);
	virtual ~CSphere(void) {}

	static void setPocketListener(CPocketListener* listener) { s_listener = listener; }

	// number: ball number (it decides solid / stripe), color: ball color
	bool create(IDirect3DDevice9* pDevice, int number, D3DXCOLOR color = d3d::WHITE);
	virtual void destroy(void);
	virtual void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld);

	bool hasIntersected(CSphere& ball);   // do the two balls touch?
	void hitBy(CSphere& ball);            // collision response
	void ballUpdate(float timeDiff);      // move the ball + friction

	// called by CHole::hitBy when the ball falls into a pocket
	virtual void fallIntoPocket(void);
	virtual bool isCueBall(void) const { return false; }

	double getVelocity_X() { return this->m_velocity_x; }
	double getVelocity_Z() { return this->m_velocity_z; }
	float  getSpeed(void) const;
	void   setPower(double vx, double vz);
	void   setCenter(float x, float y, float z);

	int  getNumber(void) const { return m_number; }
	int  getState(void) const { return m_state; }
	void setState(int state) { m_state = state; }
	bool isOnTable(void) const { return m_state == BALL_ON_TABLE; }
	bool isMoving(void) const { return isOnTable() && getSpeed() > 0; }

	float getRadius(void)  const { return (float)(M_RADIUS); }
	const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	D3DXVECTOR3 getCenter(void) const { return D3DXVECTOR3(center_x, center_y, center_z); }

private:
	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	D3DMATERIAL9            m_bandMtrl;     // color of the stripe band
	ID3DXMesh*              m_pSphereMesh;
	ID3DXMesh*              m_pBandMesh;    // only for stripe balls (9~15)
};

class CCueBall : public CSphere {
public:
	// the cue ball is not moved to the tray. It disappears and the game
	// puts it back on the table later (this is a foul : "scratch").
	virtual void fallIntoPocket(void);
	virtual bool isCueBall(void) const { return true; }
};

#endif // __SphereH__
