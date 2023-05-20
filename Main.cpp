#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include <dshow.h>   
#include <dinput.h>
#include "Camera.h"

//We define an event id 
#define WM_GRAPHNOTIFY  WM_APP + 1

//Make sure the libraries d3d9.lib and d3dx9.lib are linked
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

LPDIRECT3D9             g_pD3D = NULL; // Used to create the D3DDevice   
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device  

LPD3DXMESH              Mesh = NULL; // Our mesh object in sysmem
D3DMATERIAL9* MeshMaterials = NULL; // Materials for our mesh
LPDIRECT3DTEXTURE9* MeshTextures = NULL; // Textures for our mesh
DWORD                   NumMaterials = 0L;   // Number of mesh materials
CXCamera* camera; 

IGraphBuilder* graphBuilder = NULL;
//Help us to start/stop the play
IMediaControl* mediaControl = NULL;
//We receie events in case something happened - during playing, stoping, errors etc..
IMediaEventEx* mediaEvent = NULL;
//We can use to fast forward, revert etc..
IMediaSeeking* mediaSeeking = NULL;

HWND hWnd;
HDC hdc;

LPDIRECTINPUT8			g_pDin;							// the pointer to our DirectInput interface
LPDIRECTINPUTDEVICE8	g_pDinKeyboard;					// the pointer to the keyboard device

LPDIRECT3DSURFACE9		g_pSurface = NULL;			// Our surface on which we load the image
LPDIRECT3DSURFACE9		g_pBackBuffer = NULL;			// The back buffer

LPCTSTR					g_Path = "crosshair.jpg"; // The path to the image
D3DXIMAGE_INFO			g_Info;							// Here we will keep the information read from the image file

POINT					g_SurfacePosition;				// The position of the surface
BYTE					g_Keystate[256];				// the storage for the key-information

LPDIRECTINPUTDEVICE8	g_pDinmouse;					// the pointer to the mouse device
DIMOUSESTATE			g_pMousestate;					// the storage for the mouse-information

HRESULT InitD3D(HWND hWnd);
HRESULT InitDInput(HINSTANCE hInstance, HWND hWnd);

VOID DetectInput();
VOID Render();
VOID Cleanup();
VOID CleanDInput();

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT);

struct CUSTOMVERTEX
{
    FLOAT x, y, z; //Position
    FLOAT tu, tv;   // The texture coordinates
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

CUSTOMVERTEX SkyBox[24] = {
    //Fata lateral stanga
    { -1.0f, -1.0f, -1.0f, 1, 0 }, //rosu
    { -1.0f, 1.0f, -1.0f, 1, 1 }, //galben
    { -1.0f, -1.0f, 1.0f, 0, 0 }, //albastru
    { -1.0f, 1.0f, 1.0f, 0, 1 },  //verde

    //Fata din fata
    { 1.0f, -1.0f, 1.0f, 0, 0 }, //rosu
    { 1.0f, 1.0f, 1.0f, 0, 1 }, //galben
    { -1.0f, -1.0f, 1.0f, 1, 0 }, //albastru
    { -1.0f, 1.0f, 1.0f, 1, 1 },  //verde

    //Fata laterala dreapta
    { 1.0f, -1.0f, 1.0f, 1, 0 }, //rosu
    { 1.0f, 1.0f, 1.0f, 1, 1 }, //galben
    { 1.0f, -1.0f, -1.0f, 0, 0 }, //albastru
    { -1.0f, 1.0f, 1.0f, 0, 1 },  //verde

    //Fata din spate
    { 1.0f, -1.0f, -1.0f, 1, 0 }, //rosu
    { 1.0f, 1.0f, -1.0f, 1, 1 }, //galben
    { -1.0f, -1.0f, -1.0f, 0, 0 }, //albastru
    { -1.0f, 1.0f, -1.0f, 0, 1 },  //verde

    //Fata sus
    { -1.0f, 1.0f, -1.0f, 1, 0 }, //rosu
    { 1.0f, 1.0f, -1.0f, 1, 1 }, //galben
    { -1.0f, 1.0f, 1.0f, 0, 0 }, //albastru
    { 1.0f, 1.0f, 1.0f, 0, 1 },  //verde

    //Fata jos
    { 1.0f, -1.0f, -1.0f, 1, 0 }, //rosu
    { -1.0f, -1.0f, -1.0f, 1, 1 }, //galben
    { 1.0f, -1.0f, 1.0f, 0, 0 }, //albastru
    { -1.0f, -1.0f, 1.0f, 0, 1 }  //verde
};

//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D(HWND hWnd)
{
    // Create the D3D object.
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
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
    if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp, &g_pd3dDevice)))
    {
        if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &d3dpp, &g_pd3dDevice)))
            return E_FAIL;
    }

    // Read the information from the image
    if (D3DXGetImageInfoFromFile(g_Path, &g_Info) == D3D_OK) {

        // Create a surface using the information read from the image
        g_pd3dDevice->CreateOffscreenPlainSurface(g_Info.Width, g_Info.Height, g_Info.Format, D3DPOOL_SYSTEMMEM, &g_pSurface, NULL);

        // Get the backbuffer so we can copy the contents of our surface
        g_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &g_pBackBuffer);

        // Load the image in our surface
        if (D3DXLoadSurfaceFromFile(g_pSurface, NULL, NULL, g_Path, NULL, D3DX_DEFAULT, 0, NULL) != D3D_OK) {

            //If the image could not be loaded show an error message and exit
            MessageBox(NULL, "An exception occured while loading image info from file", "Error", 0);
            return E_FAIL;

        }
    }
    else {

        // If the information could not be read from the file show an error message and exit
        MessageBox(NULL, "An exception occured while loading image info from file", "Error", 0);
        return E_FAIL;

    }

    // Turn on the zbuffer`
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

    // Turn on ambient lighting 
    g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, 0xffffffff);


    return S_OK;
}

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
    hr = graphBuilder->RenderFile(L"tiger.avi", NULL);

    //Set window for events  - basically we tell our event in case you raise an event use the following event id.
    mediaEvent->SetNotifyWindow((OAHWND)hWnd, WM_GRAPHNOTIFY, 0);

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

    // create the mouse device
    g_pDin->CreateDevice(GUID_SysMouse,
        &g_pDinmouse,  // the pointer to the device interface
        NULL); // COM stuff, so we'll set it to NULL

    // set the data format to keyboard format
    g_pDinKeyboard->SetDataFormat(&c_dfDIKeyboard);

    // set the data format to mouse format
    g_pDinmouse->SetDataFormat(&c_dfDIMouse);

    // set the control we will have over the keyboard
    g_pDinKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

    // set the control we will have over the mouse
    g_pDinmouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);

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
    g_pDinmouse->Acquire();

    // get the input data
    g_pDinKeyboard->GetDeviceState(256, (LPVOID)g_Keystate);
    g_pDinmouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&g_pMousestate);


}

//-----------------------------------------------------------------------------
// Name: CleanDInput(void)
// Desc: This is the function that closes DirectInput
//-----------------------------------------------------------------------------
VOID CleanDInput()
{
    g_pDinKeyboard->Unacquire();    // make sure the keyboard is unacquired
    g_pDinmouse->Unacquire();    // make sure the mouse in unacquired
    g_pDin->Release();    // close DirectInput before exiting
}


void InitiateCamera()
{
    camera = new CXCamera(g_pd3dDevice);

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

//-----------------------------------------------------------------------------
// Name: InitGeometry()
// Desc: Load the mesh and build the material and texture arrays
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
    LPD3DXBUFFER pD3DXMtrlBuffer;

    // Load the mesh from the specified file
    if (FAILED(D3DXLoadMeshFromX("PanzerTiger.x", D3DXMESH_SYSTEMMEM,
        g_pd3dDevice, NULL,
        &pD3DXMtrlBuffer, NULL, &NumMaterials,
        &Mesh)))
    {
        // If model is not in current folder, try parent folder
        if (FAILED(D3DXLoadMeshFromX("..\\PanzerTiger.x", D3DXMESH_SYSTEMMEM,
            g_pd3dDevice, NULL,
            &pD3DXMtrlBuffer, NULL, &NumMaterials,
            &Mesh)))
        {
            MessageBox(NULL, "Could not find PanzerTiger.x", "Meshes.exe", MB_OK);
            return E_FAIL;
        }
    }

    // We need to extract the material properties and texture names from the 
    // pD3DXMtrlBuffer
    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    MeshMaterials = new D3DMATERIAL9[NumMaterials];
    MeshTextures = new LPDIRECT3DTEXTURE9[NumMaterials];

    for (DWORD i = 0; i < NumMaterials; i++)
    {
        // Copy the material
        MeshMaterials[i] = d3dxMaterials[i].MatD3D;

        // Set the ambient color for the material (D3DX does not do this)
        MeshMaterials[i].Ambient = MeshMaterials[i].Diffuse;

        MeshTextures[i] = NULL;
        if (d3dxMaterials[i].pTextureFilename != NULL &&
            lstrlen(d3dxMaterials[i].pTextureFilename) > 0)
        {
            // Create the texture
            if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice,
                d3dxMaterials[i].pTextureFilename,
                &MeshTextures[i])))
            {
                // If texture is not in current folder, try parent folder
                const TCHAR* strPrefix = TEXT("..\\");
                const int lenPrefix = lstrlen(strPrefix);
                TCHAR strTexture[MAX_PATH];
                lstrcpyn(strTexture, strPrefix, MAX_PATH);
                lstrcpyn(strTexture + lenPrefix, d3dxMaterials[i].pTextureFilename, MAX_PATH - lenPrefix);
                // If texture is not in current folder, try parent folder
                if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice,
                    strTexture,
                    &MeshTextures[i])))
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
    VertexBuffer->Lock(0, 0, (VOID**)&Vertices, D3DLOCK_DISCARD);

    /*
        Avem access
    */

    VertexBuffer->Unlock();
    VertexBuffer->Release();

    InitiateCamera();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
VOID Cleanup()
{
    if (MeshMaterials != NULL)
        delete[] MeshMaterials;

    if (MeshTextures)
    {
        for (DWORD i = 0; i < NumMaterials; i++)
        {
            if (MeshTextures[i])
                MeshTextures[i]->Release();
        }
        delete[] MeshTextures;
    }
    if (Mesh != NULL)
        Mesh->Release();

    if (g_pd3dDevice != NULL)
        g_pd3dDevice->Release();

    if (g_pD3D != NULL)
        g_pD3D->Release();

    if (graphBuilder)
        graphBuilder->Release();

    if (mediaControl)
        mediaControl->Release();

    if (mediaEvent)
        mediaEvent->Release();
}


VOID SetupWorldMatrix()
{
    // For our world matrix, we will just leave it as the identity
    D3DXMATRIXA16 matWorld;
    D3DXMatrixRotationY(&matWorld, timeGetTime() / 1000.0f);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
}

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
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
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

//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
VOID Render()
{
    // Clear the backbuffer and the zbuffer
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
        D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

    // Begin the scene
    if (SUCCEEDED(g_pd3dDevice->BeginScene()))
    {
        // Setup the world, view, and projection matrices
        SetupMatrices();

        //Copy the contents of the surface into the backbuffer
        g_pd3dDevice->UpdateSurface(g_pSurface, NULL, g_pBackBuffer, &g_SurfacePosition);

        // Meshes are divided into subsets, one for each material. Render them in
        // a loop
        for (DWORD i = 0; i < NumMaterials; i++)
        {
            // Set the material and texture for this subset
            g_pd3dDevice->SetMaterial(&MeshMaterials[i]);
            g_pd3dDevice->SetTexture(0, MeshTextures[i]);

            // Draw the mesh subset
            Mesh->DrawSubset(i);
        }

        // End the scene
        g_pd3dDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_DESTROY:
            Cleanup();
            CleanDInput();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
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
    HWND hWnd = CreateWindow("D3D Tutorial", "Bucur Dan-Alexandru: Proiect la SM",
        WS_OVERLAPPEDWINDOW, 100, 100, 800, 800,
        GetDesktopWindow(), NULL, wc.hInstance, NULL);

    //!!!!!! This is very important and must be called prior to any initialization of DirectShow
    HRESULT hr = CoInitialize(NULL);

    hdc = GetDC(hWnd);

    // Initialize Direct3D
    if (SUCCEEDED(InitD3D(hWnd)))
    {
        InitDInput(hInst, hWnd);

        // Create the scene geometry
        if (SUCCEEDED(InitGeometry()))
        {
            //Init video
            if (FAILED(InitDirectShow(hWnd)))
                return 0;

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
                    DetectInput();    // update the input data before rendering
                    Render();

                    g_SurfacePosition.x += g_pMousestate.lX;
                    g_SurfacePosition.y += g_pMousestate.lY;

                    if (g_Keystate[DIK_ESCAPE] & 0x80) {
                        PostMessage(hWnd, WM_DESTROY, 0, 0);
                    }

                    if (g_Keystate[DIK_P] & 0x80) {
                        //Play media control
                        mediaControl->Run();
                    }
                }
            }
        }
    }

    CoUninitialize();

    UnregisterClass("D3D Tutorial", wc.hInstance);
    return 0;
}