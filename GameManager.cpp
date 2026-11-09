
////////////////////////////////////////////////////////////////////////////////
//
// File: GameManager.cpp
//
////////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

#include "GameManager.h"
#include "Sphere.h"
#include <cstdio>

////////////////////////////////////////////////////////////////////////////////
// NEW: Draw a solid 2D rectangle using DirectX 9
////////////////////////////////////////////////////////////////////////////////

static void drawHUDRect(
    IDirect3DDevice9* device,
    int x,
    int y,
    int width,
    int height,
    D3DCOLOR color)
{
    if (device == NULL || width <= 0 || height <= 0)
        return;

    D3DRECT rect;
    rect.x1 = x;
    rect.y1 = y;
    rect.x2 = x + width;
    rect.y2 = y + height;

    device->Clear(
        1,
        &rect,
        D3DCLEAR_TARGET,
        color,
        1.0f,
        0
    );
}

////////////////////////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////////////////////////

CGameManager::CGameManager(void)
{
    m_balls = NULL;
    m_numBalls = 0;
    m_state = STATE_READY;
    m_turn = 0;

    m_group[0] = m_group[1] = GROUP_NONE;

    m_firstHit = -1;
    m_scratch = false;
    m_groupClearedBeforeShot = false;

    // NEW: initial shot power
    m_shotPower = 0.0f;

    m_message[0] = '\0';

    m_pFont = NULL;
    m_pBigFont = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// NEW: Set shot power
////////////////////////////////////////////////////////////////////////////////

void CGameManager::setShotPower(float power)
{
    if (power < 0.0f)
        power = 0.0f;

    if (power > 1.0f)
        power = 1.0f;

    m_shotPower = power;
}

////////////////////////////////////////////////////////////////////////////////
// Create
////////////////////////////////////////////////////////////////////////////////

bool CGameManager::create(
    IDirect3DDevice9* pDevice,
    CSphere** balls,
    int numBalls)
{
    m_balls = balls;
    m_numBalls = numBalls;

    CSphere::setPocketListener(this);

    if (FAILED(D3DXCreateFont(
        pDevice, 22, 0, FW_BOLD, 1, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        "Arial",
        &m_pFont)))
    {
        return false;
    }

    if (FAILED(D3DXCreateFont(
        pDevice, 40, 0, FW_BOLD, 1, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        "Arial",
        &m_pBigFont)))
    {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Destroy
////////////////////////////////////////////////////////////////////////////////

void CGameManager::destroy(void)
{
    if (m_pFont != NULL) {
        m_pFont->Release();
        m_pFont = NULL;
    }

    if (m_pBigFont != NULL) {
        m_pBigFont->Release();
        m_pBigFont = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Reset
////////////////////////////////////////////////////////////////////////////////

void CGameManager::reset(void)
{
    static const int rack[15] = {
        1, 9, 2, 10, 8,
        3, 11, 7, 14, 4,
        5, 13, 15, 6, 12
    };

    const float rowGap = M_RADIUS * 1.75f;
    const float colGap = M_RADIUS * 2.02f;

    float footX = TABLE_HALF_X / 2;

    int k = 0;

    for (int row = 0; row < 5; row++) {
        for (int i = 0; i <= row; i++) {

            CSphere* b = m_balls[rack[k++]];

            float x = footX + row * rowGap;
            float z = (i - row / 2.0f) * colGap;

            b->setCenter(x, M_RADIUS, z);
            b->setPower(0, 0);
            b->setState(BALL_ON_TABLE);
        }
    }

    m_balls[0]->setCenter(
        -TABLE_HALF_X / 2,
        M_RADIUS,
        0
    );

    m_balls[0]->setPower(0, 0);
    m_balls[0]->setState(BALL_ON_TABLE);

    m_state = STATE_READY;
    m_turn = 0;

    m_group[0] = m_group[1] = GROUP_NONE;

    m_firstHit = -1;
    m_pocketed.clear();
    m_scratch = false;
    m_groupClearedBeforeShot = false;

    // NEW: reset power
    m_shotPower = 0.0f;

    sprintf(
        m_message,
        "Player 1 breaks. Aim and hold SPACE to charge."
    );
}

////////////////////////////////////////////////////////////////////////////////
// Start shot
////////////////////////////////////////////////////////////////////////////////

void CGameManager::startShot(void)
{
    m_firstHit = -1;
    m_pocketed.clear();

    m_scratch = false;

    m_groupClearedBeforeShot =
        (m_group[m_turn] != GROUP_NONE &&
            remaining(m_group[m_turn]) == 0);

    m_state = STATE_ROLLING;

    // NEW: clear power after shooting
    m_shotPower = 0.0f;

    m_message[0] = '\0';
}

////////////////////////////////////////////////////////////////////////////////
// Record contact
////////////////////////////////////////////////////////////////////////////////

void CGameManager::recordContact(int a, int b)
{
    if (m_firstHit >= 0)
        return;

    if (a == 0)
        m_firstHit = b;
    else if (b == 0)
        m_firstHit = a;
}

////////////////////////////////////////////////////////////////////////////////
// Ball pocketed
////////////////////////////////////////////////////////////////////////////////

void CGameManager::onBallPocketed(CSphere* ball)
{
    if (ball->isCueBall()) {
        m_scratch = true;
        return;
    }

    m_pocketed.push_back(ball->getNumber());

    int n = ball->getNumber();
    float x;

    if (n < 8)
        x = -3.6f + (n - 1) * 0.48f;
    else if (n == 8)
        x = 0.0f;
    else
        x = 0.72f + (n - 9) * 0.48f;

    ball->setCenter(
        x,
        M_RADIUS,
        -(TABLE_HALF_Z + 1.0f)
    );
}

////////////////////////////////////////////////////////////////////////////////
// Remaining balls
////////////////////////////////////////////////////////////////////////////////

int CGameManager::remaining(int group) const
{
    int count = 0;

    for (int i = 1; i < m_numBalls; i++) {
        if (groupOf(i) == group &&
            m_balls[i]->isOnTable())
        {
            count++;
        }
    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////
// Check shot end
////////////////////////////////////////////////////////////////////////////////

void CGameManager::checkShotEnd(void)
{
    if (m_state != STATE_ROLLING)
        return;

    for (int i = 0; i < m_numBalls; i++) {
        if (m_balls[i]->isMoving())
            return;
    }

    judgeShot();
}

////////////////////////////////////////////////////////////////////////////////
// Draw HUD
////////////////////////////////////////////////////////////////////////////////

void CGameManager::drawHUD(IDirect3DDevice9* pDevice)
{
    if (m_pFont == NULL || pDevice == NULL)
        return;

    D3DVIEWPORT9 vp;
    pDevice->GetViewport(&vp);

    int w = (int)vp.Width;
    int h = (int)vp.Height;

    const D3DCOLOR ACTIVE =
        D3DCOLOR_XRGB(255, 60, 60);

    const D3DCOLOR NORMAL =
        D3DCOLOR_XRGB(30, 30, 30);

    char buf[128];

    ////////////////////////////////////////////////////////////////
    // Player information
    ////////////////////////////////////////////////////////////////

    for (int p = 0; p < 2; p++) {

        int x = (p == 0) ? 20 : w - 220;

        D3DCOLOR c =
            (p == m_turn && m_state != STATE_OVER)
            ? ACTIVE : NORMAL;

        sprintf(buf, "Player %d", p + 1);

        drawText(
            m_pFont,
            buf,
            x,
            15,
            c
        );

        int count = 0;

        if (m_group[p] != GROUP_NONE)
            count = 7 - remaining(m_group[p]);

        sprintf(buf, "Ball Count : %d", count);

        drawText(
            m_pFont,
            buf,
            x,
            40,
            NORMAL
        );

        const char* g = "-";

        if (m_group[p] == GROUP_SOLID)
            g = "SOLID";

        if (m_group[p] == GROUP_STRIPE)
            g = "STRIPE";

        drawText(
            m_pFont,
            g,
            x,
            65,
            NORMAL
        );
    }

    ////////////////////////////////////////////////////////////////
    // Game state
    ////////////////////////////////////////////////////////////////

    if (m_state == STATE_ROLLING) {

        drawText(
            m_pFont,
            "Processing...",
            w / 2 - 70,
            15,
            NORMAL
        );
    }
    else if (m_state == STATE_OVER) {

        drawText(
            m_pBigFont,
            "GAME OVER",
            w / 2 - 110,
            h / 2 - 60,
            ACTIVE
        );
    }

    ////////////////////////////////////////////////////////////////
    // NEW: Shot power bar
    ////////////////////////////////////////////////////////////////

    if (m_state == STATE_READY && h > 220 && w > 80) {

        const int barX = 20;
        const int barY = h - 145;

        int barWidth = 250;
        const int barHeight = 20;

        if (barWidth > w - 40)
            barWidth = w - 40;

        int percentage =
            (int)(m_shotPower * 100.0f + 0.5f);

        sprintf(
            buf,
            "SHOT POWER : %d%%",
            percentage
        );

        drawText(
            m_pFont,
            buf,
            barX,
            barY - 34,
            NORMAL
        );

        // Border
        drawHUDRect(
            pDevice,
            barX - 2,
            barY - 2,
            barWidth + 4,
            barHeight + 4,
            D3DCOLOR_XRGB(40, 40, 40)
        );

        // Background
        drawHUDRect(
            pDevice,
            barX,
            barY,
            barWidth,
            barHeight,
            D3DCOLOR_XRGB(100, 100, 100)
        );

        // Current filled width
        int filledWidth =
            (int)(barWidth * m_shotPower);

        // Color zones:
        // 0-40% green
        // 40-75% yellow
        // 75-100% red

        int greenEnd = barWidth * 40 / 100;
        int yellowEnd = barWidth * 75 / 100;

        // Green section
        int greenFill = filledWidth;

        if (greenFill > greenEnd)
            greenFill = greenEnd;

        if (greenFill > 0) {
            drawHUDRect(
                pDevice,
                barX,
                barY,
                greenFill,
                barHeight,
                D3DCOLOR_XRGB(40, 200, 70)
            );
        }

        // Yellow section
        if (filledWidth > greenEnd) {

            int end = filledWidth;

            if (end > yellowEnd)
                end = yellowEnd;

            drawHUDRect(
                pDevice,
                barX + greenEnd,
                barY,
                end - greenEnd,
                barHeight,
                D3DCOLOR_XRGB(245, 205, 35)
            );
        }

        // Red section
        if (filledWidth > yellowEnd) {

            drawHUDRect(
                pDevice,
                barX + yellowEnd,
                barY,
                filledWidth - yellowEnd,
                barHeight,
                D3DCOLOR_XRGB(235, 55, 55)
            );
        }
    }

    ////////////////////////////////////////////////////////////////
    // Bottom messages
    ////////////////////////////////////////////////////////////////

    drawText(
        m_pFont,
        m_message,
        20,
        h - 70,
        NORMAL
    );

    drawText(
        m_pFont,
        "R-drag: aim  Hold SPACE: charge  Release: shoot  L-drag: rotate  V: view  R: restart",
        20,
        h - 40,
        D3DCOLOR_XRGB(70, 70, 70)
    );
}

////////////////////////////////////////////////////////////////////////////////
// Draw text
////////////////////////////////////////////////////////////////////////////////

void CGameManager::drawText(
    ID3DXFont* font,
    const char* text,
    int x,
    int y,
    D3DCOLOR color)
{
    RECT rc;

    SetRect(
        &rc,
        x,
        y,
        x + 1200,
        y + 50
    );

    font->DrawText(
        NULL,
        text,
        -1,
        &rc,
        DT_LEFT | DT_NOCLIP,
        color
    );
}

////////////////////////////////////////////////////////////////////////////////
// Respawn cue ball
////////////////////////////////////////////////////////////////////////////////

void CGameManager::respawnCueBall(void)
{
    CSphere* cue = m_balls[0];

    float x = -TABLE_HALF_X / 2;

    for (int tries = 0; tries < 40; tries++) {

        bool free = true;

        for (int i = 1; i < m_numBalls; i++) {

            if (!m_balls[i]->isOnTable())
                continue;

            D3DXVECTOR3 c =
                m_balls[i]->getCenter();

            float dx = c.x - x;
            float dz = c.z;

            if (dx * dx + dz * dz <
                (4.2f * M_RADIUS * M_RADIUS))
            {
                free = false;
                break;
            }
        }

        if (free)
            break;

        x -= M_RADIUS;
    }

    cue->setCenter(x, M_RADIUS, 0);
    cue->setPower(0, 0);
    cue->setState(BALL_ON_TABLE);
}

////////////////////////////////////////////////////////////////////////////////
// Game over
////////////////////////////////////////////////////////////////////////////////

void CGameManager::gameOver(
    int winner,
    const char* reason)
{
    m_state = STATE_OVER;

    m_shotPower = 0.0f;

    sprintf(
        m_message,
        "%s  Player %d wins!  (press R to restart)",
        reason,
        winner + 1
    );
}

////////////////////////////////////////////////////////////////////////////////
// Judge shot
////////////////////////////////////////////////////////////////////////////////

void CGameManager::judgeShot(void)
{
    int me = m_turn;
    int other = 1 - m_turn;

    bool eightIn = false;

    for (size_t i = 0; i < m_pocketed.size(); i++) {
        if (m_pocketed[i] == 8)
            eightIn = true;
    }

    ////////////////////////////////////////////////////////////////
    // 1. Foul check
    ////////////////////////////////////////////////////////////////

    const char* foul = NULL;

    if (m_scratch) {

        foul = "Foul: the cue ball was pocketed.";

    }
    else if (m_firstHit < 0) {

        foul = "Foul: the cue ball did not hit any ball.";

    }
    else if (m_group[me] == GROUP_NONE) {

        if (m_firstHit == 8)
            foul = "Foul: the 8-ball was hit first.";

    }
    else if (m_groupClearedBeforeShot) {

        if (m_firstHit != 8)
            foul = "Foul: you must hit the 8-ball.";

    }
    else if (groupOf(m_firstHit) != m_group[me]) {

        foul = "Foul: an opponent's ball was hit first.";
    }

    ////////////////////////////////////////////////////////////////
    // 2. Eight ball
    ////////////////////////////////////////////////////////////////

    if (eightIn) {

        if (m_groupClearedBeforeShot && foul == NULL) {

            gameOver(me, "The 8-ball is in!");

        }
        else {

            gameOver(
                other,
                "The 8-ball was pocketed too early."
            );
        }

        return;
    }

    ////////////////////////////////////////////////////////////////
    // 3. Assign groups
    ////////////////////////////////////////////////////////////////

    if (m_group[me] == GROUP_NONE &&
        foul == NULL &&
        !m_pocketed.empty())
    {
        m_group[me] = groupOf(m_pocketed[0]);

        m_group[other] =
            (m_group[me] == GROUP_SOLID)
            ? GROUP_STRIPE
            : GROUP_SOLID;
    }

    ////////////////////////////////////////////////////////////////
    // 4. Check own ball
    ////////////////////////////////////////////////////////////////

    bool ownBallIn = false;

    for (size_t i = 0; i < m_pocketed.size(); i++) {

        if (groupOf(m_pocketed[i]) == m_group[me])
            ownBallIn = true;
    }

    if (m_scratch)
        respawnCueBall();

    if (foul != NULL) {

        m_turn = other;

        sprintf(
            m_message,
            "%s  Player %d's turn.",
            foul,
            m_turn + 1
        );
    }
    else if (ownBallIn) {

        sprintf(
            m_message,
            "Good shot! Player %d plays again.",
            m_turn + 1
        );
    }
    else {

        m_turn = other;

        sprintf(
            m_message,
            "Player %d's turn.",
            m_turn + 1
        );
    }

    m_state = STATE_READY;
}
