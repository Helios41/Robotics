#include "Definitions.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

const char WindowClassName[] = "WindowClass";
WNDCLASSEX WindowClass = {0};

HWND WindowHandle = {0};

typedef struct 
{
   u32 width;
   u32 height;
   u32 *memory;
}Bitmap;

typedef struct
{
   Bitmap bmp;
   BITMAPINFO info;
}Win32Bitmap;

typedef union
{
   struct
   {
      r32 r;
      r32 g;
      r32 b;
      r32 a;
   }r;
   
   struct
   {
      r32 x;
      r32 y;
      r32 z;
      r32 w;
   }p;
   
   r32 v[4];
}v4;

inline v4 V4(r32 x, r32 y, r32 z, r32 w)
{
   v4 result = {0};
   
   result.p.x = x;
   result.p.y = y;
   result.p.z = z;
   result.p.w = w;
   
   return result;
}

b32 Running = false;
HANDLE hstdout;

LRESULT CALLBACK WindowMessageEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

Win32Printf(HANDLE stream, const char* fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   
   char buffer[1024];
   
   vsprintf(buffer, fmt, args);
   WriteFile(stream, buffer, strlen(buffer), 0, 0);
   
   va_end(args);
}

//TODO: aligned bitmap memory (aligned to DWord)
void Win32SetBitmapSize(Win32Bitmap *bitmap, u32 width, u32 height)
{
   if(bitmap->bmp.memory)
   {
      VirtualFree(bitmap->bmp.memory, 0, MEM_RELEASE);
   }
   
   bitmap->info.bmiHeader.biSize = sizeof(bitmap->info.bmiHeader);
   bitmap->info.bmiHeader.biWidth = width;
   bitmap->info.bmiHeader.biHeight = height;
   bitmap->info.bmiHeader.biPlanes = 1;
   bitmap->info.bmiHeader.biBitCount = 32;
   bitmap->info.bmiHeader.biCompression = BI_RGB;
   
   bitmap->bmp.width = width;
   bitmap->bmp.height = height;
   
   u32 memory_size = (width * height * sizeof(u32));
   bitmap->bmp.memory = VirtualAlloc(0, memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
   
   Win32Printf(hstdout, "W: %u H: %u\n", width, height);
}

void Win32BlitBitmap(HDC renderContext, u32 x, u32 y, u32 width, u32 height, Win32Bitmap *bitmap)
{
   //TODO: rewrite this
   StretchDIBits(renderContext,
                 x, y, bitmap->bmp.width, bitmap->bmp.height,
                 0, 0, bitmap->bmp.width, bitmap->bmp.height,
                 bitmap->bmp.memory,
                 &bitmap->info,
                 DIB_RGB_COLORS,
                 SRCCOPY);
}

void BitmapDrawRect(Bitmap *bitmap, u32 inx, u32 iny, u32 width, u32 height, v4 color)
{
   //TODO: fix this
   
   //TODO: clamp values
   
   u32 *bitmap_mem = bitmap->memory;
   
   for(u32 y = iny; y < (iny + height); ++y)
   {
      u32 *row = bitmap_mem;
      
      for(u32 x = inx; x < (inx + width); ++x)
      {
         u32 color32 = *row;
         r32 oldr = ((r32)((color32 & 0x00FF0000) >> 16)) / 255.0f;
         r32 oldg = ((r32)((color32 & 0x0000FF00) >> 8)) / 255.0f;
         r32 oldb = ((r32)((color32 & 0x000000FF) >> 0)) / 255.0f;
         
         u8 rcolor = (u8)((color.r.a * color.r.r + (1 - color.r.a) * oldr) * 255);
         u8 gcolor = (u8)((color.r.a * color.r.g + (1 - color.r.a) * oldg) * 255);
         u8 bcolor = (u8)((color.r.a * color.r.b + (1 - color.r.a) * oldb) * 255);
         
         u32 color = ((rcolor << 16) | (gcolor << 8) | (bcolor << 0));
         *row++ = color;
      }
      
      bitmap_mem += bitmap->width;
   }
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   AllocConsole();
   SetConsoleTitleA("Dashboard V2 Console");
   hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
   
   Win32Printf(hstdout, "Starting...\n");
   
   WindowClass.cbSize = sizeof(WNDCLASSEX);
   WindowClass.style = CS_OWNDC;
   WindowClass.lpfnWndProc = WindowMessageEvent;
   WindowClass.hInstance = hInstance;
   WindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   WindowClass.hCursor = LoadCursor(NULL, IDC_CROSS);
   WindowClass.lpszClassName = WindowClassName;
   
   RegisterClassEx(&WindowClass);
   
   WindowHandle = CreateWindowExA(WS_EX_CLIENTEDGE,
                                  WindowClassName,
                                  "Dashboard V2",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  NULL);
   
   HDC DeviceContext = GetDC(WindowHandle);
   
   RECT client_rect = {0};
   GetClientRect(WindowHandle, &client_rect);
   
   Win32Bitmap backbuffer = {0};
   Win32SetBitmapSize(&backbuffer, client_rect.right / 2, client_rect.bottom / 2);
   
   ShowWindow(WindowHandle, nCmdShow);
	UpdateWindow(WindowHandle);
   Running = true;
 
   MSG msg = {0};
   
   u32 x = 0;
   u32 y = 0;
   
   while(Running)
   {
      while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
         switch(msg.message)
         {
            case WM_KEYDOWN:
            {
               switch(msg.wParam)
               {
                  case VK_UP:
                     x++;
                     break;
                     
                  case VK_DOWN:
                     x--;
                     break;
                     
                  case VK_LEFT:
                     y++;
                     break;
                     
                  case VK_RIGHT:
                     y--;
                     break;
               }
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
      
      //Clear
      BitmapDrawRect(&backbuffer.bmp, 0, 0, backbuffer.bmp.width, backbuffer.bmp.height, V4(1.0f, 0.0f, 0.0f, 1.0f));
      
      BitmapDrawRect(&backbuffer.bmp, x, y, backbuffer.bmp.width / 2, backbuffer.bmp.height / 2, V4(0.0f, 1.0f, 0.0f, 1.0f));
      BitmapDrawRect(&backbuffer.bmp, 0, 0, backbuffer.bmp.width / 2, backbuffer.bmp.height / 2, V4(0.0f, 0.0f, 1.0f, 0.5f));
      
      RECT client_rect = {0};
      GetClientRect(WindowHandle, &client_rect);
      Win32BlitBitmap(DeviceContext, 0, 0, client_rect.right, client_rect.bottom, &backbuffer);
      
      Sleep(15);
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
         
         FillRect(paint_context, &paint.rcPaint, CreateSolidBrush(RGB(0, 0, 0)));
         
         EndPaint(window, &paint);
		}
		break;
      
      case WM_SIZE:
      {
         RECT window_size;
         GetClientRect(WindowHandle, &window_size);
         
         u32 width = window_size.right - window_size.left;
         u32 height = window_size.bottom - window_size.top;
      }
      break;
	}
		
	return DefWindowProc(window, message, wParam, lParam);;
}