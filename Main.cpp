
#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include "Camera.h"



//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
LPDIRECT3D9             D3D = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       d3dDevice = NULL; // Our rendering device

LPD3DXMESH              Mesh = NULL; // Our mesh object in sysmem
D3DMATERIAL9* MeshMaterials = NULL; // Materials for our mesh
LPDIRECT3DTEXTURE9* MeshTextures = NULL; // Textures for our mesh
DWORD                   NumMaterials = 0L;   // Number of mesh materials
CXCamera* camera;


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


    return S_OK;
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

//-----------------------------------------------------------------------------
// Name: InitGeometry()
// Desc: Load the mesh and build the material and texture arrays
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
    LPD3DXBUFFER pD3DXMtrlBuffer;

    // Load the mesh from the specified file
    if (FAILED(D3DXLoadMeshFromX("PanzerTiger.x", D3DXMESH_SYSTEMMEM,
        d3dDevice, NULL,
        &pD3DXMtrlBuffer, NULL, &NumMaterials,
        &Mesh)))
    {
        // If model is not in current folder, try parent folder
        if (FAILED(D3DXLoadMeshFromX("..\\PanzerTiger.x", D3DXMESH_SYSTEMMEM,
            d3dDevice, NULL,
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
            if (FAILED(D3DXCreateTextureFromFile(d3dDevice,
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
                if (FAILED(D3DXCreateTextureFromFile(d3dDevice,
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

    if (d3dDevice != NULL)
        d3dDevice->Release();

    if (D3D != NULL)
        D3D->Release();
}


VOID SetupWorldMatrix()
{
    // For our world matrix, we will just leave it as the identity
    D3DXMATRIXA16 matWorld;
    D3DXMatrixRotationY(&matWorld, timeGetTime() / 1000.0f);
    d3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
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

        // End the scene
        d3dDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    d3dDevice->Present(NULL, NULL, NULL, NULL);
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
    HWND hWnd = CreateWindow("D3D Tutorial", "D3D Tutorial 06: Meshes",
        WS_OVERLAPPEDWINDOW, 100, 100, 300, 300,
        GetDesktopWindow(), NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (SUCCEEDED(InitD3D(hWnd)))
    {
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
                    Render();
            }
        }
    }

    UnregisterClass("D3D Tutorial", wc.hInstance);
    return 0;
}



