#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include "Camera.h"
#include <dshow.h>   
#include "Meshes.h"
#include <dinput.h>
#include <strsafe.h>

//We define an event id 
#define WM_GRAPHNOTIFY  WM_APP + 1   
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
LPDIRECT3D9             D3D           = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       d3dDevice     = NULL; // Our rendering device
LPD3DXMESH              Mesh          = NULL; // Our mesh object in sysmem
D3DMATERIAL9*           MeshMaterials = NULL; // Materials for our mesh
LPDIRECT3DTEXTURE9*     MeshTextures  = NULL; // Textures for our mesh
DWORD                   NumMaterials = 0L;   // Number of mesh materials
CXCamera *camera; 
HWND hWnd;
HDC hdc;


LPDIRECT3DVERTEXBUFFER9 vrtxBuffer = NULL, vrtxBufferCircle = NULL;


LPDIRECTINPUT8			g_pDin;							// the pointer to our DirectInput interface
LPDIRECTINPUTDEVICE8	g_pDinKeyboard;					// the pointer to the keyboard device

BYTE					g_Keystate[256];				// the storage for the key-information

HRESULT InitDInput(HINSTANCE hInstance, HWND hWnd);

VOID DetectInput();
VOID CleanDInput();

float dxTigru2 = 1;
// For playing video
IGraphBuilder* graphBuilder = NULL;
//Help us to start/stop the play
IMediaControl* mediaControl = NULL;
//We receie events in case something happened - during playing, stoping, errors etc..
IMediaEventEx* mediaEvent = NULL;
//We can use to fast forward, revert etc..
IMediaSeeking* mediaSeeking = NULL;

struct CUSTOMVERTEX
{
    FLOAT x, y, z; //Position
    DWORD color; //Colour
    FLOAT tu, tv;   // The texture coordinates
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)



HRESULT InitDirectShow(HWND hWnd)
{
    //Create Filter Graph   
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL,
        CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&graphBuilder);

    //Create Media Control and Events   
    hr = graphBuilder->QueryInterface(IID_IMediaControl, (void**)&mediaControl);
    hr = graphBuilder->QueryInterface(IID_IMediaEventEx, (void**)&mediaEvent);
    hr = graphBuilder->QueryInterface(IID_IMediaSeeking, (void**)&mediaSeeking);

    //Load a file   
    hr = graphBuilder->RenderFile(L"test.avi", NULL);

    //Set window for events  - basically we tell our event in case you raise an event use the following event id.
    mediaEvent->SetNotifyWindow((OAHWND)hWnd, WM_GRAPHNOTIFY, 0);

    //Play media control   
    mediaControl->Run();


    return S_OK;
}
void HandleGraphEvent()
{
    // Get all the events   
    long evCode;
    LONG_PTR param1, param2;

    while (SUCCEEDED(mediaEvent->GetEvent(&evCode, &param1, &param2, 0)))
    {
        mediaEvent->FreeEventParams(evCode, param1, param2);
        switch (evCode)
        {
        case EC_COMPLETE:  // Fall through.   
        case EC_USERABORT: // Fall through.   
        case EC_ERRORABORT:
            PostQuitMessage(0);
            return;
        }
    }
}

//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D(HWND hWnd)
{
    // Create the D3D object.
    if (NULL == (D3D = Direct3DCreate9(D3D_SDK_VERSION)))
        return E_FAIL;

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // Create the D3DDevice
    if (FAILED(D3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp, &d3dDevice)))
    {
        if (FAILED(D3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &d3dpp, &d3dDevice)))
            return E_FAIL;
    }

    // Turn on the zbuffer
    d3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

    // Turn on ambient lighting 
    d3dDevice->SetRenderState(D3DRS_AMBIENT, 0xffffffff);

    // Turn off culling, so we see the front and back of the triangle
    d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: InitDInput(HINSTANCE hInstance, HWND hWnd)
// Desc: Initializes DirectInput
//-----------------------------------------------------------------------------
HRESULT InitDInput(HINSTANCE hInstance, HWND hWnd)
{
    // create the DirectInput interface
    DirectInput8Create(hInstance,    // the handle to the application
        DIRECTINPUT_VERSION,    // the compatible version
        IID_IDirectInput8,    // the DirectInput interface version
        (void**)&g_pDin,    // the pointer to the interface
        NULL);    // COM stuff, so we'll set it to NULL

    // create the keyboard device
    g_pDin->CreateDevice(GUID_SysKeyboard,    // the default keyboard ID being used
        &g_pDinKeyboard,    // the pointer to the device interface
        NULL);    // COM stuff, so we'll set it to NULL

    // set the data format to keyboard format
    g_pDinKeyboard->SetDataFormat(&c_dfDIKeyboard);

    // set the control we will have over the keyboard
    g_pDinKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: DetectInput(void)
// Desc: This is the function that gets the latest input data
//-----------------------------------------------------------------------------
VOID DetectInput()
{
    // get access if we don't have it already
    g_pDinKeyboard->Acquire();

    // get the input data
    g_pDinKeyboard->GetDeviceState(256, (LPVOID)g_Keystate);
}
//-----------------------------------------------------------------------------
// Name: CleanDInput(void)
// Desc: This is the function that closes DirectInput
//-----------------------------------------------------------------------------
VOID CleanDInput()
{
    g_pDinKeyboard->Unacquire();    // make sure the keyboard is unacquired
    g_pDin->Release();    // close DirectInput before exiting
}

void InitiateCamera()
{
    camera = new CXCamera(d3dDevice);

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the 
    // origin, and define "up" to be in the y-direction.
    D3DXVECTOR3 vEyePt(0.0f, 3.0f, -30.0f);
    D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
    D3DXMATRIXA16 matView;

    camera->LookAtPos(&vEyePt, &vLookatPt, &vUpVec);
}

float AngleToRadian(float angle)
{
    return angle * D3DX_PI / 180.0f;
}

//-----------------------------------------------------------------------------
// Name: InitGeometry()
// Desc: Load the mesh and build the material and texture arrays
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
    LPD3DXBUFFER pD3DXMtrlBuffer;

    // Load the mesh from the specified file
    if( FAILED( D3DXLoadMeshFromX( "PanzerTiger.x", D3DXMESH_SYSTEMMEM, 
                                   d3dDevice, NULL, 
                                   &pD3DXMtrlBuffer, NULL, &NumMaterials, 
                                   &Mesh ) ) )
    {
        // If model is not in current folder, try parent folder
        if( FAILED( D3DXLoadMeshFromX( "..\\PanzerTiger.x", D3DXMESH_SYSTEMMEM, 
                                    d3dDevice, NULL, 
                                    &pD3DXMtrlBuffer, NULL, &NumMaterials, 
                                    &Mesh ) ) )
        {
            MessageBox(NULL, "Could not find PanzerTiger.x", "Meshes.exe", MB_OK);
            return E_FAIL;
        }
    }


    // We need to extract the material properties and texture names from the 
    // pD3DXMtrlBuffer
    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    MeshMaterials = new D3DMATERIAL9[NumMaterials];
    MeshTextures  = new LPDIRECT3DTEXTURE9[NumMaterials];

    for( DWORD i=0; i<NumMaterials; i++ )
    {
        // Copy the material
        MeshMaterials[i] = d3dxMaterials[i].MatD3D;

        // Set the ambient color for the material (D3DX does not do this)
        MeshMaterials[i].Ambient = MeshMaterials[i].Diffuse;

        MeshTextures[i] = NULL;
        if( d3dxMaterials[i].pTextureFilename != NULL && 
            lstrlen(d3dxMaterials[i].pTextureFilename) > 0 )
        {
            // Create the texture
            if( FAILED( D3DXCreateTextureFromFile( d3dDevice, 
                                                d3dxMaterials[i].pTextureFilename, 
                                                &MeshTextures[i] ) ) )
            {
                // If texture is not in current folder, try parent folder
                const TCHAR* strPrefix = TEXT("..\\");
                const int lenPrefix = lstrlen( strPrefix );
                TCHAR strTexture[MAX_PATH];
                lstrcpyn( strTexture, strPrefix, MAX_PATH );
                lstrcpyn( strTexture + lenPrefix, d3dxMaterials[i].pTextureFilename, MAX_PATH - lenPrefix );
                // If texture is not in current folder, try parent folder
                if( FAILED( D3DXCreateTextureFromFile( d3dDevice, 
                                                    strTexture, 
                                                    &MeshTextures[i] ) ) )
                {
                    MessageBox(NULL, "Could not find texture map", "Meshes.exe", MB_OK);
                }
            }
        }
    }

    // Done with the material buffer
    pD3DXMtrlBuffer->Release();

	//Declare object of type VertexBuffer
	LPDIRECT3DVERTEXBUFFER9 VertexBuffer = NULL;
	//The pointer to the current element in the array in VertexBuffer
	BYTE* Vertices = NULL;
	//Get the size of one element in the array of VertexBuffer 
	DWORD FVFVertexSize = D3DXGetFVFVertexSize(Mesh->GetFVF());
	//Get the VertexBuffer
	Mesh->GetVertexBuffer(&VertexBuffer);
	//Get the address of the first element in the array of VertexBuffer
	VertexBuffer->Lock(0,0, (VOID**) &Vertices, D3DLOCK_DISCARD);
	
	/*
		Avem access
	*/
	
	VertexBuffer->Unlock();
	VertexBuffer->Release();

	InitiateCamera();

    CUSTOMVERTEX g_Vertices[] =
    {
        { 0.5f,1.0f, 0.5f,0xffffff00,0.0f,0.0f },
        {  0.0f,0.0f, 0.0f,0xffffff00, 1.0f,1.0f},
        {  1.0f, 0.0f, 0.0f,0xffffff00, 0.0f,1.0f},
        {  1.0f, 0.0f, 1.0f,0xffffff00,1.0f,0.0f},
        {  0.0f, 0.0f, 1.0f,0xffffff00,0.0f,0.0f},
    };


    // Create the vertex buffer.
    if (FAILED(d3dDevice->CreateVertexBuffer(5 * sizeof(CUSTOMVERTEX),
        0, D3DFVF_CUSTOMVERTEX,
        D3DPOOL_DEFAULT, &vrtxBuffer, NULL)))
    {
        return E_FAIL;
    }

    // Fill the vertex buffer.
    VOID* pVertices;
    if (FAILED(vrtxBuffer->Lock(0, sizeof(g_Vertices), (void**)&pVertices, 0)))
        return E_FAIL;
    memcpy(pVertices, g_Vertices, sizeof(g_Vertices));
    vrtxBuffer->Unlock();

    // Circle vertices
    CUSTOMVERTEX circle_Vertices[36];
    for (int i = 0; i < 36; i++)
    {
        float angle = AngleToRadian(i * 10.0f);
        circle_Vertices[i] = { sinf(angle) + 1.0f, cosf(angle), 1.0f, 0x000000ff, ((sinf(angle) + 1.0f) / 4) + 0.5f, ((cosf(angle) + 1.0f) / 4) + 0.5f };
    }
    // Circle vertex buffer
    if (FAILED(d3dDevice->CreateVertexBuffer(36 * sizeof(CUSTOMVERTEX),
        0, D3DFVF_CUSTOMVERTEX,
        D3DPOOL_DEFAULT, &vrtxBufferCircle, NULL)))
    {
        return E_FAIL;
    }
    // Fill circle vertex buffer
    VOID* pVerticesCircle;
    if (FAILED(vrtxBufferCircle->Lock(0, sizeof(circle_Vertices), (void**)&pVerticesCircle, 0)))
        return E_FAIL;
    memcpy(pVerticesCircle, circle_Vertices, sizeof(circle_Vertices));
    vrtxBufferCircle->Unlock();


    return S_OK;


}




//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
VOID Cleanup()
{
    if( MeshMaterials != NULL ) 
        delete[] MeshMaterials;

    if( MeshTextures )
    {
        for( DWORD i = 0; i < NumMaterials; i++ )
        {
            if( MeshTextures[i] )
                MeshTextures[i]->Release();
        }
        delete[] MeshTextures;
    }
    if( Mesh != NULL )
        Mesh->Release();

    if (graphBuilder)
        graphBuilder->Release();

    if (mediaControl)
        mediaControl->Release();

    if (mediaEvent)
        mediaEvent->Release();
    
    if( d3dDevice != NULL )
        d3dDevice->Release();

    if( D3D != NULL )
        D3D->Release();
}


VOID SetupWorldMatrix()
{
	// For our world matrix, we will just leave it as the identity
    D3DXMATRIXA16 matWorld, matWorldTranslation;
    D3DXMatrixTranslation(&matWorldTranslation, -1, 0.0f, 0);
	D3DXMatrixRotationY(&matWorld, timeGetTime() / 1000.0f);
    matWorld = matWorldTranslation * matWorld;
	d3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
}
VOID MoveWorldMatrix()
{
    D3DXMATRIXA16 matWorld, matWorldRot, matWorldTranslation;

    D3DXMatrixTranslation(&matWorldTranslation, dxTigru2, 0.0f, 0);
    D3DXMatrixRotationY(&matWorldRot, timeGetTime() / 1000.0f);

    matWorldRot = matWorldTranslation * matWorldRot;

    d3dDevice->SetTransform(D3DTS_WORLD, &matWorldRot);
}
float rotationAngle = 0.0f; // initial rotation angle
float distance;
int direction = 1;//1 forward, -1 backward
int maximumStepDirection = 300;
int currentStep = 0;
void SetupViewMatrix()
{
	
	//Below is a simulation of mooving forward - backwards. This is done by calling the camera method;
	distance = 0.01 * direction;
	currentStep = currentStep + direction;
	if (currentStep > maximumStepDirection)
	{
		direction = -1;
	}
	if (currentStep < 0)
		direction = 1;
	camera->MoveForward(distance);

	//Each time you render you must call update camera
	//By calling camera update the View Matrix is set;
	camera->Update();
}


void SetupProjectionMatrix()
{
	// For the projection matrix, we set up a perspective transform (which
	// transforms geometry from 3D view space to 2D viewport space, with
	// a perspective divide making objects smaller in the distance). To build
	// a perpsective transform, we need the field of view (1/4 pi is common),
	// the aspect ratio, and the near and far clipping planes (which define at
	// what distances geometry should be no longer be rendered).
	D3DXMATRIXA16 matProj;
D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
d3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

//-----------------------------------------------------------------------------
// Name: SetupMatrices()
// Desc: Sets up the world, view, and projection transform matrices.
//-----------------------------------------------------------------------------
VOID SetupMatrices()
{

	SetupWorldMatrix();

	SetupViewMatrix();
	
	SetupProjectionMatrix();

}
VOID SetupLights()
{
    // Set up a material. The material here just has the diffuse and ambient
    // colors set to yellow. Note that only one material can be used at a time.
    D3DMATERIAL9 mtrl;
    ZeroMemory(&mtrl, sizeof(D3DMATERIAL9));
    mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
    mtrl.Diffuse.g = mtrl.Ambient.g = 5.0f;
    mtrl.Diffuse.b = mtrl.Ambient.b = 10.0f;
    mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
    d3dDevice->SetMaterial(&mtrl);

    // Set up a white, directional light, with an oscillating direction.
    // Note that many lights may be active at a time (but each one slows down
    // the rendering of our scene). However, here we are just using one. Also,
    // we need to set the D3DRS_LIGHTING renderstate to enable lighting
    D3DXVECTOR3 vecDir;
    D3DLIGHT9 light;
    ZeroMemory(&light, sizeof(D3DLIGHT9));
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 1.0f;
    light.Diffuse.b = 1.0f;
    // sa se execute aplicatia cu lumina in pozitie fixa si cu lumina rotindu-se
    vecDir = D3DXVECTOR3(cosf(timeGetTime() / 350.0f),
        1.0f,
        sinf(timeGetTime() / 350.0f));
    //vecDir = D3DXVECTOR3(1, 1.0f, 1 );
    D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDir);
    light.Range = 1000.0f;
    d3dDevice->SetLight(0, &light);
    d3dDevice->LightEnable(0, TRUE);
    d3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

    // Finally, turn on some ambient light.
    d3dDevice->SetRenderState(D3DRS_AMBIENT, 0x00202020);
}

//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
VOID Render()
{
    // Clear the backbuffer and the zbuffer
    d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
        D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

    // Begin the scene
    if (SUCCEEDED(d3dDevice->BeginScene()))
    {
        // Setup the world, view, and projection matrices
        SetupMatrices();
        //SetupLights();

        // Piramid
       d3dDevice->SetStreamSource(0, vrtxBuffer, 0, sizeof(CUSTOMVERTEX));
       d3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
       d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 4);



        // Circle
        d3dDevice->SetStreamSource(0, vrtxBufferCircle, 0, sizeof(CUSTOMVERTEX));
        d3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
        d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 34);

        DrawMesh();

        // Second PanzerTiger
       MoveWorldMatrix();
       DrawMesh();




        // End the scene
        d3dDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    d3dDevice->Present(NULL, NULL, NULL, NULL);
}

void DrawMesh()
{
    // Meshes are divided into subsets, one for each material. Render them in
    // a loop
    for (DWORD i = 0; i < NumMaterials; i++)
    {
        // Set the material and texture for this subset
        d3dDevice->SetMaterial(&MeshMaterials[i]);
        d3dDevice->SetTexture(0, MeshTextures[i]);

        // Draw the mesh subset
        Mesh->DrawSubset(i);
    }
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            Cleanup();
            CleanDInput();
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}


//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
    // Register the window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      "D3D Tutorial", NULL };
    RegisterClassEx(&wc);

    // Create the application's window
    HWND hWnd = CreateWindow("D3D Tutorial", "D3D Tutorial 06: Meshes",
        WS_OVERLAPPEDWINDOW, 100, 100, 300, 300,
        GetDesktopWindow(), NULL, wc.hInstance, NULL);

    //!!!!!! This is very important and must be called prior to any initialization of DirectShow
    HRESULT hr = CoInitialize(NULL);

    // Initialize Direct3D
    if (SUCCEEDED(InitD3D(hWnd)))
    {
        // Init input
        InitDInput(hInst, hWnd);


        // Create the scene geometry
        if (SUCCEEDED(InitGeometry()))
        {
            // Show the window
            ShowWindow(hWnd, SW_SHOWDEFAULT);
            UpdateWindow(hWnd);

            // Enter the message loop
            MSG msg;
            ZeroMemory(&msg, sizeof(msg));
            while (msg.message != WM_QUIT)
            {
                if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                else
                {
                    DetectInput();
                    Render();

                    if (g_Keystate[DIK_ESCAPE] & 0x80)
                    {
                        PostMessage(hWnd, WM_DESTROY, 0, 0);
                    }

                    if (g_Keystate[DIK_P] & 0x80)
                    {
                        if (FAILED(InitDirectShow(hWnd)))
                            return 0;
                        g_Keystate[DIK_P] = 0x00;
                    }

                    if (g_Keystate[DIK_LEFT] & 0x80)
                    {
                        dxTigru2 += 0.3;
                    }
                    if (g_Keystate[DIK_RIGHT] & 0x80)
                    {
                        dxTigru2 -= 0.3;
                    }

                }
            }
        }
    }

    CoUninitialize();

    UnregisterClass("D3D Tutorial", wc.hInstance);
    return 0;
}


