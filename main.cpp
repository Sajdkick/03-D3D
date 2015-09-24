//--------------------------------------------------------------------------------------
// BTH - Stefan Petersson 2014.
//--------------------------------------------------------------------------------------
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

HWND InitWindow(HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HRESULT CreateDirect3DContext(HWND wndHandle);

IDXGISwapChain* gSwapChain = nullptr;
ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gDeviceContext = nullptr;
ID3D11RenderTargetView* gBackbufferRTV = nullptr;

ID3D11Buffer* gVertexBuffer = nullptr;

ID3D11InputLayout* gVertexLayout = nullptr;
ID3D11VertexShader* gVertexShader = nullptr;
ID3D11PixelShader* gPixelShader = nullptr;

void CreateShaders()
{
	const char* vertex_shader = R"(
		struct VS_IN
		{
			float3 Pos : POSITION;
			float3 Color : COLOR;
		};

		struct VS_OUT
		{
			float4 Pos : SV_POSITION;
			float3 Color : COLOR;
		};
		//-----------------------------------------------------------------------------------------
		// VertexShader: VSScene
		//-----------------------------------------------------------------------------------------
		VS_OUT VS_main(VS_IN input)
		{
			VS_OUT output = (VS_OUT)0;

			output.Pos = float4(input.Pos, 1);
			output.Color = input.Color;

			return output;
		}
	)";

	const char* pixel_shader = R"(
		struct VS_OUT
		{
			float4 Pos : SV_POSITION;
			float3 Color : COLOR;
		};
	
		float4 PS_main(VS_OUT input) : SV_Target
		{
			return float4(input.Color, 1.0f);
		}
	)";

	//create vertex shader
	ID3DBlob* pVS = nullptr;
	D3DCompile(vertex_shader, strlen(vertex_shader), NULL, nullptr,
		NULL, "VS_main", "vs_4_0", 0, NULL, &pVS, nullptr);

	gDevice->CreateVertexShader(pVS->GetBufferPointer(), pVS->GetBufferSize(), nullptr, &gVertexShader);
	
	//create input layout (verified using vertex shader)
	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	gDevice->CreateInputLayout(inputDesc, ARRAYSIZE(inputDesc), pVS->GetBufferPointer(), pVS->GetBufferSize(), &gVertexLayout);
	pVS->Release();

	//create pixel shader
	ID3DBlob* pPS = nullptr;
	D3DCompile(pixel_shader, strlen(pixel_shader), NULL, nullptr,
		NULL, "PS_main", "ps_4_0", 0, NULL, &pPS, nullptr);

	gDevice->CreatePixelShader(pPS->GetBufferPointer(), pPS->GetBufferSize(), nullptr, &gPixelShader);
	pPS->Release();
}

void CreateTriangleData()
{
	struct TriangleVertex
	{
		float x, y, z;
		float r, g, b;
	}
	triangleVertices[3] =
	{
		0.0f, 0.5f, 0.0f,	//v0 pos
		1.0f, 0.0f, 0.0f,	//v0 color

		0.5f, -0.5f, 0.0f,	//v1
		0.0f, 1.0f, 0.0f,	//v1 color

		-0.5f, -0.5f, 0.0f, //v2
		0.0f, 0.0f, 1.0f	//v2 color
	};

	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(triangleVertices);

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = triangleVertices;
	gDevice->CreateBuffer(&bufferDesc, &data, &gVertexBuffer);
}

void SetViewport()
{
	D3D11_VIEWPORT vp;
	vp.Width = (float)640;
	vp.Height = (float)480;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gDeviceContext->RSSetViewports(1, &vp);
}

void Render()
{
	// clear the back buffer to a deep blue
	float clearColor[] = { 0, 0, 0, 1 };
	gDeviceContext->ClearRenderTargetView(gBackbufferRTV, clearColor);

	gDeviceContext->VSSetShader(gVertexShader, nullptr, 0);
	gDeviceContext->HSSetShader(nullptr, nullptr, 0);
	gDeviceContext->DSSetShader(nullptr, nullptr, 0);
	gDeviceContext->GSSetShader(nullptr, nullptr, 0);
	gDeviceContext->PSSetShader(gPixelShader, nullptr, 0);

	UINT32 vertexSize = sizeof(float) * 6;
	UINT32 offset = 0;
	gDeviceContext->IASetVertexBuffers(0, 1, &gVertexBuffer, &vertexSize, &offset);

	gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gDeviceContext->IASetInputLayout(gVertexLayout);


	gDeviceContext->Draw(3, 0);
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	MSG msg = { 0 };
	HWND wndHandle = InitWindow(hInstance); //1. Skapa fönster
	
	if (wndHandle)
	{
		CreateDirect3DContext(wndHandle); //2. Skapa och koppla SwapChain, Device och Device Context

		SetViewport(); //3. Sätt viewport

		CreateShaders(); //4. Skapa vertex- och pixel-shaders

		CreateTriangleData(); //5. Definiera triangelvertiser, 6. Skapa vertex buffer, 7. Skapa input layout

		ShowWindow(wndHandle, nCmdShow);

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Render(); //8. Rendera

				gSwapChain->Present(0, 0); //9. Växla front- och back-buffer
			}
		}

		gVertexBuffer->Release();

		gVertexLayout->Release();
		gVertexShader->Release();
		gPixelShader->Release();

		gBackbufferRTV->Release();
		gSwapChain->Release();
		gDevice->Release();
		gDeviceContext->Release();
		DestroyWindow(wndHandle);
	}

	return (int) msg.wParam;
}

HWND InitWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.hInstance      = hInstance;
	wcex.lpszClassName = L"BTH_D3D_DEMO";
	if (!RegisterClassEx(&wcex))
		return false;

	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND handle = CreateWindow(
		L"BTH_D3D_DEMO",
		L"BTH Direct3D Demo",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	return handle;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message) 
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;		
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

HRESULT CreateDirect3DContext(HWND wndHandle)
{
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = wndHandle;                           // the window to be used
	scd.SampleDesc.Count = 4;                               // how many multisamples
	scd.Windowed = TRUE;                                    // windowed/full-screen mode

	// create a device, device context and swap chain using the information in the scd struct
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&gSwapChain,
		&gDevice,
		NULL,
		&gDeviceContext);

	if (SUCCEEDED(hr))
	{
		// get the address of the back buffer
		ID3D11Texture2D* pBackBuffer = nullptr;
		gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

		// use the back buffer address to create the render target
		gDevice->CreateRenderTargetView(pBackBuffer, NULL, &gBackbufferRTV);
		pBackBuffer->Release();

		// set the render target as the back buffer
		gDeviceContext->OMSetRenderTargets(1, &gBackbufferRTV, NULL);
	}
	return hr;
}