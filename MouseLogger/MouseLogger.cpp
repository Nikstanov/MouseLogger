// MouseLogger.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "MouseLogger.h"
#include "Logger.h"
#include "Properties.h"
#include "basewin.h"
#include "LogsReader.h"

#define MAX_LOADSTRING 100
#define EPSILON 1.0e-5
#define RESOLUTION 32

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

WNDCLASSEXW         MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    FormViewLoggerProperties(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    FormViewDisplayProperties(HWND, UINT, WPARAM, LPARAM);

LogsReader* logsReader;
ULONGLONG startPaintTime;
ULONGLONG realModeStartPaintTime = GetTickCount64();

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class Point2D
{
public:
    float x, y;

    Point2D() { x = y = 0.0; };
    Point2D(float _x, float _y) { x = _x; y = _y; };

    Point2D operator +(const Point2D& point) const { return Point2D(x + point.x, y + point.y); };
    Point2D operator -(const Point2D& point) const { return Point2D(x - point.x, y - point.y); };
    Point2D operator *(double v) const { return Point2D(x * v, y * v); };
    void operator +=(const Point2D& point) { x += point.x; y += point.y; };
    void operator -=(const Point2D& point) { x -= point.x; y -= point.y; };

    void normalize()
    {
        float l = sqrt(x * x + y * y);
        x /= l;
        y /= l;
    }
};

class Segment
{
public:
    Point2D points[4];

    void calc(float t, Point2D& p)
    {
        float t2 = t * t;
        float t3 = t2 * t;
        float nt = 1.0 - t;
        float nt2 = nt * nt;
        float nt3 = nt2 * nt;
        p.x = nt3 * points[0].x + 3.0 * t * nt2 * points[1].x + 3.0 * t2 * nt * points[2].x + t3 * points[3].x;
        p.y = nt3 * points[0].y + 3.0 * t * nt2 * points[1].y + 3.0 * t2 * nt * points[2].y + t3 * points[3].y;
    };
};

bool calculateSpline(std::vector<Point2D>& values)
{
    int n = values.size() - 1;

    if (n < 2)
        return false;

    std::vector<Segment> bezier;
    bezier.resize(n);

    Point2D tgL;
    Point2D tgR;
    Point2D cur;
    Point2D next = values[1] - values[0];
    next.normalize();

    float l1, l2, tmp, x;

    --n;

    for (int i = 0; i < n; ++i)
    {
        bezier[i].points[0] = bezier[i].points[1] = values[i];
        bezier[i].points[2] = bezier[i].points[3] = values[i + 1];

        cur = next;
        next = values[i + 2] - values[i + 1];
        next.normalize();

        tgL = tgR;

        tgR = cur + next;
        tgR.normalize();

        if (abs(values[i + 1].y - values[i].y) < EPSILON)
        {
            l1 = l2 = 0.0;
        }
        else
        {
            tmp = values[i + 1].x - values[i].x;
            l1 = abs(tgL.x) > EPSILON ? tmp / (2.0 * tgL.x) : 1.0;
            l2 = abs(tgR.x) > EPSILON ? tmp / (2.0 * tgR.x) : 1.0;
        }

        if (abs(tgL.x) > EPSILON && abs(tgR.x) > EPSILON)
        {
            tmp = tgL.y / tgL.x - tgR.y / tgR.x;
            if (abs(tmp) > EPSILON)
            {
                x = (values[i + 1].y - tgR.y / tgR.x * values[i + 1].x - values[i].y + tgL.y / tgL.x * values[i].x) / tmp;
                if (x > values[i].x && x < values[i + 1].x)
                {
                    if (tgL.y > 0.0)
                    {
                        if (l1 > l2)
                            l1 = 0.0;
                        else
                            l2 = 0.0;
                    }
                    else
                    {
                        if (l1 < l2)
                            l1 = 0.0;
                        else
                            l2 = 0.0;
                    }
                }
            }
        }

        bezier[i].points[1] += tgL * l1;
        bezier[i].points[2] -= tgR * l2;
    }

    l1 = abs(tgL.x) > EPSILON ? (values[n + 1].x - values[n].x) / (2.0 * tgL.x) : 1.0;

    bezier[n].points[0] = bezier[n].points[1] = values[n];
    bezier[n].points[2] = bezier[n].points[3] = values[n + 1];
    bezier[n].points[1] += tgR * l1;

    values.clear();
    Point2D p;
    for (auto s : bezier)
    {
        for (int i = 0; i < RESOLUTION; ++i)
        {
            s.calc((float)i / (float)RESOLUTION, p);
            values.push_back(Point2D(p.x, p.y));
        }
    }
    return true;
}


class MainWindow : public BaseWindow<MainWindow>
{
    ID2D1Factory*           pFactory;
    ID2D1HwndRenderTarget*  pRenderTarget;
    ID2D1SolidColorBrush*   pBrush;
    ID2D1SolidColorBrush*   pBrushClicks;
    ID2D1SolidColorBrush*   pBrushMoves;
    ID2D1SolidColorBrush*   pBrushZone;
    ID2D1SolidColorBrush*   pBrushZones30;
    ID2D1PathGeometry*      pPathGeometry;
    ID2D1GeometrySink*      pGeometrySink;
    ID2D1Bitmap*            pBitmap;

    D2D1_RECT_F             bitmapSize;
    bool                    isResized = false;
    bool                    isInterpolated = true;

    HRESULT             CreateGraphicsResources();
    void                DiscardGraphicsResources();
    void                OnPaint();
    void                DrawClickFrequencyMatrix();
    void                Resize();
    void                init();
    void                clearResources();

public:

    MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL){}

    PCWSTR  ClassName() const { return L"Main Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

void MainWindow::Resize()
{
    
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
        SafeRelease(&pBitmap);
        pRenderTarget->Resize(size);
        D2D1_BITMAP_PROPERTIES prop;
        pRenderTarget->GetDpi(&prop.dpiX, &prop.dpiY);
        prop.pixelFormat = pRenderTarget->GetPixelFormat();
        pRenderTarget->CreateBitmap(size, prop, &pBitmap);
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    isResized = true;
}

HRESULT MainWindow::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc); 


        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
        
        auto rtProps = D2D1::RenderTargetProperties();
        rtProps.type = D2D1_RENDER_TARGET_TYPE_HARDWARE;
        rtProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

        hr = pFactory->CreateHwndRenderTarget(
            rtProps,
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            if (pBitmap == NULL) {
                D2D1_BITMAP_PROPERTIES prop;
                pRenderTarget->GetDpi(&prop.dpiX, &prop.dpiY);
                prop.pixelFormat = pRenderTarget->GetPixelFormat();
                pRenderTarget->CreateBitmap(size, prop, &pBitmap);
            }
            hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &pBrush);
            hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0, 0), &pBrushClicks);
            hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &pBrushMoves);
            hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0.1f), &pBrushZones30);
            hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.2f), &pBrushZone);
        }
    }
    return hr;
}

void MainWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
    SafeRelease(&pBrushClicks);
    SafeRelease(&pBrushMoves);
    SafeRelease(&pBrushZones30);
    SafeRelease(&pBrushZone);
}

void MainWindow::DrawClickFrequencyMatrix() {
    D2D1_SIZE_F size = pRenderTarget->GetSize();
    float xDiff = size.width / logsReader->getLoggedScreenSize().cx;
    float yDiff = size.height / logsReader->getLoggedScreenSize().cy;

    if (isResized || startPaintTime == 0 || (logsReader == getCurrentLogsReader() && logsReader->isUpdated())) {
        isResized = false;
        int** priorityMatrix = logsReader->getClickFrequencyMatrix();
        for (size_t i = 0; i < logsReader->getLoggedScreenSize().cx; i++)
        {
            for (size_t j = 0; j < logsReader->getLoggedScreenSize().cy; j++)
            {
                if (priorityMatrix[i][j] > 0) {
                    int x = i * xDiff;
                    int y = j * yDiff;
                    D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2(x, y), 10.0f, 10.0f);
                    pBrushZone->SetColor(D2D1::ColorF(0.1f, 1.0f, 1.0f, 0.1f * priorityMatrix[i][j]));
                    pRenderTarget->FillEllipse(ellipse, pBrushZone);
                }
            }
        }
        D2D1_RECT_U newSize = D2D1::RectU(0, 0, size.width, size.height);
        pRenderTarget->Flush();
        HRESULT res = pBitmap->CopyFromRenderTarget(nullptr, pRenderTarget, &newSize);
        bitmapSize = D2D1::RectF(0, 0, size.width, size.width);
    }
    else {
        pRenderTarget->DrawBitmap(pBitmap);
    }
}

void MainWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {

        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        D2D1_SIZE_F size = pRenderTarget->GetSize();

        pRenderTarget->BeginDraw();
        
        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
        
        float xDiff = size.width / logsReader->getLoggedScreenSize().cx;
        float yDiff = size.height / logsReader->getLoggedScreenSize().cy;

        std::vector<Log> logs = logsReader->getLogs();
        
        DrawClickFrequencyMatrix();
        
        pRenderTarget->DrawRectangle(D2D1::RectF(1, 1, size.width - 1, size.height - 1), pBrush);
        pFactory->CreatePathGeometry(&pPathGeometry);
        pPathGeometry->Open(&pGeometrySink);

        if (startPaintTime == 0) {
            startPaintTime = GetTickCount64();
        }

        ULONGLONG timeDiff = GetTickCount64() - startPaintTime;

        std::vector<Point2D> values;

        for (int i = 0; i < logs.size(); i++) {
            POINT newPoint = logs[i].point;
            float x = newPoint.x * xDiff;
            float y = newPoint.y * yDiff;
            
            if ((timeDiff - logs[i].timestamp) > 10000) {
                continue;
            }
            if ((timeDiff - logs[i].timestamp) < 0) {
                break;
            }
            if (logs[i].command == LogsCommands::LDWN) {    
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2(x, y), 3.0f, 3.0f);
                pRenderTarget->FillEllipse(ellipse, pBrushClicks);
            }
            if (logs[i].command == LogsCommands::MV) {
                values.push_back(Point2D(x, y));
            }
        }

        bool startFlag = true;
        D2D1_QUADRATIC_BEZIER_SEGMENT segment;
        D2D1_POINT_2F prev;
        if (isInterpolated) {
            calculateSpline(values);
        }
        for (auto s : values) {
            if (startFlag) {
                prev.x = s.x;
                prev.y = s.y;
                pGeometrySink->BeginFigure(D2D1::Point2(s.x, s.y), D2D1_FIGURE_BEGIN_HOLLOW);
                startFlag = false;
                continue;
            }
            segment.point1 = D2D1::Point2F(prev.x, prev.y);
            segment.point2 = D2D1::Point2F(s.x, s.y);
            pGeometrySink->AddQuadraticBezier(segment);
            prev.x = s.x;
            prev.y = s.y;
        }
        if (!startFlag) {
            pGeometrySink->EndFigure(D2D1_FIGURE_END_OPEN);
        }

        pGeometrySink->Close();
        
        pRenderTarget->DrawGeometry(pPathGeometry, pBrush, 2.0f);

        SafeRelease(&pGeometrySink);
        SafeRelease(&pPathGeometry);
        
        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(m_hwnd, &ps);
    }
}

//
//  FUNCTION: init()
//
//  PURPOSE: Initialize all modules.
//
void MainWindow::init() {
    OpenProperties();
    initLogger();
    logsReader = getCurrentLogsReader();
}

//
//  FUNCTION: clearResources()
//
//  PURPOSE: Close all modules.
//
void MainWindow::clearResources() {
    CloseProperies();
    closeLogger();
}

// WINMAIN - entry point
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MainWindow win;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MOUSELOGGER, szWindowClass, MAX_LOADSTRING);
    if (!win.Create(L"MouseLogger", MyRegisterClass(hInstance), WS_OVERLAPPEDWINDOW))
    {
        return 0;
    }

    ShowWindow(win.Window(), SW_SHOW);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MOUSELOGGER));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            InvalidateRect(win.Window(), NULL, FALSE);
        }
    }

    return (int) msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
WNDCLASSEXW MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = 0;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSELOGGER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MOUSELOGGER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return wcex;
}   

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), Window(), About);
                break;
            case IDM_PROPERTIES:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), Window(), FormViewLoggerProperties);
                break;
            case ID_DISPLAYSETTINGS_PROPERTIES:
                //DialogBox(hInst, MAKEINTRESOURCE(IDD_DISPLAY_DIALOG), Window(), FormViewDisplayProperties);
                break;
            case ID_DISPLAYSETTINGS_REAL:
                if (logsReader != getCurrentLogsReader()) {
                    logsReader->~LogsReader();
                }
                logsReader = getCurrentLogsReader();
                startPaintTime = realModeStartPaintTime;
                break;
            case ID_DISPLAYSETTINGS_INTERPOLATION:
                isInterpolated = !isInterpolated;
                
                if (isInterpolated) {
                    CheckMenuItem(GetMenu(m_hwnd), ID_DISPLAYSETTINGS_INTERPOLATION, MF_CHECKED);
                }
                else {
                    CheckMenuItem(GetMenu(m_hwnd), ID_DISPLAYSETTINGS_INTERPOLATION, MF_UNCHECKED);
                }
                break;
            case ID_DISPLAYSETTINGS_OPENFILE:
            {
                IFileDialog* pFileOpen;
                HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pFileOpen);
                if (SUCCEEDED(hr)) {
                    hr = pFileOpen->Show(NULL);
                    if (SUCCEEDED(hr)) {
                        IShellItem* pItem;
                        hr = pFileOpen->GetResult(&pItem);
                        if (SUCCEEDED(hr)) {
                            PWSTR pszFilePath;
                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                            if (SUCCEEDED(hr)) {
                                if (logsReader != getCurrentLogsReader()) {
                                    logsReader->~LogsReader();
                                }
                                logsReader = new LogsReader(pszFilePath);
                            }
                            pItem->Release();
                        }

                    }
                }
                startPaintTime = 0;
                break;
            }
            case IDM_EXIT:
                DestroyWindow(Window());
                break;
            default:
                return DefWindowProc(Window(), uMsg, wParam, lParam);
            }
        }
        break;
    case WM_CREATE:
        init();
        if (FAILED(D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
        {
            return -1;
        }
        break;

    case WM_DESTROY:
        SafeRelease(&pBitmap);
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        clearResources();
        PostQuitMessage(0);
        break;

    case WM_PAINT:
        OnPaint();
        break;

    case WM_SIZE:
        Resize();
        break;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for properties form.
INT_PTR CALLBACK FormViewLoggerProperties(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {

        SendDlgItemMessageA(hDlg, IDC_SIZE_EDIT, WM_SETTEXT, 0, (LPARAM)std::string{ std::to_string(GetMaxFilesSizeEx()) + " " + GetMaxFilesSizeType()}.c_str());
        SendDlgItemMessageA(hDlg, IDC_DIRECTORY_EDIT, WM_SETTEXT, 0, (LPARAM)getDir().c_str());
        SendDlgItemMessageA(hDlg, IDC_POLLING_EDIT, WM_SETTEXT, 0, (LPARAM)std::to_string(GetFrequence()).c_str());
        SendDlgItemMessageA(hDlg, IDC_FILES_COUNT_EDIT, WM_SETTEXT, 0, (LPARAM)std::to_string(GetFilesCount()).c_str());

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            LPSTR size_str = new char[20];
            GetDlgItemTextA(hDlg, IDC_SIZE_EDIT, size_str, 20);
            try {
                std::string newSize{ size_str };
                std::string num = newSize.substr(0, newSize.find(' '));
                newSize.erase(0, newSize.find(' ') + 1);
                SetMaxFilesSize(std::stol(num));
                SetMaxFilesSizeType((CHAR*)newSize.c_str());
            }
            catch (...) {
                SendDlgItemMessageA(hDlg, IDC_SIZE_EDIT, WM_SETTEXT, 0, (LPARAM)std::string{ "Incorrect Input" }.c_str());
            }

            LPSTR poll_str = new char[7];
            GetDlgItemTextA(hDlg, IDC_POLLING_EDIT, poll_str, 7);
            try {
                SetFrequence(std::stol(poll_str));
            }
            catch (...) {
                SendDlgItemMessageA(hDlg, IDC_POLLING_EDIT, WM_SETTEXT, 0, (LPARAM)std::string{"Incorrect Input"}.c_str());
            }

            LPSTR dir_str = new char[30];
            GetDlgItemTextA(hDlg, IDC_DIRECTORY_EDIT, dir_str, 30);

            LPSTR files_count_str = new char[7];
            GetDlgItemTextA(hDlg, IDC_FILES_COUNT_EDIT, files_count_str, 7);
            try {
                SetFilesCount(std::stol(files_count_str));
            }
            catch (...) {
                SendDlgItemMessageA(hDlg, IDC_FILES_COUNT_EDIT, WM_SETTEXT, 0, (LPARAM)std::string{ "Incorrect Input" }.c_str());
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK FormViewDisplayProperties(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
