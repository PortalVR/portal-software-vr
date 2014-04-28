#pragma once

#include <d3dx9.h>

#define _SAFE_RELEASE(p) if(p) { p->Release(); p = NULL; }

// D3D Colors
const D3DXCOLOR      WHITE( D3DCOLOR_XRGB(255, 255, 255) );
const D3DXCOLOR       GREY( D3DCOLOR_XRGB(128, 128, 128) );
const D3DXCOLOR      BLACK( D3DCOLOR_XRGB(  0,   0,   0) );
const D3DXCOLOR        RED( D3DCOLOR_XRGB(255,   0,   0) );
const D3DXCOLOR      GREEN( D3DCOLOR_XRGB(  0, 255,   0) );
const D3DXCOLOR       BLUE( D3DCOLOR_XRGB(  0,   0, 255) );
const D3DXCOLOR     YELLOW( D3DCOLOR_XRGB(255, 255,   0) );
const D3DXCOLOR       CYAN( D3DCOLOR_XRGB(  0, 255, 255) );
const D3DXCOLOR    MAGENTA( D3DCOLOR_XRGB(  0, 255, 255) );

const D3DXCOLOR    SKY_BLUE( D3DCOLOR_XRGB(135, 206, 235) );
const D3DXCOLOR  BEACH_SAND( D3DCOLOR_XRGB(255, 249, 157) );
const D3DXCOLOR DESERT_SAND( D3DCOLOR_XRGB(250, 205, 135) );

const D3DXCOLOR LIGHTGREEN( D3DCOLOR_XRGB( 60, 184, 120) );
const D3DXCOLOR  PUREGREEN( D3DCOLOR_XRGB(  0, 166,  81) );
const D3DXCOLOR  DARKGREEN( D3DCOLOR_XRGB(  0, 114,  54) );

const D3DXCOLOR LIGHT_YELLOW_GREEN( D3DCOLOR_XRGB(124, 197, 118) );
const D3DXCOLOR  PURE_YELLOW_GREEN( D3DCOLOR_XRGB( 57, 181,  74) );
const D3DXCOLOR  DARK_YELLOW_GREEN( D3DCOLOR_XRGB( 25, 123,  48) );

const D3DXCOLOR LIGHTBROWN( D3DCOLOR_XRGB(198, 156, 109) );
const D3DXCOLOR DARKBROWN( D3DCOLOR_XRGB(115, 100,  87) );

// color conversion
D3DXCOLOR vectorToColor (const D3DXVECTOR3 &vec, float rAlpha = 1.0f);

// D3D Font
ID3DXFont* initFont (IDirect3DDevice9* pD3DDevice);

// D3D Light Sources
D3DLIGHT9 initDirectionalLight(const D3DXVECTOR3& direction, const D3DXCOLOR& color);
D3DLIGHT9 initPointLight(const D3DXVECTOR3& position, const D3DXCOLOR& color);
D3DLIGHT9 initSpotLight(const D3DXVECTOR3& position, const D3DXVECTOR3& direction, const D3DXCOLOR& color);

// D3D Materials
D3DMATERIAL9 initMtrl(const D3DXCOLOR& a, const D3DXCOLOR& d, const D3DXCOLOR& s, const D3DXCOLOR& e, float p);
const D3DMATERIAL9 WHITE_MTRL  = initMtrl(WHITE, WHITE, WHITE, BLACK, 2.0f);
const D3DMATERIAL9 GREY_MTRL   = initMtrl(GREY, GREY, GREY, BLACK, 2.0f);
const D3DMATERIAL9 RED_MTRL    = initMtrl(RED, RED, RED, BLACK, 2.0f);
const D3DMATERIAL9 GREEN_MTRL  = initMtrl(GREEN, GREEN, GREEN, BLACK, 2.0f);
const D3DMATERIAL9 BLUE_MTRL   = initMtrl(BLUE, BLUE, BLUE, BLACK, 2.0f);
const D3DMATERIAL9 YELLOW_MTRL = initMtrl(YELLOW, YELLOW, YELLOW, BLACK, 2.0f);

// Compute face normal
D3DXVECTOR3 computeNormal(const D3DXVECTOR3& p0, const D3DXVECTOR3& p1, const D3DXVECTOR3& p2);
