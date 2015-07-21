#include "Definitions.h"
#include <windows.h>

const char WindowClassName[] = "WindowClass";
WNDCLASSEX WindowClass = {0};
HWND WindowHandle = {0};
HDC DeviceContext = {0};
HDC DeviceMemory = {0};

b32 Running = false;

LRESULT CALLBACK WindowMessageEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

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
 
   DeviceContext = GetDC(WindowHandle);
   DeviceMemory = CreateCompatibleDC(DeviceContext);
   
   
   
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
      
      //TODO: create & render to DIB
      BitBlt(DeviceContext, 10, 10, 450, 400, DeviceMemory, 0, 0, SRCCOPY);
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
         PAINTSTRUCT paint_info;
         HDC GDI_context = BeginPaint(window, &paint_info);
         FillRect(GDI_context, &paint_info.rcPaint, CreateSolidBrush(RGB(255, 255, 255)));
         EndPaint(window, &paint_info);
		}
		break;
	}
		
	return DefWindowProc(window, message, wParam, lParam);;
}