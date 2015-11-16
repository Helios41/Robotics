#include <windows.h>
#include <Shobjidl.h>
#include "Definitions.h"
#include "Dashboard.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

LRESULT CALLBACK WindowMessageEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{	
	switch(message)
	{
		case WM_CLOSE:
			DestroyWindow(window);
         break;
		
		case WM_DESTROY:
			PostQuitMessage(0);
         break;
		
		case WM_PAINT:
         break;
      
      case WM_SIZE:
         break;
	}
		
	return DefWindowProc(window, message, wParam, lParam);;
}

EntireFile LoadEntireFile(const char* path)
{
   HANDLE file_handle = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, NULL);
                                    
   EntireFile result = {};
   
   result.length = GetFileSize(file_handle, NULL);
   result.contents = VirtualAlloc(0, result.length, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
   ReadFile(file_handle, result.contents, result.length, NULL, NULL);
   
   return result;
}

#pragma pack(push, 1)
struct BMPHeader
{
   u16 file_type;
   u32 file_size;
   u16 reserved1;
   u16 reserved2;
   u32 bitmap_offset;
   u32 size;
   s32 width;
   s32 height;
   u16 planes;
   u16 bits_per_pixel;
};
#pragma pack(pop)

LoadedBitmap LoadBitmapFromBMP(const char* path)
{
   LoadedBitmap result = {};
   EntireFile bitmap_file = LoadEntireFile(path);
   
   if(bitmap_file.length != 0)
   {
      BMPHeader *header = (BMPHeader *)bitmap_file.contents;
      result.pixels = (u32 *)((u8 *)bitmap_file.contents + header->bitmap_offset);
      result.width = header->width;
      result.height = header->height;
      
      u32 *pixels = result.pixels;
      for(u32 y = 0; y < result.height; ++y)
      {
         for(u32 x = 0; x < result.width; ++x)
         {
            *pixels = (*pixels >> 8) | (*pixels << 24);
            pixels++;
         }
      }
   }
   
   return result;
}

LoadedFont GetCharBitmap(stbtt_fontinfo *font, char c, u32 height_scale)
{
   LoadedFont result = {};
   
   int width, height, xoffset, yoffset;
   u8 *mono_bitmap = stbtt_GetCodepointBitmap(font, 0, stbtt_ScaleForPixelHeight(font, height_scale),
                                              c, &width, &height, &xoffset, &yoffset);
   
   result.bitmap.width = (u32)width;
   result.bitmap.height = (u32)height;
   result.offset.x = (r32)xoffset;
   result.offset.y = (r32)yoffset;
   
   
   result.bitmap.pixels = (u32 *)VirtualAlloc(0, (result.bitmap.width * result.bitmap.height * sizeof(u32)), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
   
   u8 *source_row = mono_bitmap + width*height;
   u32 *dest_row = result.bitmap.pixels;
   
   for(u32 y = 0; y < result.bitmap.height; ++y)
   {
      u8 *source = source_row;
      u32 *dest = dest_row + width;
      
      for(u32 x = 0; x < result.bitmap.width; ++x)
      {
         *dest = (*source << 24) |
                 (*source << 16) |
                 (*source << 8) |
                 (*source << 0);
         
         source--;
         dest--;
      }
      
      source_row -= width;
      dest_row += width;
   }
   
   stbtt_FreeBitmap(mono_bitmap, 0);
   
   return result;
}

void AllocateBitmap(LoadedBitmap *bitmap, u32 width, u32 height)
{
   if(bitmap->pixels)
   {
      VirtualFree(bitmap->pixels, 0, MEM_RELEASE);
   }
   
   bitmap->width = width;
   bitmap->height = height;
   bitmap->pixels = (u32 *)VirtualAlloc(0, (width * height * sizeof(u32)), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

void AllocateBitmapFromWindow(LoadedBitmap *bitmap, HWND window)
{
   RECT client_rect = {};
   GetClientRect(window, &client_rect);
   AllocateBitmap(bitmap, client_rect.right, client_rect.bottom);
}

void BlitToScreen(LoadedBitmap *bitmap, HDC device_context)
{
   BITMAPINFO info = {};
   
   info.bmiHeader.biSize = sizeof(info.bmiHeader);
   info.bmiHeader.biWidth = bitmap->width;
   info.bmiHeader.biHeight = bitmap->height;
   info.bmiHeader.biPlanes = 1;
   info.bmiHeader.biBitCount = 32;
   info.bmiHeader.biCompression = BI_RGB;
   
   StretchDIBits(device_context,
                 0, 0, bitmap->width, bitmap->height,
                 0, 0, bitmap->width, bitmap->height,
                 bitmap->pixels,
                 &info,
                 DIB_RGB_COLORS,
                 SRCCOPY);
}

void DrawRectangle(LoadedBitmap *dest, r32 rx, r32 ry, r32 rwidth, r32 rheight, v4 color)
{
   s32 minx = RoundR32ToS32(rx);
   s32 miny = RoundR32ToS32(ry);
   s32 maxx = RoundR32ToS32(rx + rwidth);
   s32 maxy = RoundR32ToS32(ry + rheight);
   
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
   
   u32 *row = (dest->pixels + minx + (miny * dest->width)); 
   
   for(u32 y = miny; y < maxy; ++y)
   {
      u32 *pixel = row;
      
      for(u32 x = minx; x < maxx; ++x)
      {
         u32 color32 = *pixel;
         r32 oldr = ((r32)((color32 & 0x00FF0000) >> 16)) / 255.0f;
         r32 oldg = ((r32)((color32 & 0x0000FF00) >> 8)) / 255.0f;
         r32 oldb = ((r32)((color32 & 0x000000FF) >> 0)) / 255.0f;
         
         u8 rcolor = (u8)((color.a * color.r + (1 - color.a) * oldr) * 255);
         u8 gcolor = (u8)((color.a * color.g + (1 - color.a) * oldg) * 255);
         u8 bcolor = (u8)((color.a * color.b + (1 - color.a) * oldb) * 255);
         
         u32 color = ((rcolor << 16) | (gcolor << 8) | (bcolor << 0));
         *pixel++ = color;
      }
      
      row += dest->width;
   }
}

void DrawBitmap(LoadedBitmap *dest, LoadedBitmap *src, r32 rx, r32 ry)
{   
   s32 minx = RoundR32ToS32(rx);
   s32 miny = RoundR32ToS32(ry);
   s32 maxx = RoundR32ToS32(rx) + src->width;
   s32 maxy = RoundR32ToS32(ry) + src->height;
   
   u32 u = 0;
   u32 v = 0;
   
   if(minx < 0)
   {
      u = -minx;
      minx = 0;
   }
   
   if(miny < 0)
   {
      //TODO: handle v
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
   
   u32 *dest_row = (dest->pixels + minx + (miny * dest->width)); 
   u32 *src_row = src->pixels + src->width*v; 
   
   for(u32 y = miny; y < maxy; ++y)
   {
      u32 *dest_pixel = dest_row;
      u32 *src_pixel = src_row + u;
      
      for(u32 x = minx; x < maxx; ++x)
      {
         u32 color32 = *dest_pixel;
         r32 oldr = ((r32)((color32 & 0x00FF0000) >> 16)) / 255.0f;
         r32 oldg = ((r32)((color32 & 0x0000FF00) >> 8)) / 255.0f;
         r32 oldb = ((r32)((color32 & 0x000000FF) >> 0)) / 255.0f;
         
         u32 src_color = *src_pixel++;
         
         r32 srca = ((r32)((src_color & 0xFF000000) >> 24)) / 255.0f;
         r32 srcr = ((r32)((src_color & 0x00FF0000) >> 16)) / 255.0f;
         r32 srcg = ((r32)((src_color & 0x0000FF00) >> 8)) / 255.0f;
         r32 srcb = ((r32)((src_color & 0x000000FF) >> 0)) / 255.0f;
         
         u8 rcolor = (u8)((srca * srcr + (1 - srca) * oldr) * 255);
         u8 gcolor = (u8)((srca * srcg + (1 - srca) * oldg) * 255);
         u8 bcolor = (u8)((srca * srcb + (1 - srca) * oldb) * 255);
         
         u32 color = ((rcolor << 16) | (gcolor << 8) | (bcolor << 0));
         
         *dest_pixel++ = color;
      }
      
      dest_row += dest->width;
      src_row += src->width;
   }
}

void DrawText(LoadedBitmap *dest, stbtt_fontinfo *font, v2 pos, char *text, u32 scale)
{
   u32 xoffset = 0;
   while(*text)
   {
      if(*text == ' ')
      {
         xoffset += 10;
      }
      else
      {
         LoadedFont char_bitmap = GetCharBitmap(font, *text, scale);
         DrawBitmap(dest, &char_bitmap.bitmap, pos.x + xoffset + char_bitmap.offset.x, pos.y + char_bitmap.offset.y + 10);
         xoffset += char_bitmap.bitmap.width;
      }
      text++;
   }
}

b32 GUIButton(LoadedBitmap *dest, v2 mouse, rect2 bounds, b32 left_mouse, LoadedBitmap *icon)
{
   b32 selected = Contains(bounds, mouse);
   
   if(selected && left_mouse)
   {
      DrawRectangle(dest, bounds.min.x, bounds.min.y, bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(selected)
   {
      DrawRectangle(dest, bounds.min.x, bounds.min.y, bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
      DrawRectangle(dest, bounds.min.x, bounds.min.y, bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   DrawBitmap(dest, icon, bounds.min.x, bounds.min.y);
   
   return selected && left_mouse;
}

b32 GUIButton(LoadedBitmap *dest, v2 mouse, rect2 bounds, b32 left_mouse, char *text, stbtt_fontinfo *font)
{
   b32 selected = Contains(bounds, mouse);
   
   if(selected && left_mouse)
   {
      DrawRectangle(dest, bounds.min.x, bounds.min.y, bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(selected)
   {
      DrawRectangle(dest, bounds.min.x, bounds.min.y, bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
      DrawRectangle(dest, bounds.min.x, bounds.min.y, bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   DrawText(dest, font, V2(bounds.min.x + 10, bounds.min.y + 10), text, 20);
   
   return selected && left_mouse;
}

v2 GetCursorPosition(HWND window)
{
   v2 result = {};
   
   POINT p;
   GetCursorPos(&p);
   ScreenToClient(window, &p);
   
   result.x = p.x;
   result.y = p.y;
   
   return result;
}

void DrawAutoBuilderBlock(LoadedBitmap *dest, u32 selected_block_id, v2 pos, v4 color, stbtt_fontinfo *font)
{
   if(selected_block_id > 0)
      DrawRectangle(dest, pos.x, pos.y, 100, 20, color);

   switch(selected_block_id)
   {
      case 0:
         break;
         
      case 1:
         DrawText(dest, font, V2(pos.x, pos.y), "Test1", 20);
         break;
         
      case 2:
         DrawText(dest, font, V2(pos.x, pos.y), "Test2", 20);
         break;
         
      case 3:
         DrawText(dest, font, V2(pos.x, pos.y), "Test3", 20);
         break;
         
      case 4:
         DrawText(dest, font, V2(pos.x, pos.y), "Test4", 20);
         break;
         
      case 5:
         DrawText(dest, font, V2(pos.x, pos.y), "Test5", 20);
         break;
   }
}

b32 AutoBuilderButton(LoadedBitmap *dest, u32 selected_block_id, v2 pos, v2 mouse, v4 color, b32 left_mouse, stbtt_fontinfo *font)
{
   b32 result = Contains(RectPosSize(pos.x, pos.y, 100, 20), mouse) && left_mouse;
   
   if(result)
      DrawRectangle(dest, pos.x - 5, pos.y - 5, 110, 30, V4(0.0f, 0.0f, 0.0f, 1.0f));
   
   DrawAutoBuilderBlock(dest, selected_block_id, pos, color, font);
   return result;
}

HDC SetupWindow(HINSTANCE hInstance, int nCmdShow, HWND *window, LoadedBitmap *backbuffer)
{
   *window = CreateWindowExA(WS_EX_CLIENTEDGE, "WindowClass", "Dashboard V2",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
                             hInstance, NULL);
   
   HANDLE hIcon = LoadImageA(0, "icon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
   if(hIcon)
   {
       SendMessageA(*window, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
       SendMessageA(*window, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

       SendMessageA(GetWindow(*window, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
       SendMessageA(GetWindow(*window, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM) hIcon);
   }
   else
   {
      MessageBox(*window, "Error", "Icon Not Found", 0);
   }
   
   AllocateBitmapFromWindow(backbuffer, *window);
   ShowWindow(*window, nCmdShow);
	UpdateWindow(*window);
   
   return GetDC(*window);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   WNDCLASSEX window_class = {};
   
   window_class.cbSize = sizeof(window_class);
   window_class.style = CS_OWNDC;
   window_class.lpfnWndProc = WindowMessageEvent;
   window_class.hInstance = hInstance;
   window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   window_class.hCursor = LoadCursor(NULL, IDC_CROSS);
   window_class.lpszClassName = "WindowClass";
   
   RegisterClassExA(&window_class);
   
   HWND window = {};
   LoadedBitmap backbuffer = {};
   HDC device_context = SetupWindow(hInstance, nCmdShow, &window, &backbuffer);
   
   EntireFile font_file = LoadEntireFile("font.ttf");
   stbtt_fontinfo font;
   stbtt_InitFont(&font, (u8 *)font_file.contents, stbtt_GetFontOffsetForIndex((u8 *)font_file.contents, 0));
   
   LoadedBitmap logo = LoadBitmapFromBMP("logo.bmp");
   LoadedBitmap home = LoadBitmapFromBMP("home.bmp");
   LoadedBitmap gear = LoadBitmapFromBMP("gear.bmp");
   LoadedBitmap dinosaur = LoadBitmapFromBMP("dinosaur.bmp");
   LoadedBitmap eraser = LoadBitmapFromBMP("eraser.bmp");
   
   b32 running = true;
   b32 left_mouse = false;
   b32 up_arrow = false;
   b32 down_arrow = false;
   b32 left_arrow = false;
   b32 right_arrow = false;
   b32 connected = false;
   PageType page = PageType_Home;
   v2 mouse = V2(0, 0);
   MSG msg = {};
   b32 auto_block_eraser = false;
   
   u32 selected_block_id = 0;
   u32 auto_blocks[100];
   u32 auto_block_count = 0;
   v4 auto_block_colors[5] = 
   {
      {1.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 1.0f, 0.0f, 1.0f},
      {0.0f, 0.0f, 1.0f, 1.0f},
      {1.0f, 1.0f, 0.0f, 1.0f},
      {0.0f, 1.0f, 1.0f, 1.0f}
   };
   char *auto_file_name = "unnamed";
   
   v2 player_pos = V2(0.0f, 0.0f);
   
   while(running)
   {
      while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
      {
         switch(msg.message)
         {            
            case WM_LBUTTONUP:
               left_mouse = false;
               break;
         
            case WM_LBUTTONDOWN:
               left_mouse = true;
               break;
               
            case WM_KEYDOWN:
            {
               switch(msg.wParam)
               {
                  case VK_LEFT:
                     left_arrow = true;
                     break;
                     
                  case VK_RIGHT:
                     right_arrow = true;
                     break;
                     
                  case VK_UP:
                     up_arrow = true;
                     break;
                     
                  case VK_DOWN:
                     down_arrow = true;
                     break;
               }
            }
            break;
            
            case WM_KEYUP:
            {
               switch(msg.wParam)
               {
                  case VK_LEFT:
                     left_arrow = false;
                     break;
                     
                  case VK_RIGHT:
                     right_arrow = false;
                     break;
                     
                  case VK_UP:
                     up_arrow = false;
                     break;
                     
                  case VK_DOWN:
                     down_arrow = false;
                     break;
               }
            }
            break;
            
            case WM_QUIT:
               running = false;
               break;
         }
         
         TranslateMessage(&msg);
			DispatchMessageA(&msg);
      }
      
      mouse = GetCursorPosition(window);
      
      DrawRectangle(&backbuffer, 0, 0, backbuffer.width, backbuffer.height, V4(1.0f, 1.0f, 1.0f, 1.0f));
      DrawBitmap(&backbuffer, &logo, (backbuffer.width / 2) - (logo.width / 2), (backbuffer.height / 2) - (logo.height / 2));
      
      DrawRectangle(&backbuffer, 0, 0, backbuffer.width, 15, V4(1.0f, 0.0f, 0.0f, 1.0f));
      
      if(connected)
      {
         DrawRectangle(&backbuffer, 10, 3, 10, 10, V4(0.0f, 1.0f, 0.0f, 1.0f));
      }
      else
      {
         DrawRectangle(&backbuffer, 10, 3, 10, 10, V4(0.0f, 0.0f, 0.0f, 1.0f));
      }
      
      if(GUIButton(&backbuffer, mouse, RectPosSize(10, 20, 40, 40), left_mouse, &home))
      {
         page = PageType_Home;
      }
      
      if(GUIButton(&backbuffer, mouse, RectPosSize(60, 20, 40, 40), left_mouse, &gear))
      {
         page = PageType_Config;
      }
      
      if(GUIButton(&backbuffer, mouse, RectPosSize(110, 20, 15, 40), left_mouse, "", &font))
      {
         running = false;
      }
      
      if(GUIButton(&backbuffer, mouse, RectPosSize(10, 70, 120, 40), left_mouse, "Autonomous Builder", &font))
      {
         page = PageType_Auto;
      }
      
      if(GUIButton(&backbuffer, mouse, RectPosSize(10, 120, 120, 40), left_mouse, "Fun", &font))
      {
         page = PageType_Fun;
      }
      
      if(page == PageType_Home)
      {
         DrawRectangle(&backbuffer, 10, 20, 5, 5, V4(0.0f, 0.0f, 0.0f, 1.0f));
         
         DrawRectangle(&backbuffer, 280, 20, 800, 85, V4(0.5f, 0.0f, 0.0f, 0.5f));
         DrawText(&backbuffer, &font, V2(600, 40), "CN Robotics", 40);
      }
      else if(page == PageType_Config)
      {
         DrawRectangle(&backbuffer, 60, 20, 5, 5, V4(0.0f, 0.0f, 0.0f, 1.0f));
      }
      else if(page == PageType_Auto)
      {
         DrawRectangle(&backbuffer, 10, 70, 5, 40, V4(0.0f, 0.0f, 0.0f, 1.0f));
         b32 button_clicked = false;
         
         rect2 sandbox_bounds = RectPosSize(280, 20, 800, 675);
         DrawRectangle(&backbuffer, 280, 20, 800, 675, V4(0.5f, 0.0f, 0.0f, 0.5f));
         DrawText(&backbuffer, &font, V2(600, 40), auto_file_name, 25);
         
         if(GUIButton(&backbuffer, mouse, RectPosSize(160, 20, 40, 40), left_mouse, &eraser))
         {
            auto_block_eraser = !auto_block_eraser;
         }
         
         if(auto_block_eraser)
         {
            DrawRectangle(&backbuffer, 160, 20, 5, 5, V4(0.0f, 0.0f, 0.0f, 1.0f));
         }
         
         for(u32 i = 1; i <= 5; ++i)
         {
            b32 hit = AutoBuilderButton(&backbuffer, i, V2(160, 70 + ((i - 1) * 25)), 
                                        mouse, auto_block_colors[i - 1], left_mouse, &font);
            
            if(hit)
            {
               selected_block_id = i;
               button_clicked = true;
            }
         }
         
         if(Contains(sandbox_bounds, mouse) && left_mouse && selected_block_id)
         {
            auto_blocks[auto_block_count++] = selected_block_id;
            selected_block_id = 0;
         }
         else if(!button_clicked && left_mouse)
         {
            selected_block_id = 0;
         }
         
         if(selected_block_id > 0)
            DrawAutoBuilderBlock(&backbuffer, selected_block_id, mouse, auto_block_colors[selected_block_id - 1], &font);
         
         for(u32 i = 0; i < auto_block_count; ++i)
         {
            if(auto_blocks[i] > 0)
            {
               b32 hit = AutoBuilderButton(&backbuffer, auto_blocks[i], 
                                           V2(sandbox_bounds.min.x + 10, sandbox_bounds.min.y + 10 + (30 * i)), 
                                           mouse, auto_block_colors[auto_blocks[i] - 1], left_mouse, &font);
               
               if(hit && auto_block_eraser)
               {
                  for(u32 i2 = i; i2 < auto_block_count; ++i2)
                  {
                     if((i2 + 1) < auto_block_count)
                     {
                        auto_blocks[i2] = auto_blocks[i2 + 1];
                     }
                  }
                  auto_block_count -= 1;
                  auto_block_eraser = false;
               }
            }
         }
         
         if(GUIButton(&backbuffer, mouse, RectPosSize(1100, 20, 100, 20), left_mouse, "Save", &font))
         {
            //TODO: open file dialog
         }
         
         if(GUIButton(&backbuffer, mouse, RectPosSize(1100, 45, 100, 20), left_mouse, "Open", &font))
         {
            
         }
         
         GUIButton(&backbuffer, mouse, RectPosSize(1100, 70, 100, 20), left_mouse, "Upload", &font);
      }
      else if(page == PageType_Fun)
      {
         DrawRectangle(&backbuffer, 10, 120, 5, 40, V4(0.0f, 0.0f, 0.0f, 1.0f));
         
         DrawRectangle(&backbuffer, 180, 20, 1200, 675, V4(0.5f, 0.0f, 0.0f, 0.5f));
         
         if(up_arrow)
         {
            player_pos.y++;
         }
         
         if(down_arrow && (player_pos.y > 0))
         {
            player_pos.y--;
         }
         
         if(player_pos.y > 0)
            player_pos.y -= 0.5f;
         
         DrawBitmap(&backbuffer, &dinosaur, player_pos.x + 190, (600 - player_pos.y) + 30);
      }
      
      DrawRectangle(&backbuffer, mouse.x, mouse.y, 10, 10, V4(mouse.x, mouse.y, 0.0f, 1.0f));
      DrawBitmap(&backbuffer, &dinosaur, mouse.x, mouse.y);      

      BlitToScreen(&backbuffer, device_context);
   }
   
   return 0;
}