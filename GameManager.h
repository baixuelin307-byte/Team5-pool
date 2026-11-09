
////////////////////////////////////////////////////////////////////////////////
//
// File: GameManager.h
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __GameManagerH__
#define __GameManagerH__

#include "GameObject.h"
#include "Constants.h"
#include <vector>

class CSphere;

class CGameManager : public CPocketListener {
public:
    enum { STATE_READY, STATE_ROLLING, STATE_OVER };

    CGameManager(void);
    virtual ~CGameManager(void) {}

    bool create(IDirect3DDevice9* pDevice, CSphere** balls, int numBalls);
    void destroy(void);
    void reset(void);

    bool canShoot(void) const { return m_state == STATE_READY; }
    bool isRolling(void) const { return m_state == STATE_ROLLING; }

    void startShot(void);
    void recordContact(int a, int b);
    void checkShotEnd(void);
    int remaining(int group) const;

    virtual void onBallPocketed(CSphere* ball);

    void drawHUD(IDirect3DDevice9* pDevice);

    // NEW: shot power
    // Range: 0.0f - 1.0f
    void setShotPower(float power);
    float getShotPower(void) const {
        return m_shotPower;
    }

private:
    void drawText(
        ID3DXFont* font,
        const char* text,
        int x,
        int y,
        D3DCOLOR color
    );

    void respawnCueBall(void);
    void gameOver(int winner, const char* reason);
    void judgeShot(void);

    CSphere** m_balls;
    int m_numBalls;
    int m_state;
    int m_turn;
    int m_group[2];

    int m_firstHit;
    std::vector<int> m_pocketed;
    bool m_scratch;
    bool m_groupClearedBeforeShot;

    // NEW: current shot power
    float m_shotPower;

    char m_message[128];

    ID3DXFont* m_pFont;
    ID3DXFont* m_pBigFont;
};

#endif // __GameManagerH__
