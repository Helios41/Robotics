#if 0

#include "Definitions.h"
#include <stdio.h>
#include "Dashboard.h"

const char WindowClassName[] = "WindowClass";
WNDCLASSEX WindowClass = {0};

HWND WindowHandle = {0};

typedef struct 
{
   u32 width;
   u32 height;
   u32 *memory; //0x AA RR GG BB
}dbBitmap;

typedef struct
{
   dbBitmap bmp;
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

LRESULT CALLBACK WindowMessageEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

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
   bitmap->bmp.memory = (u32 *)VirtualAlloc(0, memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
   
   Win32Printf(hstdout, "W: %u H: %u\n", width, height);
}

s32 RoundR32ToS32(r32 real)
{
   s32 result = (s32)(real + 0.5f);
   return result;
}

void dbDrawBitmap(dbBitmap *dest, r32 rminx, r32 rminy, r32 rmaxx, r32 rmaxy, dbBitmap *src)
{
   s32 minx = dbRoundR32ToS32(rminx);
   s32 miny = dbRoundR32ToS32(rminy);
   s32 maxx = dbRoundR32ToS32(rmaxx);
   s32 maxy = dbRoundR32ToS32(rmaxy);
   
   if(minx < 0)
   {
      minx = 0;
   }
   
   if(miny < 0)
   {
      miny = 0;
   }
   
   if(maxx > dest->width)
   {
      maxx = dest->width;
   }
   
   if(maxy > dest->height)
   {
      maxy = dest->height;
   }
   
   {
      s32 new_miny = dest->height - miny;
      s32 new_maxy = dest->height - maxy;
      
      maxy = new_miny;
      miny = new_maxy;
   }
   
   u32 *row = (dest->memory + minx + (miny * dest->width)); 
   u32 *src_row = (src->memory + minx + (miny * src->width)); 
   
   for(u32 y = miny; y < maxy; ++y)
   {
      u32 *pixel = row;
      u32 *src_pixel = src_row;
      
      for(u32 x = minx; x < maxx; ++x)
      {
         u32 color32 = *pixel;
         r32 oldr = ((r32)((color32 & 0x00FF0000) >> 16)) / 255.0f;
         r32 oldg = ((r32)((color32 & 0x0000FF00) >> 8)) / 255.0f;
         r32 oldb = ((r32)((color32 & 0x000000FF) >> 0)) / 255.0f;
         
         u32 src_color = *src_pixel;
         r32 srca = 1.0; //((r32)((src_color & 0xFF000000) >> 24)) / 255.0f; //TODO: alpha
         r32 srcr = ((r32)((src_color & 0x00FF0000) >> 16)) / 255.0f;
         r32 srcg = ((r32)((src_color & 0x0000FF00) >> 8)) / 255.0f;
         r32 srcb = ((r32)((src_color & 0x000000FF) >> 0)) / 255.0f;
         
         u8 rcolor = (u8)((srca * srcr + (1 - srca) * oldr) * 255);
         u8 gcolor = (u8)((srca * srcg + (1 - srca) * oldg) * 255);
         u8 bcolor = (u8)((srca * srcb + (1 - srca) * oldb) * 255);
         
         u32 color = ((rcolor << 16) | (gcolor << 8) | (bcolor << 0));
         *pixel++ = color;
         src_pixel++;
      }
      
      row += dest->width;
      src_row += src->width;
   }
}

void dbFillRectRaw(dbBitmap *bitmap, r32 rminx, r32 rminy, r32 rmaxx, r32 rmaxy, v4 color)
{
   s32 minx = dbRoundR32ToS32(rminx);
   s32 miny = dbRoundR32ToS32(rminy);
   s32 maxx = dbRoundR32ToS32(rmaxx);
   s32 maxy = dbRoundR32ToS32(rmaxy);
   
   if(minx < 0)
   {
      minx = 0;
   }
   
   if(miny < 0)
   {
      miny = 0;
   }
   
   if(maxx > bitmap->width)
   {
      maxx = bitmap->width;
   }
   
   if(maxy > bitmap->height)
   {
      maxy = bitmap->height;
   }
   
   {
      s32 new_miny = bitmap->height - miny;
      s32 new_maxy = bitmap->height - maxy;
      
      maxy = new_miny;
      miny = new_maxy;
   }
   
   u32 *row = (bitmap->memory + minx + (miny * bitmap->width)); 
   
   for(u32 y = miny; y < maxy; ++y)
   {
      u32 *pixel = row;
      
      for(u32 x = minx; x < maxx; ++x)
      {
         u32 color32 = *pixel;
         r32 oldr = ((r32)((color32 & 0x00FF0000) >> 16)) / 255.0f;
         r32 oldg = ((r32)((color32 & 0x0000FF00) >> 8)) / 255.0f;
         r32 oldb = ((r32)((color32 & 0x000000FF) >> 0)) / 255.0f;
         
         u8 rcolor = (u8)((color.r.a * color.r.r + (1 - color.r.a) * oldr) * 255);
         u8 gcolor = (u8)((color.r.a * color.r.g + (1 - color.r.a) * oldg) * 255);
         u8 bcolor = (u8)((color.r.a * color.r.b + (1 - color.r.a) * oldb) * 255);
         
         u32 color = ((rcolor << 16) | (gcolor << 8) | (bcolor << 0));
         *pixel++ = color;
      }
      
      row += bitmap->width;
   }
}

void dbFillRect(dbBitmap *bitmap, r32 x, r32 y, r32 width, r32 height, v4 color)
{
   dbFillRectRaw(bitmap, x, y, width + x, height + y, color);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
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
   
   //Setup background backbuffer
   Win32Bitmap background_backbuffer = {0};
   Win32SetBitmapSize(&background_backbuffer, client_rect.right, client_rect.bottom);
   
   dbFillRect(&background_backbuffer.bmp, 0, 0, background_backbuffer.bmp.width, background_backbuffer.bmp.height, V4(1.0f, 1.0f, 1.0f, 1.0f));
      
   dbFillRect(&background_backbuffer.bmp, 0, 0, background_backbuffer.bmp.width, background_backbuffer.bmp.height / 4, V4(1.0f, 0.0f, 0.0f, 1.0f));
   dbFillRect(&background_backbuffer.bmp, 0, background_backbuffer.bmp.height / 4, background_backbuffer.bmp.width, background_backbuffer.bmp.height / 16, V4(0.0f, 0.0f, 0.0f, 1.0f));
   
   dbFillRect(&background_backbuffer.bmp, 0, 300, background_backbuffer.bmp.width, background_backbuffer.bmp.height / 8, V4(1.0f, 0.0f, 0.0f, 1.0f));
   dbFillRect(&background_backbuffer.bmp, 0, 300 + (background_backbuffer.bmp.height / 8), background_backbuffer.bmp.width, background_backbuffer.bmp.height / 32, V4(0.0f, 0.0f, 0.0f, 1.0f));
   
   //Allocate real backbuffer
   Win32Bitmap backbuffer = {0};
   Win32SetBitmapSize(&backbuffer, client_rect.right, client_rect.bottom);
   
   ShowWindow(WindowHandle, nCmdShow);
	UpdateWindow(WindowHandle);
   Running = true;
 
   MSG msg = {0};
   
   s32 x = backbuffer.bmp.width / 2;
   s32 y = backbuffer.bmp.height / 2;
   
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
                     y--;
                     break;
                     
                  case VK_DOWN:
                     y++;
                     break;
                     
                  case VK_LEFT:
                     x--;
                     break;
                     
                  case VK_RIGHT:
                     x++;
                     break;
                     
                  case VK_RETURN:
                     
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
      
      //blit background
      dbDrawBitmap(&backbuffer.bmp, 0, 0, backbuffer.bmp.width, backbuffer.bmp.height, &background_backbuffer.bmp);
      
      //Random test square
      dbFillRect(&backbuffer.bmp, x, y, 100, 100, V4(0.0f, 1.0f, 0.0f, 0.5f));
      
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
         
         FillRect(paint_context, &paint.rcPaint, CreateSolidBrush(RGB(255, 0, 255)));
         //paint
         
         EndPaint(window, &paint);
		}
		break;
      
      case WM_SIZE:
      {
         RECT window_size;
         GetClientRect(WindowHandle, &window_size);
         
         u32 width = window_size.right - window_size.left;
         u32 height = window_size.bottom - window_size.top;
         
         //resize bitmap
      }
      break;
	}
		
	return DefWindowProc(window, message, wParam, lParam);;
}

#endif