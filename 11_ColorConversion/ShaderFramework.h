//**********************************************************************
//
// ShaderFramework.h
// 
// Super simple C-style framework for Shader Demo
// (NEVER ever write framework like this when you are making real games.)

//
// Author: Pope Kim
//
//**********************************************************************


#pragma once

#include <d3d9.h>
#include <d3dx9.h>

// ---------- constants ------------------------------------
#define WIN_WIDTH		800
#define WIN_HEIGHT		600

// ---------------- function prototype  ------------------------

// Message procedure related
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ProcessInput(HWND hWnd, WPARAM keyPress);

// Initialization-related
bool InitEverything(HWND hWnd);
bool InitD3D(HWND hWnd);
bool LoadAssets();
LPD3DXEFFECT LoadShader(const char * filename);
LPDIRECT3DTEXTURE9 LoadTexture(const char * filename);
LPD3DXMESH LoadModel(const char * filename);

// game loop related
void PlayDemo();
void Update();

// Rendering related
void RenderFrame();
void RenderScene();
void RenderInfo();

// cleanup related
void Cleanup();


void InitFullScreenQuad();