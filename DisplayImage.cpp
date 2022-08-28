#include "framework.h"
#include "DisplayImage.h"


template <typename T>
inline void SafeRelease(T*& p)
{
	if (NULL != p)
	{
		p->Release();
		p = NULL;
	}
}

/******************************************************************
*  Application entrypoint                                         *
******************************************************************/

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR pszCmdLine,
	int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(pszCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(hr))
	{
		{
			DemoApp app;
			hr = app.Initialize(hInstance);

			if (SUCCEEDED(hr))
			{
				BOOL fRet;
				MSG msg;

				// Main message loop:
				while ((fRet = GetMessage(&msg, NULL, 0, 0)) != 0)
				{
					if (fRet == -1)
					{
						break;
					}
					else
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
		}
		CoUninitialize();
	}

	return 0;
}

/******************************************************************
*  Initialize member data                                         *
******************************************************************/

DemoApp::DemoApp()
	:
	m_pD2DBitmap(NULL),
	m_pConvertedSourceBitmap(NULL),
	m_pIWICFactory(NULL),
	m_pD2DFactory(NULL),
	m_pRT(NULL),
	uiWidth(NULL),
	uiHeight(NULL)
{
}

/******************************************************************
*  Tear down resources                                            *
******************************************************************/

DemoApp::~DemoApp()
{
	SafeRelease(m_pD2DBitmap);
	SafeRelease(m_pConvertedSourceBitmap);
	SafeRelease(m_pIWICFactory);
	SafeRelease(m_pD2DFactory);
	SafeRelease(m_pRT);
}

/******************************************************************
*  Create application window and resources                        *
******************************************************************/

HRESULT DemoApp::Initialize(HINSTANCE hInstance)
{
	HRESULT hr = S_OK;

	// Create WIC factory
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_pIWICFactory)
	);

	if (SUCCEEDED(hr))
	{
		// Create D2D factory
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	}

	if (SUCCEEDED(hr))
	{
		WNDCLASSEX wcex;

		// Register window class
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = DemoApp::s_WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = hInstance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = L"DisplayImage";
		wcex.hIconSm = NULL;

		m_hInst = hInstance;

		hr = RegisterClassEx(&wcex) ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		hr = InitializeBitmapFromResourceFile();
	}

	HWND hWnd = NULL;
	if (SUCCEEDED(hr))
	{
		// Create window
		hWnd = CreateWindowEx(
			0,
			L"DisplayImage",
			L"DisplayImage",
			WS_OVERLAPPEDWINDOW,
			(ScreenX - uiWidth) / 2,//CW_USEDEFAULT,
			(ScreenY - uiHeight) / 2,//CW_USEDEFAULT,
			uiWidth,
			uiHeight,
			NULL,
			NULL,
			hInstance,
			this
		);

		ShowWindow(hWnd, true);
		UpdateWindow(hWnd);

		//SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP); //3d argument=style
		//SetWindowPos(hWnd, HWND_TOPMOST, (ScreenX - uiWidth) / 2, (ScreenY - uiHeight) / 2, uiWidth, uiHeight, SWP_SHOWWINDOW);

		hr = hWnd ? S_OK : E_FAIL;
	}
	
	if (SUCCEEDED(hr))
	{
		hr = CreateD2DBitmapFromWicBitmap(hWnd);
	}


	return hr;
}


/******************************************************************
*  Init WIC bitmap from the resource image file                   *
******************************************************************/
HRESULT DemoApp::InitializeBitmapFromResourceFile() {

	HRESULT hr = S_OK;


	// Step 2: Decode the source image
	// ======= preload the image to get it's size ============

	hr = S_OK;

	// WIC interface pointers.
	IWICStream* pIWICStream = NULL;
	IWICBitmapDecoder* pIDecoder = NULL;

	// Resource management.
	HRSRC imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void* pImageFile = NULL;
	DWORD imageFileSize = 0;

	// Locate the resource in the application's executable.
	imageResHandle = FindResource(
		NULL,             // This component.
		L"IDB_IMAGE1",         // Resource name.
		L"IMAGE");        // Resource type.

	hr = (imageResHandle ? S_OK : E_FAIL);

	// Load the resource to the HGLOBAL.
	if (SUCCEEDED(hr)) {
		imageResDataHandle = LoadResource(NULL, imageResHandle);
		hr = (imageResDataHandle ? S_OK : E_FAIL);
	}

	// Lock the resource to retrieve memory pointer.
	if (SUCCEEDED(hr)) {
		pImageFile = LockResource(imageResDataHandle);
		hr = (pImageFile ? S_OK : E_FAIL);
	}

	// Calculate the size.
	if (SUCCEEDED(hr)) {
		imageFileSize = SizeofResource(NULL, imageResHandle);
		hr = (imageFileSize ? S_OK : E_FAIL);
	}

	// Create a WIC stream to map onto the memory.
	if (SUCCEEDED(hr)) {
		hr = m_pIWICFactory->CreateStream(&pIWICStream);
	}

	// Initialize the stream with the memory pointer and size.
	if (SUCCEEDED(hr)) {
		hr = pIWICStream->InitializeFromMemory(
			reinterpret_cast<BYTE*>(pImageFile),
			imageFileSize);
	}

	// Create a decoder for the stream.
	if (SUCCEEDED(hr)) {
		hr = m_pIWICFactory->CreateDecoderFromStream(
			pIWICStream,                   // The stream to use to create the decoder
			NULL,                          // Do not prefer a particular vendor
			WICDecodeMetadataCacheOnLoad,  // Cache metadata when needed
			&pIDecoder);                   // Pointer to the decoder
	}


	// Retrieve the initial frame.
	IWICBitmapFrameDecode* pFrame = NULL;
	if (SUCCEEDED(hr)) {
		hr = pIDecoder->GetFrame(0, &pFrame);
	}

	
	if (SUCCEEDED(hr)) {
		hr = pFrame->GetSize(&uiWidth, &uiHeight);
	}

	if (uiWidth > ScreenX)
		uiWidth = ScreenX;

	if (uiHeight > ScreenY)
		uiHeight = ScreenY;

	// Format convert the frame to 32bppPBGRA
	if (SUCCEEDED(hr))
	{
		SafeRelease(m_pConvertedSourceBitmap);
		hr = m_pIWICFactory->CreateFormatConverter(&m_pConvertedSourceBitmap);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pConvertedSourceBitmap->Initialize(
			pFrame,                          // Input bitmap to convert
			GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
			WICBitmapDitherTypeNone,         // Specified dither pattern
			NULL,                            // Specify a particular palette 
			0.f,                             // Alpha threshold
			WICBitmapPaletteTypeCustom       // Palette translation type
		);
	}

	SafeRelease(pIDecoder);
	SafeRelease(pFrame);

	return hr;
}

/******************************************************************
*  Create an D2DBitmap from the WIC bitmap                        *
******************************************************************/

HRESULT DemoApp::CreateD2DBitmapFromWicBitmap(HWND hWnd)
{
	HRESULT hr = S_OK;

	//Step 4: Create render target and D2D bitmap from IWICBitmapSource
	if (SUCCEEDED(hr))
	{
		hr = CreateDeviceResources(hWnd);
	}

	if (SUCCEEDED(hr))
	{
		// Need to release the previous D2DBitmap if there is one
		SafeRelease(m_pD2DBitmap);
		hr = m_pRT->CreateBitmapFromWicBitmap(m_pConvertedSourceBitmap, NULL, &m_pD2DBitmap);
	}


	return hr;
}



/******************************************************************
*  This method creates resources which are bound to a particular  *
*  D2D device. It's all centralized here, in case the resources   *
*  need to be recreated in the event of D2D device loss           *
* (e.g. display change, remoting, removal of video card, etc).    *
******************************************************************/

HRESULT DemoApp::CreateDeviceResources(HWND hWnd)
{
	HRESULT hr = S_OK;
	
	if (!m_pRT)
	{
		RECT rc;
		hr = GetClientRect(hWnd, &rc) ? S_OK : E_FAIL;

		if (SUCCEEDED(hr))
		{
			// Create a D2D render target properties
			D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties();

			// Set the DPI to be the default system DPI to allow direct mapping
			// between image pixels and desktop pixels in different system DPI settings
			renderTargetProperties.dpiX = DEFAULT_DPI;
			renderTargetProperties.dpiY = DEFAULT_DPI;
			renderTargetProperties.type = D2D1_RENDER_TARGET_TYPE_SOFTWARE;
			//renderTargetProperties.pixelFormat = { };
			//renderTargetProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_STRAIGHT;
			//renderTargetProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
			// Create a D2D render target
			D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

			hr = m_pD2DFactory->CreateHwndRenderTarget(
				renderTargetProperties,
				D2D1::HwndRenderTargetProperties(hWnd, size),
				&m_pRT
			);
		}
	}

	return hr;
}

/******************************************************************
*  Registered Window message handler                              *
******************************************************************/

LRESULT CALLBACK DemoApp::s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DemoApp* pThis;
	LRESULT lRet = 0;

	if (uMsg == WM_NCCREATE)
	{
		LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT> (lParam);
		pThis = reinterpret_cast<DemoApp*> (pcs->lpCreateParams);

		SetWindowLongPtr(hWnd, GWLP_USERDATA, PtrToUlong(pThis));
		lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	else
	{
		pThis = reinterpret_cast<DemoApp*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (pThis)
		{
			lRet = pThis->WndProc(hWnd, uMsg, wParam, lParam);
		}
		else
		{
			lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}
	return lRet;
}

/******************************************************************
*  Internal Window message handler                                *
******************************************************************/

LRESULT DemoApp::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		return OnPaint(hWnd);
	}
	case WM_LBUTTONDOWN: {

		PostQuitMessage(0);
		return 0; // uncomment for dragging support

		mousedown = true;
		GetCursorPos(&lastLocation);
		RECT rect;
		GetWindowRect(hWnd, &rect);
		lastLocation.x = lastLocation.x - rect.left;
		lastLocation.y = lastLocation.y - rect.top;
		break;
	}
	case WM_LBUTTONUP: {
		mousedown = false;
		break;
	}
	case WM_MOUSEMOVE: {
		if (mousedown) {
			POINT currentpos;
			GetCursorPos(&currentpos);
			int x = currentpos.x - lastLocation.x;
			int y = currentpos.y - lastLocation.y;
			MoveWindow(hWnd, x, y, uiWidth, uiHeight, false);
		}
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

/******************************************************************
* Rendering routine using D2D                                     *
******************************************************************/
LRESULT DemoApp::OnPaint(HWND hWnd)
{
	
	HRESULT hr = S_OK;
	PAINTSTRUCT ps;
	
	if (BeginPaint(hWnd, &ps))
	{
		// Create render target if not yet created
		hr = CreateDeviceResources(hWnd);

		if (SUCCEEDED(hr) && !(m_pRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
		{
			m_pRT->BeginDraw();

			m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());

			// Clear the background
			m_pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));

			//D2D1_SIZE_F rtSize = m_pRT->GetSize();

			// Create a rectangle same size of current window
			D2D1_RECT_F rectangle = D2D1::RectF(0.0f, 0.0f, (FLOAT)uiWidth, (FLOAT)uiHeight/*rtSize.width, rtSize.height*/);

			// D2DBitmap may have been released due to device loss. 
			// If so, re-create it from the source bitmap
			if (m_pConvertedSourceBitmap && !m_pD2DBitmap)
			{
				m_pRT->CreateBitmapFromWicBitmap(m_pConvertedSourceBitmap, NULL, &m_pD2DBitmap);
			}

			// Draws an image and scales it to the current window size
			if (m_pD2DBitmap)
			{
				m_pRT->DrawBitmap(m_pD2DBitmap, rectangle);
			}

			hr = m_pRT->EndDraw();

			// In case of device loss, discard D2D render target and D2DBitmap
			// They will be re-create in the next rendering pass
			if (hr == D2DERR_RECREATE_TARGET)
			{
				SafeRelease(m_pD2DBitmap);
				SafeRelease(m_pRT);
				// Force a re-render
				hr = InvalidateRect(hWnd, NULL, TRUE) ? S_OK : E_FAIL;
			}
		}

		EndPaint(hWnd, &ps);
	}

	return SUCCEEDED(hr) ? 0 : 1;
}