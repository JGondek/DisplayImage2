#pragma once

const FLOAT DEFAULT_DPI = 96.f;   // Default DPI that maps image resolution directly to screen resoltuion
#define ScreenX (UINT)GetSystemMetrics(SM_CXSCREEN)
#define ScreenY (UINT)GetSystemMetrics(SM_CYSCREEN)

/******************************************************************
*                                                                 *
*  DemoApp                                                        *
*                                                                 *
******************************************************************/

class DemoApp
{
public:
    DemoApp();
    ~DemoApp();

    HRESULT Initialize(HINSTANCE hInstance);

private:
    HRESULT InitializeBitmapFromResourceFile();
    HRESULT CreateD2DBitmapFromWicBitmap(HWND hWnd);
    HRESULT CreateDeviceResources(HWND hWnd);

    LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnPaint(HWND hWnd);

    static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

    HINSTANCE m_hInst;
    IWICImagingFactory* m_pIWICFactory;

    ID2D1Factory* m_pD2DFactory;
    ID2D1HwndRenderTarget* m_pRT;
    ID2D1Bitmap* m_pD2DBitmap;
    IWICFormatConverter* m_pConvertedSourceBitmap;

    UINT uiWidth; // set using the resource file dimensions
    UINT uiHeight;

    bool mousedown = false;
    POINT lastLocation;
};


