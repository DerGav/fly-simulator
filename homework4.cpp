//--------------------------------------------------------------------------------------
// File: lecture 8.cpp
//
// This application demonstrates texturing
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "render_to_texture.h"
#include "Font.h"
#include <sstream>
//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;
ID3D11PixelShader*                  g_pPixelShader_shadow = NULL;
ID3D11PixelShader*                  g_pPSsky = NULL;

RenderTextureClass					RenderToTexture;
ID3D11VertexShader*                 g_pVertexShader_screen = NULL;
ID3D11PixelShader*                  g_pPixelShader_screen = NULL;
ID3D11InputLayout*                  g_pVertexLayout = NULL;
ID3D11Buffer*                       g_pVertexBuffer = NULL;
ID3D11Buffer*                       g_pVertexBuffer_screen = NULL;
ID3D11Buffer*                       g_pVertexBuffer_sky = NULL;
ID3D11Buffer*                       g_pVertexBuffer_test = NULL;

//shadow stuff:
RenderTextureClass					ShadowMap;




//states for turning off and on the depth buffer
ID3D11DepthStencilState				*ds_on, *ds_off;
ID3D11BlendState*					g_BlendState;

ID3D11Buffer*                       g_pCBuffer = NULL;
ID3D11ShaderResourceView*			g_pTextureSky;

ID3D11ShaderResourceView*			redTex;
ID3D11ShaderResourceView*			greenTex;
ID3D11ShaderResourceView*			blueTex;

ID3D11ShaderResourceView*			g_pTextureGrape;
ID3D11ShaderResourceView*			g_pTextureTest;

int									model_vertex_sky = 0;
ID3D11RasterizerState				*rs_CW, *rs_CCW, *rs_NO, *rs_Wire;

ID3D11SamplerState*                 g_pSamplerLinear = NULL;
ID3D11SamplerState*                 SamplerScreen = NULL;

XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Light;

XMMATRIX                            g_Projection;
XMMATRIX                            g_ProjectionLight;
XMFLOAT4                            g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);



vector<Model>						models;

camera								cam;
level								level1;
billboard							score;
ID3D11ShaderResourceView*			g_pTextureScore;

Model								table;
Model								centerTable;
Model								House;
Model								test;
Model								sofa;
Model								sideboard;
Model								kitchen;

bool								update_boundaries = true;
bool								collision_flag = true;
music_ music;
int track1;
int track2;
int track3;
int track4;
bool first_loop = true;
bool closeEnough = false;
int collected = 0;
bool useBoost = false;
bool first_pass_boost = true;
//int speedMultiplier = 1;
Font font;

bool render_boundary_test = false;
//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();
//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}
	srand(time(0));
	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 1280, 720 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 7", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;


	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;



	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Compile the vertex shader
	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Compile the vertex shader
	pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VS_screen", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader_screen);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile the pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS_Shadow", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader_shadow);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile the pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS_screen", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader_screen);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile the pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSsky", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPSsky);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;


	//create skybox vertex buffer

	// Create vertex buffer
	SimpleVertex vertices_screen[] =
	{
		{ XMFLOAT3(-1,1,0),XMFLOAT2(0,0),XMFLOAT3(0,0,1) },
		{ XMFLOAT3(1,1,0),XMFLOAT2(1,0),XMFLOAT3(0,0,1) },
		{ XMFLOAT3(-1,-1,0),XMFLOAT2(0,1),XMFLOAT3(0,0,1) },
		{ XMFLOAT3(1,1,0),XMFLOAT2(1,0),XMFLOAT3(0,0,1) },
		{ XMFLOAT3(1,-1,0),XMFLOAT2(1,1),XMFLOAT3(0,0,1) },
		{ XMFLOAT3(-1,-1,0),XMFLOAT2(0,1),XMFLOAT3(0,0,1) }
	};

	//initialize d3dx verexbuff:
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices_screen;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer_screen);
	if (FAILED(hr))
		return FALSE;

	//D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	font.init(g_pd3dDevice, g_pImmediateContext, font.defaultFontMapDesc);
	//Load skysphere
	LoadCatmullClark(L"ccsphere.cmp", g_pd3dDevice, &g_pVertexBuffer_sky, &model_vertex_sky);

	House.load_Model("house.3ds", g_pd3dDevice, COORD, FALSE);
	sideboard.load_Model("test/sideboard.3ds", g_pd3dDevice, COORD, FALSE);
	//test.load_Model("test/Kitchen.3ds", g_pd3dDevice, COORD, FALSE);
	kitchen.load_Model("test/Kitchen.3ds", g_pd3dDevice, COORD, FALSE);
	sofa.load_Model("test/sofa.3ds", g_pd3dDevice, COORD, FALSE);
	table.load_Model("simpleTable.3ds", g_pd3dDevice, COORD, FALSE); // file name, g_p3dDevice, type of collision detection, gourad shading
	centerTable.load_Model("centerTable/centerTable.3ds", g_pd3dDevice, BOX, FALSE);
	
	models.push_back(table); //0 
	models.push_back(House);
	models.push_back(sideboard); //2
	models.push_back(kitchen);
	models.push_back(sofa); //4
	cam.models = &models;
	// Set vertex buffer
	SimpleVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },

		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }

	};

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffers
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBuffer);
	if (FAILED(hr))
		return hr;

	// Load the Texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"greenGrape.jpg", NULL, NULL, &g_pTextureGrape, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"test/kitchenTex.jpg", NULL, NULL, &test.g_pTexture, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"test/kitchenTex.jpg", NULL, NULL, &kitchen.g_pTexture, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"test/blackL.jpg", NULL, NULL, &sofa.g_pTexture, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"test/red.jpg", NULL, NULL, &redTex, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"test/green.jpg", NULL, NULL, &greenTex, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"test/blue.png", NULL, NULL, &blueTex, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"test/blackW.jpg", NULL, NULL, &sideboard.g_pTexture, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"old-wood-texture.jpg", NULL, NULL, &House.g_pTexture, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"dayBox.png", NULL, NULL, &g_pTextureSky, NULL);
	if (FAILED(hr))
		return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"centerTable/centerTableTex.jpg", NULL, NULL, &table.g_pTexture, NULL);
	if (FAILED(hr))
		return hr;


	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	if (FAILED(hr))
		return hr;
	// Create the screen sample state

	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &SamplerScreen);
	if (FAILED(hr))
		return hr;


	// Initialize the world matrices
	g_World = XMMatrixIdentity();

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);//camera position
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);//look at
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);// normal vector on at vector (always up)
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	//for light source
	At = XMVectorSet(0.0f, 80.0f, 0.0f, 0.0f);//camera position
	Eye = XMVectorSet(1000.0f, 400.0f, 1000.0f, 0.0f);//look at
	Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);// normal vector on at vector (always up)
	g_Light = XMMatrixLookAtLH(Eye, At, Up);

	// Initialize the projection matrix
	g_ProjectionLight = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 100000.0f);

	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, width / (FLOAT)height, 0.01f, 100000.0f);

	ConstantBuffer constantbuffer;
	constantbuffer.View = XMMatrixTranspose(g_View);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.World = XMMatrixTranspose(XMMatrixIdentity());
	constantbuffer.info = XMFLOAT4(0, 1, 1, 1);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	//blendstate:
	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	g_pd3dDevice->CreateBlendState(&blendStateDesc, &g_BlendState);


	float blendFactor[] = { 0, 0, 0, 0 };
	UINT sampleMask = 0xffffffff;
	g_pImmediateContext->OMSetBlendState(g_BlendState, blendFactor, sampleMask);


	//create the depth stencil states for turning the depth buffer on and of:
	D3D11_DEPTH_STENCIL_DESC		DS_ON, DS_OFF;
	DS_ON.DepthEnable = true;
	DS_ON.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_ON.DepthFunc = D3D11_COMPARISON_LESS;
	// Stencil test parameters
	DS_ON.StencilEnable = true;
	DS_ON.StencilReadMask = 0xFF;
	DS_ON.StencilWriteMask = 0xFF;
	// Stencil operations if pixel is front-facing
	DS_ON.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DS_ON.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Stencil operations if pixel is back-facing
	DS_ON.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DS_ON.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Create depth stencil state
	DS_OFF = DS_ON;
	DS_OFF.DepthEnable = false;
	g_pd3dDevice->CreateDepthStencilState(&DS_ON, &ds_on);
	g_pd3dDevice->CreateDepthStencilState(&DS_OFF, &ds_off);


	//setting the rasterizer:
	D3D11_RASTERIZER_DESC			RS_CW, RS_Wire;

	RS_CW.AntialiasedLineEnable = FALSE;
	RS_CW.CullMode = D3D11_CULL_BACK;
	RS_CW.DepthBias = 0;
	RS_CW.DepthBiasClamp = 0.0f;
	RS_CW.DepthClipEnable = true;
	RS_CW.FillMode = D3D11_FILL_SOLID;
	RS_CW.FrontCounterClockwise = false;
	RS_CW.MultisampleEnable = FALSE;
	RS_CW.ScissorEnable = false;
	RS_CW.SlopeScaledDepthBias = 0.0f;

	RS_Wire = RS_CW;
	RS_Wire.CullMode = D3D11_CULL_NONE;
	RS_Wire.FillMode = D3D11_FILL_WIREFRAME;
	g_pd3dDevice->CreateRasterizerState(&RS_Wire, &rs_Wire);
	g_pd3dDevice->CreateRasterizerState(&RS_CW, &rs_CW);

	//rendertarget texture
	RenderToTexture.Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R8G8B8A8_UNORM, TRUE);
	ShadowMap.Initialize(g_pd3dDevice, g_hWnd, -1, -1, FALSE, DXGI_FORMAT_R32G32B32A32_FLOAT, TRUE);


	track1 = music.init_music("sounds/fly_wind_normal.mp3");
	track2 = music.init_music("sounds/fly_wind_fast.mp3");
	track3 = music.init_music("sounds/fly_wind_dying.mp3");
	track3 = music.init_music("sounds/wind.mp3");
	//music.set_auto_fadein_fadeout(true);

	//music.play(track1);
	//cam.music = &music;
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pSamplerLinear) g_pSamplerLinear->Release();
	//if (g_pTextureRV) g_pTextureRV->Release();
	if (g_pCBuffer) g_pCBuffer->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is down
///////////////////////////////////

void OnLBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{

}

///////////////////////////////////
//		This Function is called every time the Right Mouse Button is down
///////////////////////////////////
void OnRBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{

}
///////////////////////////////////
//		This Function is called every time a character key is pressed
///////////////////////////////////
void OnChar(HWND hwnd, UINT ch, int cRepeat)
{

}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is up
///////////////////////////////////
void OnLBU(HWND hwnd, int x, int y, UINT keyFlags)
{


}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is up
///////////////////////////////////
void OnRBU(HWND hwnd, int x, int y, UINT keyFlags)
{


}
///////////////////////////////////
//		This Function is called every time the Mouse Moves
///////////////////////////////////
void OnMM(HWND hwnd, int x, int y, UINT keyFlags)
{
	static int holdx = x, holdy = y;
	static int reset_cursor = 0;



	RECT rc; 			//rectange structure
	GetWindowRect(hwnd, &rc); 	//retrieves the window size
	int border = 20;
	rc.bottom -= border;
	rc.right -= border;
	rc.left += border;
	rc.top += border;
	ClipCursor(&rc);

	if ((keyFlags & MK_LBUTTON) == MK_LBUTTON)
	{
	}

	if ((keyFlags & MK_RBUTTON) == MK_RBUTTON)
	{
	}
	if (reset_cursor == 1)
	{
		reset_cursor = 0;
		holdx = x;
		holdy = y;
		return;
	}
	int diffx = holdx - x;
	int diffy = holdy - y;
	float angle_y = (float)diffx / 300.0;
	float angle_x = (float)diffy / 300.0;
	

		cam.rotation.y += angle_y;
		cam.rotation.x += angle_x;



	int midx = (rc.left + rc.right) / 2;
	int midy = (rc.top + rc.bottom) / 2;
	SetCursorPos(midx, midy);
	reset_cursor = 1;
}


BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
	RECT rc; 			//rectange structure
	GetWindowRect(hwnd, &rc); 	//retrieves the window size
	int border = 5;
	rc.bottom -= border;
	rc.right -= border;
	rc.left += border;
	rc.top += border;
	ClipCursor(&rc);
	int midx = (rc.left + rc.right) / 2;
	int midy = (rc.top + rc.bottom) / 2;
	SetCursorPos(midx, midy);
	return TRUE;
}
void OnTimer(HWND hwnd, UINT id)
{

}
//*************************************************************************
void OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	switch (vk)
	{
	case 81://q
		cam.q = 0; break;
	case 69://e
		cam.e = 0; break;
	case 65:cam.a = 0;//a
		break;
	case 68: cam.d = 0;//d
		break;
	case 89:
		closeEnough = false;
		break;
	case 32: //space
		useBoost = false;

		break;
	case 87: cam.w = 0; //w
		break;
	case 83:cam.s = 0; //s
	default:break;

	}

}

void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{

	switch (vk)
	{
	default:break;
	case 81://q
		//music.fade_out(1, 100);
		//cam.rotation.z += XM_PI;
		cam.q = 1; break;
	case 69://e
		cam.e = 1; break;
	case 65:cam.a = 1;//a
		break;
	case 68: cam.d = 1;//d
		break;
	case 89:
		closeEnough = true;
		break;
	case 32: //space
		if (cam.flying)
			useBoost = true;
		else
		{
			music.play(track1);
			cam.flying = true;
			cam.position = cam.position - cam.normal *XMFLOAT3(15, 15, 15);
		}
		break;
	case 87: cam.w = 1; //w
		break;
	case 83:cam.s = 1; //s
		break;
	case 27: PostQuitMessage(0);//escape
		break;

	case 84://t
	{
		static int laststate = 0;
		if (laststate == 0)
		{
			g_pImmediateContext->RSSetState(rs_Wire);
			laststate = 1;
		}
		else
		{
			g_pImmediateContext->RSSetState(rs_CW);
			laststate = 0;
		}

	}
	break;

	}
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
#include <windowsx.h>
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		HANDLE_MSG(hWnd, WM_LBUTTONDOWN, OnLBD);
		HANDLE_MSG(hWnd, WM_LBUTTONUP, OnLBU);
		HANDLE_MSG(hWnd, WM_MOUSEMOVE, OnMM);
		HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hWnd, WM_TIMER, OnTimer);
		HANDLE_MSG(hWnd, WM_KEYDOWN, OnKeyDown);
		HANDLE_MSG(hWnd, WM_KEYUP, OnKeyUp);
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

float grapeSize1 = .1;
float grapeSize2 = .1;
float grapeSize3 = .1;
string test2 = "||||||||||";
int count1 = 0;

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------

//############################################################################################################
void Render_to_texture(long elapsed)
{
	if (first_loop)
		music.play(track1);

	float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
	ID3D11RenderTargetView*			RenderTarget;

	RenderTarget = RenderToTexture.GetRenderTarget();
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &RenderTarget, g_pDepthStencilView);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	XMMATRIX view = cam.get_matrix(&g_View);

	if (collected == 3) {
		PostQuitMessage(0);
	}
	static StopWatchMicro_ stopwatch;
	//		long elapsed = stopwatch.elapse_micro();
	stopwatch.start();//restart


	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	//g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);


	// Clear the back buffer
	//float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
	//g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	// Clear the depth buffer to 1.0 (max depth)


	//cam.animation(elapsed);
	//XMMATRIX view = cam.get_matrix(&g_View);

	XMMATRIX Iview = view;
	Iview._41 = Iview._42 = Iview._43 = 0.0;
	XMVECTOR det;
	Iview = XMMatrixInverse(&det, Iview);

	//ENTER MATRIX CALCULATION HERE
	XMMATRIX worldmatrix;
	worldmatrix = XMMatrixIdentity();
	//XMMATRIX S, T, R, R2;
	//worldmatrix = .... probably define some other matrices here!



	// Update constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.LightView = XMMatrixTranspose(g_Light);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.info = XMFLOAT4(cam.hit, 0, 0, 0);
	//constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);

	XMMATRIX T, R, M, S;

	/*SkySphere*/
	S = XMMatrixScaling(10, 10, 10);
	T = XMMatrixTranslation(-cam.position.x, -cam.position.y, -cam.position.z);
	R = XMMatrixRotationX(-XM_PIDIV2);
	M = S * T;
	constantbuffer.World = XMMatrixTranspose(M);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPSsky, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureSky);
	g_pImmediateContext->VSSetShaderResources(0, 1, &g_pTextureSky);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sky, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);
	g_pImmediateContext->Draw(model_vertex_sky, 0);
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);


	//table
	float scale = 15;
	S = XMMatrixScaling(scale, scale, scale);
	R = XMMatrixRotationX(-XM_PIDIV2);
	T = XMMatrixTranslation(350, 5.6, -120);
	M = S * R * T;
	constantbuffer.World = XMMatrixTranspose(M);

	if (update_boundaries)
		models[0].boundary->transform_boundary(M, scale);

	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	// Render terrain
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &table.g_pTexture);
	g_pImmediateContext->VSSetShaderResources(0, 1, &table.g_pTexture);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &table.vertex_buffer, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(table.vertex_count, 0);

	/**************************SIDEBOARD***************************/

	S = XMMatrixScaling(.2 ,.2,.2);
	T = XMMatrixTranslation(-500,5 , 200);
	R = XMMatrixRotationX(-XM_PIDIV2);
	XMMATRIX Ry = XMMatrixRotationY(XM_PIDIV2);
	M = S *R * Ry * T;

	constantbuffer.World = XMMatrixTranspose(M);

	if (update_boundaries)
		models[2].boundary->transform_boundary(M, .2);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &sideboard.g_pTexture);
	g_pImmediateContext->VSSetShaderResources(0, 1, &sideboard.g_pTexture);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &sideboard.vertex_buffer, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(sideboard.vertex_count, 0);


	/**************************TEST***************************/

	S = XMMatrixScaling(30, 30, 30);
	T = XMMatrixTranslation(-250, 5, 200);
	R = XMMatrixRotationX(-XM_PIDIV2);
	Ry = XMMatrixRotationY(-XM_PIDIV2);
	M = S *R * Ry * T;
	constantbuffer.World = XMMatrixTranspose(M);

	if (update_boundaries)
		models[4].boundary->transform_boundary(M, 30);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &sofa.g_pTexture);
	g_pImmediateContext->VSSetShaderResources(0, 1, &sofa.g_pTexture);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &sofa.vertex_buffer, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(sofa.vertex_count, 0);


	/***************************Kitchen***************************/

	S = XMMatrixScaling(40, 40, 40);
	T = XMMatrixTranslation(330, 5, 130);
	R = XMMatrixRotationX(-XM_PIDIV2);
	Ry = XMMatrixRotationY(XM_PI);
	M = S *R * Ry * T;

	if (update_boundaries)
		models[3].boundary->transform_boundary(M, 40);

	constantbuffer.World = XMMatrixTranspose(M);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &kitchen.g_pTexture);
	g_pImmediateContext->VSSetShaderResources(0, 1, &kitchen.g_pTexture);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &kitchen.vertex_buffer, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(kitchen.vertex_count, 0);


	/***************************TEST***************************/


	S = XMMatrixScaling(1, 1, 1);
	T = XMMatrixTranslation(0, 20, 0);
	R = XMMatrixRotationX(-XM_PIDIV2);
	Ry = XMMatrixRotationY(XM_PIDIV2);
	M = S *R * Ry * T;

	constantbuffer.World = XMMatrixTranspose(M);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &test.g_pTexture);
	g_pImmediateContext->VSSetShaderResources(0, 1, &test.g_pTexture);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &test.vertex_buffer, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(test.vertex_count, 0);

	//grape
	if (grapeSize1 > .05) {
		if (closeEnough == true) {
			grapeSize1 += -grapeSize1 / 1000;
		}
		S = XMMatrixScaling(grapeSize1, grapeSize1, grapeSize1);
		T = XMMatrixTranslation(0, 20, 0);
		M = S * T;
		constantbuffer.World = XMMatrixTranspose(M);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
		g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureGrape);
		g_pImmediateContext->VSSetShaderResources(0, 1, &g_pTextureGrape);
		g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sky, &stride, &offset);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
		g_pImmediateContext->Draw(model_vertex_sky, 0);
	}
	else {
		if (grapeSize1 != 0) {	//still has not been set to zero so it will allow it to be collected only once
			collected++;
		}
		grapeSize1 = 0;
	}

	if (grapeSize2 > .05) {
		if (closeEnough == true) {
			grapeSize2 += -grapeSize2 / 1000;
		}
		S = XMMatrixScaling(grapeSize2, grapeSize2, grapeSize2);
		T = XMMatrixTranslation(505, 20, 130);
		M = S * T;
		constantbuffer.World = XMMatrixTranspose(M);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
		g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureGrape);
		g_pImmediateContext->VSSetShaderResources(0, 1, &g_pTextureGrape);
		g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sky, &stride, &offset);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
		g_pImmediateContext->Draw(model_vertex_sky, 0);
	}
	else {
		if (grapeSize2 != 0) {	//still has not been set to zero so it will allow it to be collected only once
			collected++;
		}
		grapeSize2 = 0;
	}


	/*House*/

	S = XMMatrixScaling(1, 1, 1);
	R = XMMatrixTranslation(0, 0, 0);
	T = XMMatrixRotationX(-XM_PIDIV2);
	M = S * R * T;
	//M = XMMatrixIdentity();



	if (update_boundaries)
		models[1].boundary->transform_boundary(M, 1);

	constantbuffer.World = XMMatrixTranspose(M);


	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	// Render terrain
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetShaderResources(0, 1, &House.g_pTexture);
	g_pImmediateContext->VSSetShaderResources(0, 1, &House.g_pTexture);

	ID3D11ShaderResourceView*           texture = ShadowMap.GetShaderResourceView();// THE MAGIC
	g_pImmediateContext->PSSetShaderResources(1, 1, &texture);

	g_pImmediateContext->IASetVertexBuffers(0, 1, &House.vertex_buffer, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(House.vertex_count, 0);

	
	//for (int x = 1; x <= 3; x++) { // change x < Num to # number of items

	//	M = models[x].scale * models[x].rotate * models[x].translate;
	//	constantbuffer.World = XMMatrixTranspose(M);
	//	models[x].update_objectSphere(&collision_detection, M, models[x].scale);

	//	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	//	// Render terrain
	//	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	//	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	//	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	//	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	//	g_pImmediateContext->PSSetShaderResources(0, 1, &models[x].texture);
	//	g_pImmediateContext->VSSetShaderResources(0, 1, &models[x].texture);
	//	g_pImmediateContext->IASetVertexBuffers(0, 1, &models[x].vertex_buffer, &stride, &offset);
	//	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	//	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	//	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	//	g_pImmediateContext->Draw(models[x].vertex_count, 0);
	//}
	stringstream ss(stringstream::in | stringstream::out);
	stringstream ss2(stringstream::in | stringstream::out);
	ss << "Items Collected: " << collected;
	font.setScaling(XMFLOAT3(1.5, 1.5, 1.5));
	font.setColor(XMFLOAT3(255, 0, 0));
	font.setPosition(XMFLOAT3(-.9, .9, 0));
	string test = ss.str();

	if (useBoost == true) {
		if (first_pass_boost)
		{
			music.play_fx("sounds/fly_wind_fast_fx.mp3");
			first_pass_boost = false;
		}
		if (count1 % 100 == 0 && test2.length() > 0) {
			test2 = test2.substr(0, test2.length() - 1);
			cam.speedMultiplier = 7;
			count1 = 0;
		}
		else if (test2.length() == 0) {
			cam.speedMultiplier = 1;
		}
	}
	else if (useBoost == false && test2.length() < 10) {//add some back
		cam.speedMultiplier = 1;
		if (count1 % 100 == 0) {
			test2.append("|");
		}
		first_pass_boost = true;
	}
	count1++;
	ss2 << count1;
	string test3 = ss2.str();
	test = test + "\n" + "Boost: " + test2; // +"\nelapsed: " + test3;

	font << test;

	if (update_boundaries)
		update_boundaries = false;


	//
	// Present our back buffer to our front buffer
	//
	g_pSwapChain->Present(0, 0);

	if (cam.hit)
	{
		if (collision_flag)
		{
			music.play_fx("sounds/fly_wind_dying_fx.mp3");
			//music.play_fx("sounds/thud.mp3");
			collision_flag = false;
		}
		//music.fade_out(1,10);
		//music.play(3);
	}
	else
	{
		collision_flag = true;
		//music.fade_out(3, 100);
	}
	if (!cam.flying)
	{
		music.fade_out(track1, 10);
		music.fade_in_and_play(track4,100);
	}
	else{
	}

	

}
void Render_to_shadowmap(long elapsed)
{
	float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
	ID3D11RenderTargetView*			RenderTarget;

	RenderTarget = ShadowMap.GetRenderTarget();
	g_pImmediateContext->ClearRenderTargetView(RenderTarget, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &RenderTarget, g_pDepthStencilView);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	XMMATRIX view = g_Light;
	XMMATRIX persp = g_ProjectionLight;
	g_pImmediateContext->PSSetShader(g_pPixelShader_shadow, NULL, 0);

	//render_objects(view, elapsed);

	
	if (collected == 3) {
		PostQuitMessage(0);
	}
	static StopWatchMicro_ stopwatch;
	//		long elapsed = stopwatch.elapse_micro();
	stopwatch.start();//restart

	//changeModels(&models[10]);


	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	//g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);


	// Clear the back buffer
	//float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
	//g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	// Clear the depth buffer to 1.0 (max depth)


	//cam.animation(elapsed);
	//XMMATRIX view = cam.get_matrix(&g_View);

	XMMATRIX Iview = view;
	Iview._41 = Iview._42 = Iview._43 = 0.0;
	XMVECTOR det;
	Iview = XMMatrixInverse(&det, Iview);

	//ENTER MATRIX CALCULATION HERE
	XMMATRIX worldmatrix;
	worldmatrix = XMMatrixIdentity();
	//XMMATRIX S, T, R, R2;
	//worldmatrix = .... probably define some other matrices here!



	// Update constant buffer
	ConstantBuffer constantbuffer;
	constantbuffer.LightView = XMMatrixTranspose(g_Light);
	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(persp);
	//constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);

	XMMATRIX T, R, M, S;


	//grape
	if (grapeSize1 > .05) {
		if (closeEnough == true) {
			grapeSize1 += -grapeSize1 / 1000;
		}
		S = XMMatrixScaling(grapeSize1, grapeSize1, grapeSize1);
		T = XMMatrixTranslation(0, 20, 0);
		R = XMMatrixRotationX(-XM_PIDIV2);
		M = S * T;
		constantbuffer.World = XMMatrixTranspose(M);
		g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

		g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
		g_pImmediateContext->VSSetShaderResources(0, 1, &g_pTextureGrape);
		g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_sky, &stride, &offset);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

		g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
		g_pImmediateContext->Draw(model_vertex_sky, 0);
	}
	else {
		if (grapeSize1 != 0) {	//still has not been set to zero so it will allow it to be collected only once
			collected++;
		}
		grapeSize1 = 0;
	}


	/*House*/

	S = XMMatrixScaling(1, 1, 1);
	R = XMMatrixTranslation(0, 0, 0);
	T = XMMatrixRotationX(-XM_PIDIV2);
	M = S * R * T;

	constantbuffer.World = XMMatrixTranspose(M);
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	// Render terrain
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->VSSetShaderResources(0, 1, &House.g_pTexture);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &House.vertex_buffer, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(House.vertex_count, 0);

	//for (int x = 1; x <= 3; x++) { // change x < Num to # number of items

	//	M = models[x].scale * models[x].rotate * models[x].translate;
	//	constantbuffer.World = XMMatrixTranspose(M);
	//	models[x].update_objectSphere(&collision_detection, M, models[x].scale);

	//	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);
	//	// Render terrain
	//	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	//	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	//	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);
	//	g_pImmediateContext->VSSetShaderResources(0, 1, &models[x].texture);
	//	g_pImmediateContext->IASetVertexBuffers(0, 1, &models[x].vertex_buffer, &stride, &offset);
	//	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	//	g_pImmediateContext->VSSetSamplers(0, 1, &g_pSamplerLinear);

	//	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	//	g_pImmediateContext->Draw(models[x].vertex_count, 0);
	//}

	stringstream ss(stringstream::in | stringstream::out);
	stringstream ss2(stringstream::in | stringstream::out);

	ss << "Items Collected: " << collected;
	font.setScaling(XMFLOAT3(1.5, 1.5, 1.5));
	font.setColor(XMFLOAT3(255, 0, 0));
	font.setPosition(XMFLOAT3(-.9, .9, 0));
	string test = ss.str();

	if (useBoost == true) {
		if (count1 % 100 == 0 && test2.length() > 0) {
			test2 = test2.substr(0, test2.length() - 1);
			cam.speedMultiplier = 7;
			count1 = 0;
		}
		else if (test2.length() == 0) {
			cam.speedMultiplier = 1;
		}
	}
	else if (useBoost == false && test2.length() < 10) {//add some back
		cam.speedMultiplier = 1;
		if (count1 % 100 == 0) {
			test2.append("|");
		}

	}
	count1++;
	ss2 << count1;
	string test3 = ss2.str();
	test = test + "\n" + "Boost: " + test2; // +"\nelapsed: " + test3;

	font << test;

	//
	// Present our back buffer to our front buffer
	//
	g_pSwapChain->Present(0, 0);


}

//############################################################################################################
void Render_to_screen(long elapsed)
{
	//and now render it on the screen:
	ConstantBuffer constantbuffer;
	XMMATRIX view = cam.get_matrix(&g_View);
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	constantbuffer.View = XMMatrixTranspose(view);
	constantbuffer.Projection = XMMatrixTranspose(g_Projection);
	constantbuffer.LightView = XMMatrixTranspose(g_Light);  
	constantbuffer.info.x = cam.hit;
//constantbuffer.CameraPos = XMFLOAT4(cam.position.x, cam.position.y, cam.position.z, 1);

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	// Clear the back buffer
	float ClearColor2[4] = { 0.0f, 1.0f, 0.0f, 1.0f }; // red, green, blue, alpha

	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor2);
	// Clear the depth buffer to 1.0 (max depth)
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);


	constantbuffer.World = XMMatrixIdentity();
	g_pImmediateContext->UpdateSubresource(g_pCBuffer, 0, NULL, &constantbuffer, 0, 0);

	// Render screen

	g_pImmediateContext->VSSetShader(g_pVertexShader_screen, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader_screen, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBuffer);

	ID3D11ShaderResourceView*           texture = RenderToTexture.GetShaderResourceView();// THE MAGIC
	//ID3D11ShaderResourceView*           texture = ShadowMap.GetShaderResourceView();// THE MAGIC
	g_pImmediateContext->GenerateMips(texture);
	//texture = g_pTextureRV;
	g_pImmediateContext->PSSetShaderResources(0, 1, &texture);
	g_pImmediateContext->VSSetShaderResources(0, 1, &texture);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer_screen, &stride, &offset);
	g_pImmediateContext->PSSetSamplers(0, 1, &SamplerScreen);
	g_pImmediateContext->VSSetSamplers(0, 1, &SamplerScreen);

	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	g_pImmediateContext->Draw(6, 0);

	g_pSwapChain->Present(0, 0);
}
//############################################################################################################
void Render()
{
	static StopWatchMicro_ stopwatch;
	long elapsed = stopwatch.elapse_micro();
	stopwatch.start();//restart
	music.process(elapsed/1000);

	cam.animation(elapsed);

	Render_to_shadowmap(elapsed);
	Render_to_texture(elapsed);
	Render_to_screen(elapsed);
}
