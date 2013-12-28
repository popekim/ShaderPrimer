//**********************************************************************
//
// ShaderFramework.cpp
// 
// Super simple C-style framework for Shader Demo
// (NEVER ever write framework like this when you are making real games.)

//
// Author: Pope Kim
//
//**********************************************************************

#include "ShaderFramework.h"
#include <stdio.h>

#define PI           3.14159265f
#define FOV          (PI/4.0f)							// Field of View
#define ASPECT_RATIO (WIN_WIDTH/(float)WIN_HEIGHT)		// aspect ratio of screen
#define NEAR_PLANE   1									
#define FAR_PLANE    10000								


//----------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------

// D3D-related
LPDIRECT3D9             gpD3D = NULL;					// D3D
LPDIRECT3DDEVICE9       gpD3DDevice = NULL;				// D3D device

// Fonts
ID3DXFont*              gpFont = NULL;

// Models
LPD3DXMESH				gpTeapot = NULL;

// Shaders
LPD3DXEFFECT			gpEnvironmentMappingShader = NULL;
LPD3DXEFFECT			gpNoEffect = NULL;
LPD3DXEFFECT			gpGrayScale = NULL;
LPD3DXEFFECT			gpSepia = NULL;
LPD3DXEFFECT			gpEdgeDetection = NULL;
LPD3DXEFFECT			gpEmboss = NULL;

// Textures
LPDIRECT3DTEXTURE9		gpStoneDM = NULL;
LPDIRECT3DTEXTURE9		gpStoneSM = NULL;
LPDIRECT3DTEXTURE9		gpStoneNM = NULL;
LPDIRECT3DCUBETEXTURE9	gpSnowENV = NULL;

// Application Name
const char*				gAppName = "Super Simple Shader Demo Framework";

// Rotation around UP vector
float					gRotationY = 0.0f;

// world position of the light
D3DXVECTOR4				gWorldLightPosition(500.0f, 500.0f, -500.0f, 1.0f);

// the color of light
D3DXVECTOR4				gLightColor(0.7f, 0.7f, 1.0f, 1.0f);

// world position of the camera
D3DXVECTOR4				gWorldCameraPosition(0.0f, 0.0f, -200.0f, 1.0f);

// fullscreen quad
LPDIRECT3DVERTEXDECLARATION9	gpFullscreenQuadDecl = NULL;
LPDIRECT3DVERTEXBUFFER9			gpFullscreenQuadVB = NULL;
LPDIRECT3DINDEXBUFFER9			gpFullscreenQuadIB = NULL;

// scene render target
LPDIRECT3DTEXTURE9		gpSceneRenderTarget = NULL;

// index of postprocess shader to use
int gPostProcessIndex = 0;

//-----------------------------------------------------------------------
// Program entry point/message loop
//-----------------------------------------------------------------------

// entry point
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	// register windows class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		gAppName, NULL };
	RegisterClassEx(&wc);

	// Create the program window
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	HWND hWnd = CreateWindow(gAppName, gAppName,
		style, CW_USEDEFAULT, 0, WIN_WIDTH, WIN_HEIGHT,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);

	// Client Rect size will be same as WIN_WIDTH and WIN_HEIGHT
	POINT ptDiff;
	RECT rcClient, rcWindow;

	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(hWnd, rcWindow.left, rcWindow.top, WIN_WIDTH + ptDiff.x, WIN_HEIGHT + ptDiff.y, TRUE);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	// Initialize everything including D3D
	if (!InitEverything(hWnd))
		PostQuitMessage(1);

	// Message Loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else // If there is no message to process, update and render the scene
		{
			PlayDemo();
		}
	}

	UnregisterClass(gAppName, wc.hInstance);
	return 0;
}

// Message procedure
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		ProcessInput(hWnd, wParam);
		break;

	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Keyboard input handling
void ProcessInput(HWND hWnd, WPARAM keyPress)
{
	switch (keyPress)
	{
		// When ESC key is pressed, exit the program
	case VK_ESCAPE:
		PostMessage(hWnd, WM_DESTROY, 0L, 0L);
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		gPostProcessIndex = keyPress - '0' - 1;
		break;
	}
}

//------------------------------------------------------------
// Game loop
//------------------------------------------------------------
void PlayDemo()
{
	Update();
	RenderFrame();
}

// Game logic update
void Update()
{
}

//------------------------------------------------------------
// rendering
//------------------------------------------------------------

void RenderFrame()
{
	D3DCOLOR bgColor = 0xFF0000FF;	// background color - blue

	gpD3DDevice->Clear(0, NULL, (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), bgColor, 1.0f, 0);

	gpD3DDevice->BeginScene();
	{
		RenderScene();				// draw 3D objects and so on
		RenderInfo();				// display debug info
	}
	gpD3DDevice->EndScene();

	gpD3DDevice->Present(NULL, NULL, NULL, NULL);
}


// draw 3D objects and so on
void RenderScene()
{
	/////////////////////////
	// 1. draw the scene into the render target
	/////////////////////////
	// current hardware backbuffer
	LPDIRECT3DSURFACE9 pHWBackBuffer = NULL;
	gpD3DDevice->GetRenderTarget(0, &pHWBackBuffer);

	// draw onto the render target
	LPDIRECT3DSURFACE9 pSceneSurface = NULL;
	if (SUCCEEDED(gpSceneRenderTarget->GetSurfaceLevel(0, &pSceneSurface)))
	{
		gpD3DDevice->SetRenderTarget(0, pSceneSurface);
		pSceneSurface->Release();
		pSceneSurface = NULL;
	}

	// clear what's drawn in the last frame
	gpD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);

	// make the view matrix
	D3DXMATRIXA16 matView;
	D3DXVECTOR3 vEyePt(gWorldCameraPosition.x, gWorldCameraPosition.y, gWorldCameraPosition.z);
	D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);

	// projection matrix
	D3DXMATRIXA16			matProjection;
	D3DXMatrixPerspectiveFovLH(&matProjection, FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE);

	// for each frame, we roate 0.4 degree
	gRotationY += 0.4f * PI / 180.0f;
	if (gRotationY > 2 * PI)
	{
		gRotationY -= 2 * PI;
	}

	// world matrix
	D3DXMATRIXA16			matWorld;
	D3DXMatrixRotationY(&matWorld, gRotationY);

	// concatenate world/view/projection matrices
	D3DXMATRIXA16 matWorldView;
	D3DXMATRIXA16 matWorldViewProjection;
	D3DXMatrixMultiply(&matWorldView, &matWorld, &matView);
	D3DXMatrixMultiply(&matWorldViewProjection, &matWorldView, &matProjection);

	// set shader global variables
	gpEnvironmentMappingShader->SetMatrix("gWorldMatrix", &matWorld);
	gpEnvironmentMappingShader->SetMatrix("gWorldViewProjectionMatrix", &matWorldViewProjection);

	gpEnvironmentMappingShader->SetVector("gWorldLightPosition", &gWorldLightPosition);
	gpEnvironmentMappingShader->SetVector("gWorldCameraPosition", &gWorldCameraPosition);

	gpEnvironmentMappingShader->SetVector("gLightColor", &gLightColor);
	gpEnvironmentMappingShader->SetTexture("DiffuseMap_Tex", gpStoneDM);
	gpEnvironmentMappingShader->SetTexture("SpecularMap_Tex", gpStoneSM);
	gpEnvironmentMappingShader->SetTexture("NormalMap_Tex", gpStoneNM);
	gpEnvironmentMappingShader->SetTexture("EnvironmentMap_Tex", gpSnowENV);

	// start a shader
	UINT numPasses = 0;
	gpEnvironmentMappingShader->Begin(&numPasses, NULL);
	{
		for (UINT i = 0; i < numPasses; ++i)
		{
			gpEnvironmentMappingShader->BeginPass(i);
			{
				// draw a sphere
				gpTeapot->DrawSubset(0);
			}
			gpEnvironmentMappingShader->EndPass();
		}
	}
	gpEnvironmentMappingShader->End();


	/////////////////////////
	// 2. apply post-processing
	/////////////////////////
	// use hardware backbuffer
	gpD3DDevice->SetRenderTarget(0, pHWBackBuffer);
	pHWBackBuffer->Release();
	pHWBackBuffer = NULL;

	// post process effect to use
	LPD3DXEFFECT effectToUse = gpNoEffect;
	if (gPostProcessIndex == 1)
	{
		effectToUse = gpGrayScale;
	}
	else if (gPostProcessIndex == 2)
	{
		effectToUse = gpSepia;
	}
	else if (gPostProcessIndex == 3)
	{
		effectToUse = gpEdgeDetection;
	}
	else if (gPostProcessIndex == 4)
	{
		effectToUse = gpEmboss;
	}

	D3DXVECTOR4 pixelOffset(1 / (float)WIN_WIDTH, 1 / (float)WIN_HEIGHT, 0, 0);
	if (effectToUse == gpEdgeDetection || effectToUse == gpEmboss)
	{
		effectToUse->SetVector("gPixelOffset", &pixelOffset);
	}

	effectToUse->SetTexture("SceneTexture_Tex", gpSceneRenderTarget);
	effectToUse->Begin(&numPasses, NULL);
	{
		for (UINT i = 0; i < numPasses; ++i)
		{
			effectToUse->BeginPass(i);
			{
				// draw a fullscreen quad
				gpD3DDevice->SetStreamSource(0, gpFullscreenQuadVB, 0, sizeof(float)* 5);
				gpD3DDevice->SetIndices(gpFullscreenQuadIB);
				gpD3DDevice->SetVertexDeclaration(gpFullscreenQuadDecl);
				gpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 6, 0, 2);
			}
			effectToUse->EndPass();
		}
	}
	effectToUse->End();

}

// display debug info
void RenderInfo()
{
	// text color
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 255, 255, 255);

	// location to display the text
	RECT rct;
	rct.left = 5;
	rct.right = WIN_WIDTH / 3;
	rct.top = 5;
	rct.bottom = WIN_HEIGHT / 3;

	// display debug key info
	gpFont->DrawText(NULL, "Demo Framework\n\nESC: Exit\n1: Color\n2: Black and White\n3: Sepia\n4: Edge Detection\n5: Emboss", -1, &rct, 0, fontColor);
}

//------------------------------------------------------------
// Initialization code
//------------------------------------------------------------
bool InitEverything(HWND hWnd)
{
	// init D3D
	if (!InitD3D(hWnd))
	{
		return false;
	}

	// create a fullscreen quad
	InitFullScreenQuad();

	// create a render target
	if (FAILED(gpD3DDevice->CreateTexture(WIN_WIDTH, WIN_HEIGHT,
		1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8,
		D3DPOOL_DEFAULT, &gpSceneRenderTarget, NULL)))
	{
		return false;
	}

	// loading models, shaders and textures
	if (!LoadAssets())
	{
		return false;
	}

	// load fonts
	if (FAILED(D3DXCreateFont(gpD3DDevice, 20, 10, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE),
		"Arial", &gpFont)))
	{
		return false;
	}

	return true;
}

// init D3D object and device
bool InitD3D(HWND hWnd)
{
	// D3D object
	gpD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!gpD3D)
	{
		return false;
	}

	// fill in the structure needed to create a D3D device
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.BackBufferWidth = WIN_WIDTH;
	d3dpp.BackBufferHeight = WIN_HEIGHT;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality = 0;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.Windowed = TRUE;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
	d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
	d3dpp.FullScreen_RefreshRateInHz = 0;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	// create a D3D device
	if (FAILED(gpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &gpD3DDevice)))
	{
		return false;
	}

	return true;
}

bool LoadAssets()
{
	// loading textures
	gpStoneDM = LoadTexture("Fieldstone_DM.tga");
	if (!gpStoneDM)
	{
		return false;
	}

	gpStoneSM = LoadTexture("Fieldstone_SM.tga");
	if (!gpStoneSM)
	{
		return false;
	}

	gpStoneNM = LoadTexture("Fieldstone_NM.tga");
	if (!gpStoneNM)
	{
		return false;
	}

	D3DXCreateCubeTextureFromFile(gpD3DDevice, "Snow_ENV.dds", &gpSnowENV);
	if (!gpSnowENV)
	{
		return false;
	}

	// loading shaders
	gpEnvironmentMappingShader = LoadShader("EnvironmentMapping.fx");
	if (!gpEnvironmentMappingShader)
	{
		return false;
	}

	gpNoEffect = LoadShader("NoEffect.fx");
	if (!gpNoEffect)
	{
		return false;
	}

	gpGrayScale = LoadShader("Grayscale.fx");
	if (!gpGrayScale)
	{
		return false;
	}

	gpSepia = LoadShader("Sepia.fx");
	if (!gpSepia)
	{
		return false;
	}

	gpEdgeDetection = LoadShader("EdgeDetection.fx");
	if (!gpEdgeDetection)
	{
		return false;
	}

	gpEmboss = LoadShader("Emboss.fx");
	if (!gpEmboss)
	{
		return false;
	}

	// loading models
	gpTeapot = LoadModel("TeapotWithTangent.x");
	if (!gpTeapot)
	{
		return false;
	}

	return true;
}

// loading shaders
LPD3DXEFFECT LoadShader(const char * filename)
{
	LPD3DXEFFECT ret = NULL;

	LPD3DXBUFFER pError = NULL;
	DWORD dwShaderFlags = 0;

#if _DEBUG
	dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

	D3DXCreateEffectFromFile(gpD3DDevice, filename,
		NULL, NULL, dwShaderFlags, NULL, &ret, &pError);

	// if failed at loading shaders, display compile error
	// to output window
	if (!ret && pError)
	{
		int size = pError->GetBufferSize();
		void *ack = pError->GetBufferPointer();

		if (ack)
		{
			char* str = new char[size];
			sprintf(str, (const char*)ack, size);
			OutputDebugString(str);
			delete[] str;
		}
	}

	return ret;
}

// loading models
LPD3DXMESH LoadModel(const char * filename)
{
	LPD3DXMESH ret = NULL;
	if (FAILED(D3DXLoadMeshFromX(filename, D3DXMESH_SYSTEMMEM, gpD3DDevice, NULL, NULL, NULL, NULL, &ret)))
	{
		OutputDebugString("failed at loading a model: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	};

	return ret;
}

// loading textures
LPDIRECT3DTEXTURE9 LoadTexture(const char * filename)
{
	LPDIRECT3DTEXTURE9 ret = NULL;
	if (FAILED(D3DXCreateTextureFromFile(gpD3DDevice, filename, &ret)))
	{
		OutputDebugString("failed at loading a texture: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	}

	return ret;
}
//------------------------------------------------------------
// cleanup code
//------------------------------------------------------------

void Cleanup()
{
	// release fonts
	if (gpFont)
	{
		gpFont->Release();
		gpFont = NULL;
	}

	// release models
	if (gpTeapot)
	{
		gpTeapot->Release();
		gpTeapot = NULL;
	}

	// release shaders
	if (gpEnvironmentMappingShader)
	{
		gpEnvironmentMappingShader->Release();
		gpEnvironmentMappingShader = NULL;
	}

	if (gpNoEffect)
	{
		gpNoEffect->Release();
		gpNoEffect = NULL;
	}

	if (gpGrayScale)
	{
		gpGrayScale->Release();
		gpGrayScale = NULL;
	}

	if (gpSepia)
	{
		gpSepia->Release();
		gpSepia = NULL;
	}

	if (gpEdgeDetection)
	{
		gpEdgeDetection->Release();
		gpEdgeDetection = NULL;
	}
	if (gpEmboss)
	{
		gpEmboss->Release();
		gpEmboss = NULL;
	}

	// release textures
	if (gpStoneDM)
	{
		gpStoneDM->Release();
		gpStoneDM = NULL;
	}

	if (gpStoneSM)
	{
		gpStoneSM->Release();
		gpStoneSM = NULL;
	}

	if (gpStoneNM)
	{
		gpStoneNM->Release();
		gpStoneNM = NULL;
	}

	if (gpSnowENV)
	{
		gpSnowENV->Release();
		gpSnowENV = NULL;
	}

	// Release the fullscreen quad
	if (gpFullscreenQuadDecl)
	{
		gpFullscreenQuadDecl->Release();
		gpFullscreenQuadDecl = NULL;
	}

	if (gpFullscreenQuadVB)
	{
		gpFullscreenQuadVB->Release();
		gpFullscreenQuadVB = NULL;
	}

	if (gpFullscreenQuadIB)
	{
		gpFullscreenQuadIB->Release();
		gpFullscreenQuadIB = NULL;
	}

	// release the render target
	if (gpSceneRenderTarget)
	{
		gpSceneRenderTarget->Release();
		gpSceneRenderTarget = NULL;
	}

	// release D3D
	if (gpD3DDevice)
	{
		gpD3DDevice->Release();
		gpD3DDevice = NULL;
	}

	if (gpD3D)
	{
		gpD3D->Release();
		gpD3D = NULL;
	}
}

void InitFullScreenQuad()
{
	// vertex declaration
	D3DVERTEXELEMENT9 vtxDesc[3];
	int offset = 0;
	int i = 0;

	// position
	vtxDesc[i].Stream = 0;
	vtxDesc[i].Offset = offset;
	vtxDesc[i].Type = D3DDECLTYPE_FLOAT3;
	vtxDesc[i].Method = D3DDECLMETHOD_DEFAULT;
	vtxDesc[i].Usage = D3DDECLUSAGE_POSITION;
	vtxDesc[i].UsageIndex = 0;

	offset += sizeof(float)* 3;
	++i;

	// UV coords 0
	vtxDesc[i].Stream = 0;
	vtxDesc[i].Offset = offset;
	vtxDesc[i].Type = D3DDECLTYPE_FLOAT2;
	vtxDesc[i].Method = D3DDECLMETHOD_DEFAULT;
	vtxDesc[i].Usage = D3DDECLUSAGE_TEXCOORD;
	vtxDesc[i].UsageIndex = 0;

	offset += sizeof(float)* 2;
	++i;

	// end of the vertex format (D3DDECL_END())
	vtxDesc[i].Stream = 0xFF;
	vtxDesc[i].Offset = 0;
	vtxDesc[i].Type = D3DDECLTYPE_UNUSED;
	vtxDesc[i].Method = 0;
	vtxDesc[i].Usage = 0;
	vtxDesc[i].UsageIndex = 0;

	gpD3DDevice->CreateVertexDeclaration(vtxDesc, &gpFullscreenQuadDecl);

	// create a vertex buffer
	gpD3DDevice->CreateVertexBuffer(offset * 4, 0, 0, D3DPOOL_MANAGED, &gpFullscreenQuadVB, NULL);
	void * vertexData = NULL;
	gpFullscreenQuadVB->Lock(0, 0, &vertexData, 0);
	{
		float * data = (float*)vertexData;
		*data++ = -1.0f;	*data++ = 1.0f;		*data++ = 0.0f;
		*data++ = 0.0f;		*data++ = 0.0f;

		*data++ = 1.0f;		*data++ = 1.0f;		*data++ = 0.0f;
		*data++ = 1.0f;		*data++ = 0;

		*data++ = 1.0f;		*data++ = -1.0f;	*data++ = 0.0f;
		*data++ = 1.0f;		*data++ = 1.0f;

		*data++ = -1.0f;	*data++ = -1.0f;	*data++ = 0.0f;
		*data++ = 0.0f;		*data++ = 1.0f;
	}
	gpFullscreenQuadVB->Unlock();

	// create an index buffer
	gpD3DDevice->CreateIndexBuffer(sizeof(short)* 6, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &gpFullscreenQuadIB, NULL);
	void * indexData = NULL;
	gpFullscreenQuadIB->Lock(0, 0, &indexData, 0);
	{
		unsigned short * data = (unsigned short*)indexData;
		*data++ = 0;	*data++ = 1;	*data++ = 3;
		*data++ = 3;	*data++ = 1;	*data++ = 2;
	}
	gpFullscreenQuadIB->Unlock();
}
