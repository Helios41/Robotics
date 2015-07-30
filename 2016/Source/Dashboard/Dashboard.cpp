#include "Definitions.h"
#include <windows.h>

const char WindowClassName[] = "WindowClass";
WNDCLASSEX WindowClass = {0};

HWND WindowHandle = {0};
HDC DeviceContext = {0};

HBITMAP BitmapHandle = {0};
u8 *BitmapMemory = NULL;
BITMAPINFO BitmapInfo = {0};

b32 Running = false;

LRESULT CALLBACK WindowMessageEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void CreateDIB(u32 width, u32 height)
{
   if(BitmapHandle)
   {
      DeleteObject(BitmapHandle);
   }
   
   BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
   BitmapInfo.bmiHeader.biWidth = width;
   BitmapInfo.bmiHeader.biHeight = height;
   BitmapInfo.bmiHeader.biPlanes = 1;
   BitmapInfo.bmiHeader.biBitCount = 32;
   BitmapInfo.bmiHeader.biCompression = BI_RGB;
   
   BitmapHandle = CreateDIBSection(DeviceContext,
                                   &BitmapInfo,
                                   DIB_RGB_COLORS,
                                   &BitmapMemory,
                                   0, 0);
}

void RenderDIB(HDC paintContext, u32 x, u32 y, u32 width, u32 height)
{
   StretchDIBits(paintContext,
                 x, y, width, height,
                 x, y, width, height,
                 BitmapMemory,
                 &BitmapInfo,
                 DIB_RGB_COLORS,
                 SRCCOPY);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   WindowClass.cbsize = sizeof(WNDCLASSEX);
   WindowClass.lpfnWndProc = WindowMessageEvent;
   WindowClass.cbsize = hInstance;
   WindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   WindowClass.hCursor = LoadCursor(NULL, IDC_CROSS);
   WindowClass.lpszClassName = WindowClassName;
   
   RegisterClassEx(&WindowClass);
   
   u32 screen_width = GetSystemMetrics(SM_CXSCREEN);
   u32 screen_height = GetSystemMetrics(SM_CYSCREEN);
   
   u32 window_width = screen_width - 20;
   u32 window_hieght = screen_height / 2;
   
   WindowHandle = CreateWindowExA(WS_EX_CLIENTEDGE,
                                  WindowClassName,
                                  "Dashboard V2",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  window_width,
                                  window_height,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  NULL);
 
   DeviceContext = CreateCompatibleDC(0);
   
   CreateDIB(window_width, window_height);
   
   ShowWindow(WindowHandle, nCmdShow);
	UpdateWindow(WindowHandle);
   Running = true;
 
   MSG msg = {0};
 
   while(Running)
   {
      while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
         switch(msg.message)
         {
            case WM_KEYDOWN:
            {
               
            }
            break;
            
            case WM_CHAR:
            {
               
            }
            break;
            
            case WM_LBUTTONDOWN:
            {
               
            }
            break;
            
            case WM_QUIT:
            {
               Running = false;
            }
            break;
         }
         
         TranslateMessage(&msg);
			DispatchMessage(&msg);
      }
   }
 
   return 0;
}

LRESULT CALLBACK WindowMessageEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{	
	switch(message)
	{
		case WM_CLOSE:
		{
			DestroyWindow(window);
		}
		break;
		
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;
		
		case WM_PAINT:
		{
         PAINTSTRUCT paint;
         HDC paint_context = BeginPaint(window, &paint);
         
         u32 x = paint.rcPaint.left;
         u32 y = paint.rcPaint.top;
         u32 width = paint.rcPaint.right - paint.rcPaint.left;
         u32 height = paint.rcPaint.bottom - paint.rcPaint.top;
         
         RenderDIB(paint_context, x, y, width, height);
         
         EndPaint(window, &paint_info);
		}
		break;
      
      case WM_SIZE:
      {
         RECT window_size;
         GetClientRect(WindowHandle, &window_size);
         
         u32 width = window_size.right - window_size.left;
         u32 height = window_size.bottom - window_size.top;
         
         CreateDIB(width, height);
      }
      break;
	}
		
	return DefWindowProc(window, message, wParam, lParam);;
}