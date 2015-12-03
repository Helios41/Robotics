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
   
   {
      s32 new_miny = dest->height - miny;
      s32 new_maxy = dest->height - maxy;
      
      maxy = new_miny;
      miny = new_maxy;
   }
   
   if(minx < 0)
   {
      minx = 0;
   }
   
   if(miny < 0)
   {
      miny = 0;
   }
   
   if(maxx > (s32)dest->width)
   {
      maxx = dest->width;
   }
   
   if(maxy > (s32)dest->height)
   {
      maxy = dest->height;
   }
   
   u32 *row = (dest->pixels + minx + (miny * dest->width)); 
   
   for(u32 y = miny; (s32)y < maxy; ++y)
   {
      u32 *pixel = row;
      
      for(u32 x = minx; (s32)x < maxx; ++x)
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
   if(!dest || !src)
      return;

   s32 minx = RoundR32ToS32(rx);
   s32 miny = RoundR32ToS32(ry);
   s32 maxx = RoundR32ToS32(rx) + src->width;
   s32 maxy = RoundR32ToS32(ry) + src->height;
   
   {
      s32 new_miny = dest->height - miny;
      s32 new_maxy = dest->height - maxy;
      
      maxy = new_miny;
      miny = new_maxy;
   }

   u32 xoffset = 0;
   u32 yoffset = 0;
   
   if(minx < 0)
   {
      xoffset = -minx;
      minx = 0;
   }
   
   if(miny < 0)
   {
      yoffset = -miny;
      miny = 0;
   }
   
   if(maxx > (s32)dest->width)
   {
      maxx = dest->width;
   }
   
   if(maxy > (s32)dest->height)
   {
      maxy = dest->height;
   }
   
   u32 *dest_row = dest->pixels + minx + (miny * dest->width); 
   u32 *src_row = src->pixels + xoffset + (yoffset * src->width); 
   
   for(u32 y = miny; (s32)y < maxy; ++y)
   {
      u32 *dest_pixel = dest_row;
      u32 *src_pixel = src_row;
      
      for(u32 x = minx; (s32)x < maxx; ++x)
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
   if(!dest || !text)
      return;
   
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

b32 GUIButton(RenderContext *context, MouseState mouse, rect2 bounds, LoadedBitmap *icon, char *text)
{
   b32 hot = Contains(bounds, mouse.pos);
   
   if(hot && mouse.left_down)
   {
      DrawRectangle(context->target, bounds.min.x, bounds.min.y, bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(hot)
   {
      DrawRectangle(context->target, bounds.min.x, bounds.min.y, bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
      DrawRectangle(context->target, bounds.min.x, bounds.min.y, bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   v2 bounds_size = RectGetSize(bounds);
   
   if(bounds_size.x == bounds_size.y)
   {
      DrawBitmap(context->target, icon, bounds.min.x, bounds.min.y);
      DrawText(context->target, context->font_info, V2(bounds.min.x + 10, bounds.min.y + 10), text, 20);
   }
   else
   {
      DrawBitmap(context->target, icon, bounds.min.x + 5, bounds.min.y);
      DrawText(context->target, context->font_info, V2(bounds.min.x + 10, bounds.min.y + 10), text, 20);
   }
   
   return hot && mouse.left_up;
}

b32 GUIButton(RenderContext *context, MouseState mouse, rect2 bounds, LoadedBitmap *icon, char *text, b32 triggered)
{
   v2 bounds_size = RectGetSize(bounds);
   b32 result = GUIButton(context, mouse, bounds, icon, text);
   
   DrawRectangle(context->target, bounds.min.x, bounds.min.y, 5,
                 (bounds_size.x == bounds_size.y) ? 5 : bounds_size.y,
                 triggered ? V4(0.0f, 0.0f, 0.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 0.0f));
   
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

b32 AutoBuilderButton(LoadedBitmap *dest, u32 selected_block_id, v2 pos, MouseState mouse, v4 color, stbtt_fontinfo *font)
{
   b32 hot = Contains(RectPosSize(pos.x, pos.y, 100, 20), mouse.pos);
   
   if(hot && mouse.left_down)
   {
      DrawRectangle(dest, pos.x - 2, pos.y - 2, 104, 24, V4(0.1f, 0.1f, 0.1f, 1.0f));
      DrawAutoBuilderBlock(dest, selected_block_id, pos, color, font);
   }
   else if(hot)
   {
      DrawRectangle(dest, pos.x - 2, pos.y - 2, 104, 24, V4(0.25f, 0.25f, 0.25f, 1.0f));
      DrawAutoBuilderBlock(dest, selected_block_id, pos, color, font);
   }
   else
   {
      DrawAutoBuilderBlock(dest, selected_block_id, pos, color, font);
   }
   
   return hot && mouse.left_up;
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

void UpdateMouseState(MouseState *mouse, HWND window)
{
   POINT p;
   GetCursorPos(&p);
   ScreenToClient(window, &p);
   
   mouse->pos.x = p.x;
   mouse->pos.y = p.y;
   
   mouse->left_up = false;
}

void InitMemoryArena(MemoryArena *arena, void *memory, size_t size)
{
   arena->size = size;
   arena->used = 0;
   arena->memory = memory;
}

void *PushSize(MemoryArena *arena, size_t size)
{
   Assert((arena->used + size) < arena->size);
   void *result = (u8 *)arena->memory + arena->used;
   arena->used += size;
   return result;
}

AutoBlock *NewAutoBlock(MemoryArena *arena, AutoBlockType type, char *name)
{
   AutoBlock *result = (AutoBlock *)PushSize(arena, sizeof(AutoBlock));
   result->type = type;
   result->name = name;
   return result;
}

void AppendAutoBlock(AutoBlock *new_block, AutoBlock **newest_block)
{
   new_block->prev = *newest_block;
   (*newest_block)->next = new_block;
   *newest_block = new_block;
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
   LoadedBitmap eraser = LoadBitmapFromBMP("eraser.bmp");
   LoadedBitmap competition = LoadBitmapFromBMP("competition.bmp");
   
   b32 running = true;
   MouseState mouse = {};
   
   MemoryArena generic_arena = {};
   InitMemoryArena(&generic_arena, VirtualAlloc(0, Megabyte(64), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE), Megabyte(64));
   
   RenderContext context = {};
   context.target = &backbuffer;
   context.font_info = &font;
   
   b32 connected = false;
   PageType page = PageType_Home;
   MSG msg = {};
   b32 auto_block_eraser = false;
   
   AutoBlock *newest_block = NewAutoBlock(&generic_arena, AutoBlockType_Root, "root");
   
   AppendAutoBlock(NewAutoBlock(&generic_arena, AutoBlockType_Root, "root"), &newest_block);
   AppendAutoBlock(NewAutoBlock(&generic_arena, AutoBlockType_Root, "root"), &newest_block);
   AppendAutoBlock(NewAutoBlock(&generic_arena, AutoBlockType_Root, "root"), &newest_block);
   
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
      UpdateMouseState(&mouse, window);
      
      while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
      {
         switch(msg.message)
         {            
            case WM_LBUTTONUP:
               mouse.left_down = false;
               mouse.left_up = true;
               break;
         
            case WM_LBUTTONDOWN:
               mouse.left_down = true;
               mouse.left_up = false;
               break;
               
            case WM_KEYDOWN:
            {
               switch(msg.wParam)
               {
               }
            }
            break;
            
            case WM_KEYUP:
            {
               switch(msg.wParam)
               {
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
      
      DrawRectangle(&backbuffer, 0, 0, backbuffer.width, backbuffer.height, V4(1.0f, 1.0f, 1.0f, 1.0f));
      DrawBitmap(&backbuffer, &logo, (backbuffer.width / 2) - (logo.width / 2), (backbuffer.height / 2) - (logo.height / 2));
      
      DrawRectangle(&backbuffer, 0, 0, backbuffer.width, 15, V4(1.0f, 0.0f, 0.0f, 1.0f));
      
      v4 connected_icon_color = connected ? V4(0.0f, 1.0f, 0.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 1.0f);
      DrawRectangle(&backbuffer, 10, 3, 10, 10, connected_icon_color);
      
      if(GUIButton(&context, mouse, RectPosSize(10, 20, 40, 40), &home, NULL, (page == PageType_Home)))
      {
         page = PageType_Home;
      }
      
      if(GUIButton(&context, mouse, RectPosSize(60, 20, 40, 40), &gear, NULL, (page == PageType_Config)))
      {
         page = PageType_Config;
      }
      
      if(GUIButton(&context, mouse, RectPosSize(10, 70, 120, 40), &competition, "Competition", (page == PageType_Competition)))
      {
         page = PageType_Competition;
      }
      
      if(GUIButton(&context, mouse, RectPosSize(10, 120, 120, 40), NULL, "Autonomous Builder", (page == PageType_Auto)))
      {
         page = PageType_Auto;
      }
      
      GUIButton(&context, mouse, RectPosSize(10, 170, 120, 40), NULL, NULL, false);
      
      if(page == PageType_Home)
      {
         DrawRectangle(&backbuffer, 280, 20, 800, 85, V4(0.5f, 0.0f, 0.0f, 0.5f));
         DrawText(&backbuffer, &font, V2(600, 40), "CN Robotics", 40);
      }
      else if(page == PageType_Config)
      {
         
      }
      else if(page == PageType_Auto)
      {
         b32 button_clicked = false;
         
         rect2 sandbox_bounds = RectPosSize(280, 20, 800, 675);
         DrawRectangle(&backbuffer, 280, 20, 800, 675, V4(0.5f, 0.0f, 0.0f, 0.5f));
         DrawText(&backbuffer, &font, V2(600, 40), auto_file_name, 25);
         
         u32 index = 0;
         for(AutoBlock *block = newest_block;
             block;
             block = block->prev)
         {
            DrawAutoBuilderBlock(&backbuffer, 1, V2(sandbox_bounds.min.x + 10, sandbox_bounds.min.y + 10 + (30 * index)), V4(0.0f, 1.0f, 0.0f, 1.0f), &font);
            index++;
         }
         
         if(GUIButton(&context, mouse, RectPosSize(160, 20, 40, 40), &eraser, NULL, auto_block_eraser))
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
                                        mouse, auto_block_colors[i - 1], &font);
            
            if(hit)
            {
               selected_block_id = i;
               button_clicked = true;
            }
         }
         
         if(Contains(sandbox_bounds, mouse.pos) && mouse.left_up && selected_block_id)
         {
            auto_blocks[auto_block_count++] = selected_block_id;
            selected_block_id = 0;
         }
         else if(!button_clicked && mouse.left_up)
         {
            selected_block_id = 0;
         }
         
         if(selected_block_id > 0)
            DrawAutoBuilderBlock(&backbuffer, selected_block_id, mouse.pos, auto_block_colors[selected_block_id - 1], &font);
         
         for(u32 i = 0; i < auto_block_count; ++i)
         {
            if(auto_blocks[i] > 0)
            {
               b32 hit = AutoBuilderButton(&backbuffer, auto_blocks[i], 
                                           V2(sandbox_bounds.min.x + 10, sandbox_bounds.min.y + 10 + (30 * i)), 
                                           mouse, auto_block_colors[auto_blocks[i] - 1], &font);
               
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
               }
            }
         }
         
         if(GUIButton(&context, mouse, RectPosSize(1100, 20, 100, 20), NULL, "Save"))
         {
            //TODO: open file dialog
         }
         
         if(GUIButton(&context, mouse, RectPosSize(1100, 45, 100, 20), NULL, "Open"))
         {
            
         }
         
         GUIButton(&context, mouse, RectPosSize(1100, 70, 100, 20), NULL, "Upload");
      }
      
      DrawRectangle(&backbuffer, mouse.pos.x, mouse.pos.y, 10, 10, V4(mouse.pos.x, mouse.pos.y, 0.0f, 1.0f));  
      DrawBitmap(&backbuffer, &home, mouse.pos.x, mouse.pos.y);
      
      BlitToScreen(&backbuffer, device_context);
   }
   
   return 0;
}