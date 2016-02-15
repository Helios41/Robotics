#include "Definitions.h"
#include "Dashboard.h"
#include <stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>

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
		
	return DefWindowProc(window, message, wParam, lParam);
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

void PopSize(MemoryArena *arena, size_t size)
{
   arena->used -= size;
}

EntireFile LoadEntireFile(const char* path)
{
   HANDLE file_handle = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, NULL);
                                    
   EntireFile result = {};
   
   DWORD number_of_bytes_read;
   result.length = GetFileSize(file_handle, NULL);
   result.contents = VirtualAlloc(0, result.length, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
   ReadFile(file_handle, result.contents, result.length, &number_of_bytes_read, NULL);
   CloseHandle(file_handle);
   
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

LoadedFont GetCharBitmap(stbtt_fontinfo *font, char c, u32 height_scale, MemoryArena *memory)
{
   LoadedFont result = {};
   
   int width, height, xoffset, yoffset;
   u8 *mono_bitmap = stbtt_GetCodepointBitmap(font, 0, stbtt_ScaleForPixelHeight(font, height_scale),
                                              c, &width, &height, &xoffset, &yoffset);
   
   result.bitmap.width = (u32)width;
   result.bitmap.height = (u32)height;
   result.offset.x = (r32)xoffset;
   result.offset.y = (r32)yoffset;
   
   
   result.bitmap.pixels = (u32 *)PushSize(memory, (result.bitmap.width * result.bitmap.height * sizeof(u32)));
   //(u32 *)VirtualAlloc(0, (result.bitmap.width * result.bitmap.height * sizeof(u32)), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
   
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

void BlitToScreen(LoadedBitmap *bitmap, HDC device_context, HWND window)
{
   BITMAPINFO info = {};
   
   info.bmiHeader.biSize = sizeof(info.bmiHeader);
   info.bmiHeader.biWidth = bitmap->width;
   info.bmiHeader.biHeight = bitmap->height;
   info.bmiHeader.biPlanes = 1;
   info.bmiHeader.biBitCount = 32;
   info.bmiHeader.biCompression = BI_RGB;
   
   RECT client_rect = {};
   GetClientRect(window, &client_rect);
   
   StretchDIBits(device_context,
                 0, 0, client_rect.right, client_rect.bottom,
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

void DrawText(LoadedBitmap *dest, stbtt_fontinfo *font, v2 pos, char *text, u32 scale, MemoryArena *memory)
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
         LoadedFont char_bitmap = GetCharBitmap(font, *text, scale, memory);
         DrawBitmap(dest, &char_bitmap.bitmap, pos.x + xoffset + char_bitmap.offset.x, pos.y + char_bitmap.offset.y + 10);
         PopSize(memory, (char_bitmap.bitmap.width * char_bitmap.bitmap.height * sizeof(u32)));
         xoffset += char_bitmap.bitmap.width;
      }
      text++;
   }
}

b32 GUIButton(RenderContext *context, InputState input, rect2 bounds, LoadedBitmap *icon, char *text)
{
   b32 hot = Contains(bounds, input.pos);
   
   if(hot && (input.left_down || input.right_down))
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
      DrawText(context->target, context->font_info, V2(bounds.min.x + 10, bounds.min.y + 10), text, 20, context->font_memory);
   }
   else
   {
      DrawBitmap(context->target, icon, bounds.min.x + 5, bounds.min.y);
      DrawText(context->target, context->font_info, V2(bounds.min.x + 10, bounds.min.y + 10), text, 20, context->font_memory);
   }
   
   return hot && (input.left_up || input.right_up);
}

b32 GUIButton(RenderContext *context, InputState input, rect2 bounds, LoadedBitmap *icon, char *text, b32 triggered)
{
   v2 bounds_size = RectGetSize(bounds);
   b32 result = GUIButton(context, input, bounds, icon, text);
   
   DrawRectangle(context->target, bounds.min.x, bounds.min.y, 5,
                 (bounds_size.x == bounds_size.y) ? 5 : bounds_size.y,
                 triggered ? V4(0.0f, 0.0f, 0.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 0.0f));
   
   return result;
}

b32 AutoBuilderBlock(RenderContext *context, AutoBlock block, v2 pos, InputState input)
{
   b32 hot = Contains(RectPosSize(pos.x, pos.y, 100, 20), input.pos);
   
   if(hot && (input.left_down || input.right_down))
   {
      DrawRectangle(context->target, pos.x - 2, pos.y - 2, 104, 24, V4(0.1f, 0.1f, 0.1f, 1.0f));
   }
   else if(hot)
   {
      DrawRectangle(context->target, pos.x - 2, pos.y - 2, 104, 24, V4(0.25f, 0.25f, 0.25f, 1.0f));
   }
   
   DrawRectangle(context->target, pos.x, pos.y, 100, 20, V4(0.5f, 0.0f, 0.0f, 1.0f));
   DrawText(context->target, context->font_info, V2(pos.x, pos.y), block.name, 20, context->font_memory);
   
   return hot && (input.left_up || input.right_up);
}

HDC SetupWindow(HINSTANCE hInstance, int nCmdShow, HWND *window, LoadedBitmap *backbuffer)
{
   *window = CreateWindowExA(WS_EX_CLIENTEDGE, "WindowClass", "Dashboard V2",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             1440, 759, NULL, NULL,
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

void UpdateInputState(InputState *input, HWND window)
{
   POINT p;
   GetCursorPos(&p);
   ScreenToClient(window, &p);
   
   input->pos.x = p.x;
   input->pos.y = p.y;
   
   input->left_up = false;
   input->right_up = false;
   
   input->char_key_up = false;
   input->key_backspace = false;
   input->key_up = false;
   input->key_down = false;
}

u32 StringLength(char *str)
{
   u32 len = 0;
   while(*str)
   {
      len++;
      str++;
   }
   return len;
}

char *R32ToString(r32 value, char *str)
{
   sprintf(str, "%f", value);
   return str;
}

char *R64ToString(r64 value, char *str)
{
   sprintf(str, "%lf", value);
   return str;
}

char *U32ToString(u32 value, char *str)
{
   sprintf(str, "%u", value);
   return str;
}

char *ConcatStrings(char *str1, char *str2, char *str)
{
   u32 i = 0;
   while(*str1)
   {
      str[i++] = *str1;
      str1++;
   }
   
   while(*str2)
   {
      str[i++] = *str2;
      str2++;
   }

   str[i] = '\0';
   return str;
}

char *ConcatStrings(char *str1, char *str2, char *str3, char *str)
{
   u32 i = 0;
   while(*str1)
   {
      str[i++] = *str1;
      str1++;
   }
   
   while(*str2)
   {
      str[i++] = *str2;
      str2++;
   }

   while(*str3)
   {
      str[i++] = *str3;
      str3++;
   }
   
   str[i] = '\0';
   return str;
}

void StringCopy(char *src, char *dest)
{
   while(*src)
   {
      *dest = *src;
      dest++;
      src++;
   }
   dest = '\0';
}

AutoBlock *AddAutoBlock(AutoBlock *auto_blocks, u32 *auto_block_count, AutoBlockPreset preset)
{
   AutoBlock result = {};
   StringCopy(preset.name, result.name);
   result.hwid = preset.hwid;
   result.type = preset.type;
   
   auto_blocks[*auto_block_count] = result;
   (*auto_block_count)++;
   
   return &auto_blocks[*auto_block_count - 1];
}

RobotSubsystem *AddSubsystem(RobotSubsystem *subsystems, u32 *subsystem_count)
{
   RobotSubsystem result = {};
   
   subsystems[*subsystem_count] = result;
   (*subsystem_count)++;
   
   return &subsystems[*subsystem_count - 1];
}

char *GetNameFromType(SubsystemType type)
{
   switch(type)
   {
      case SubsystemType_TankDrive2x2:
         return "Tank Drive (2 x 2)";
         
      default:
         return "default";
   }
}

r64 GetCounterFrequency(void)
{
   LARGE_INTEGER frequency_value = {};
   QueryPerformanceFrequency(&frequency_value);
   r64 result = ((r64)frequency_value.QuadPart) / 1000.0;
   return result;
}

r64 GetCounter(s64 *last_timer, r64 frequency)
{
   LARGE_INTEGER timer_value = {};
   QueryPerformanceCounter(&timer_value);
   r64 result = ((r64)(timer_value.QuadPart - *last_timer)) / frequency;
   *last_timer = timer_value.QuadPart;
   return result;
}

#define GOOGLE_NETWORK_TEST

#ifdef GOOGLE_NETWORK_TEST
#define SERVER_ADDR "216.58.218.195"
#define SERVER_PORT 80
#else   
#define SERVER_ADDR "10.46.18.11"
#define SERVER_PORT 8089
#endif

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
   InputState input = {};
   b32 fullscreen = false;
   b32 connected = false;
   PageType page = PageType_Home;
   MSG msg = {};
   r64 timer_freq = GetCounterFrequency();
   s64 last_timer = 0;
   r64 frame_length = 0.0;
   
   WSADATA winsock_data = {};
   WSAStartup(MAKEWORD(2, 2), &winsock_data);
   
   SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   //b32 is_nonblocking = true;
   //ioctlsocket(server_socket, FIONBIO, (u_long *)&is_nonblocking);
   
   char inet_buffer[64];
   struct sockaddr_in server;
   server.sin_family = AF_INET;
   
   //server.sin_addr.s_addr = InetPton(AF_INET, SERVER_ADDR, inet_buffer);
   server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
   server.sin_port = htons(SERVER_PORT);
   
   connected = (connect(server_socket, (struct sockaddr *)&server, sizeof(server)) == 0);
   
   MemoryArena generic_arena = {};
   InitMemoryArena(&generic_arena, VirtualAlloc(0, Megabyte(64), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE), Megabyte(64));
   
   RenderContext context = {};
   context.target = &backbuffer;
   context.font_info = &font;
   context.font_memory = &generic_arena;
   
   AutoBuilderState auto_builder_state = {};
   StringCopy("unnamed", auto_builder_state.auto_file_name);
   
   RobotState robot_state = {};
   robot_state.robot_page = RobotPageType_Hardware;
   
   //
   auto_builder_state.auto_block_preset_count = 4;
   auto_builder_state.auto_block_presets[0] = {"Victor 1", 1, AutoBlockType_Motor};
   auto_builder_state.auto_block_presets[1] = {"Victor 2", 2, AutoBlockType_Motor};
   auto_builder_state.auto_block_presets[2] = {"Victor 3", 3, AutoBlockType_Motor};
   auto_builder_state.auto_block_presets[3] = {"Solenoid 1", 1, AutoBlockType_Solenoid};
   
   robot_state.robot_hardware_count = 4;
   robot_state.robot_hardware[0] = {"Victor 1", 1, HardwareType_MotorController};
   robot_state.robot_hardware[1] = {"Victor 2", 2, HardwareType_MotorController};
   robot_state.robot_hardware[2] = {"Victor 3", 3, HardwareType_MotorController};
   robot_state.robot_hardware[3] = {"Solenoid 1", 1, HardwareType_Solenoid};
   //
   
   GetCounter(&last_timer, timer_freq);
   while(running)
   {
      UpdateInputState(&input, window);
      
      while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
      {
         switch(msg.message)
         {            
            case WM_LBUTTONUP:
               input.left_down = false;
               input.left_up = true;
               break;
         
            case WM_LBUTTONDOWN:
               input.left_down = true;
               input.left_up = false;
               break;
               
            case WM_RBUTTONUP:
               input.right_down = false;
               input.right_up = true;
               break;
         
            case WM_RBUTTONDOWN:
               input.right_down = true;
               input.right_up = false;
               break;   
            
            case WM_KEYUP:
            {
               if(msg.wParam == VK_BACK)
               {
                  input.key_backspace = true;
               }
               else if(msg.wParam == VK_UP)
               {
                  input.key_up = true;
               }
               else if(msg.wParam == VK_DOWN)
               {
                  input.key_down = true;
               }
            }
            break;
            
            case WM_CHAR:
            {
               char c = msg.wParam;
               if((c > 31) && (c < 127))
               {
                  input.char_key_up = true;
                  input.key_char = c;
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

      DrawRectangle(&backbuffer, 10, 3, 10, 10, connected ? V4(1.0f, 1.0f, 1.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 1.0f));
      
      if(GUIButton(&context, input, RectPosSize(10, 20, 40, 40), &home, NULL, (page == PageType_Home)))
      {
         page = PageType_Home;
      }
      else if(GUIButton(&context, input, RectPosSize(10, 70, 120, 40), NULL, "Autonomous Builder", (page == PageType_Auto)))
      {
         page = PageType_Auto;
      }
      else if(GUIButton(&context, input, RectPosSize(10, 120, 120, 40), NULL, "Robot", (page == PageType_Robot)))
      {
         page = PageType_Robot;
      }
      else if(GUIButton(&context, input, RectPosSize(10, 170, 120, 40), NULL, "---", false))
      {
#ifdef GOOGLE_NETWORK_TEST
         if(connected)
         {
            char test_message[] = "GET / HTTP/1.1\r\n\r\n";
            if(send(server_socket, test_message, StringLength(test_message), 0) < 0)
            {
               MessageBoxA(NULL, "Send failed", "GNT Error", MB_OK);
            }
            
            char server_reply[4000];
            int recive_size = recv(server_socket, server_reply, 4000, 0);
            if(recive_size == SOCKET_ERROR)
            {
                MessageBoxA(NULL, "Recive failed", "GNT Error", MB_OK);
            }
            
            server_reply[recive_size] = '\0';
            MessageBoxA(NULL, server_reply, "GNT", MB_OK);
         }
#endif
      }
      
      if(GUIButton(&context, input, RectPosSize(60, 20, 40, 40), &gear, NULL, fullscreen))
      {
         fullscreen = !fullscreen;
         
         if(fullscreen)
         {
            DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
            DWORD dwRemove = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
            DWORD dwNewStyle = dwStyle & ~dwRemove;
            
            SetWindowLong(window, GWL_STYLE, dwNewStyle);
            SetWindowPos(window, NULL, 0, 0, 0, 0,
                         SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                       
            SetWindowPos(window, NULL, 0, 0, 
                         GetDeviceCaps(device_context, HORZRES), backbuffer.height, SWP_FRAMECHANGED);
         }
         else
         {
            SetWindowLong(window, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
            SetWindowPos(window, NULL, 0, 0, backbuffer.width, backbuffer.height, SWP_FRAMECHANGED);
         }
      }
      
      if(page == PageType_Home)
      {
         DrawRectangle(&backbuffer, 280, 20, 800, 85, V4(0.5f, 0.0f, 0.0f, 0.5f));
         DrawText(&backbuffer, &font, V2(600, 40), "CN Robotics", 40, context.font_memory);
         
         DrawRectangle(&backbuffer, 160, 110, 400, 255, V4(0.5f, 0.0f, 0.0f, 0.5f));
         
         if(connected)
         {
            char text_buffer[512];
            DrawText(&backbuffer, &font, V2(180, 130),
                     ConcatStrings("Connected To ", SERVER_ADDR, text_buffer),
                     20, context.font_memory);
         }
         else
         {
            DrawText(&backbuffer, &font, V2(180, 130), "Not Connected", 20, context.font_memory);
         }
         
         if(GUIButton(&context, input, RectPosSize(180, 180, 100, 40), NULL, "Reconnect"))
         {
            shutdown(server_socket, SD_BOTH);
            closesocket(server_socket);
            server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
            server.sin_port = htons(SERVER_PORT);
            
            connected = (connect(server_socket, (struct sockaddr *)&server, sizeof(server)) == 0);
         }
      }
      else if(page == PageType_Auto)
      {
         rect2 sandbox_bounds = RectPosSize(280, 20, 800, 675);
         DrawRectangle(&backbuffer, 280, 20, 800, 675, V4(0.5f, 0.0f, 0.0f, 0.5f));
                  
         for(u32 i = 0; i < auto_builder_state.auto_block_count; i++)
         {
            if(auto_builder_state.selected_block == &auto_builder_state.auto_blocks[i])
            {
               DrawRectangle(&backbuffer, (sandbox_bounds.min.x + 5), 
                             (sandbox_bounds.min.y + 5 + (30 * i)), 110, 30, V4(0.5f, 0.0f, 0.0f, 0.5f));
            }
            
            b32 hit = AutoBuilderBlock(&context, auto_builder_state.auto_blocks[i],
                                       V2(sandbox_bounds.min.x + 10, sandbox_bounds.min.y + 10 + (30 * i)),
                                       input);
                         
            if(hit && input.left_up)
            {
               auto_builder_state.selected_block = &auto_builder_state.auto_blocks[i];
            }
            else if(hit && input.right_up)
            {
               for(u32 j = i + 1; j < auto_builder_state.auto_block_count; j++)
               {
                  auto_builder_state.auto_blocks[j - 1] = auto_builder_state.auto_blocks[j];
               }
               auto_builder_state.auto_block_count--;
               auto_builder_state.selected_block = NULL;
            }
         }
         
         for(u32 i = 0;
             i < auto_builder_state.auto_block_preset_count;
             ++i)
         {
            b32 hit = GUIButton(&context, input, RectPosSize(160, (20 + (i *25)), 100, 20), NULL, auto_builder_state.auto_block_presets[i].name);
            if(hit)
            {
               AddAutoBlock(auto_builder_state.auto_blocks, &auto_builder_state.auto_block_count, auto_builder_state.auto_block_presets[i]);
            }
         }
         
         if(GUIButton(&context, input, RectPosSize(1100, 20, 140, 40), NULL, auto_builder_state.auto_file_name, auto_builder_state.auto_file_name_selected))
         {
            auto_builder_state.auto_file_name_selected = !auto_builder_state.auto_file_name_selected;
         }
         
         if(auto_builder_state.auto_file_name_selected)
         {
            u32 len = StringLength(auto_builder_state.auto_file_name);
            
            if(input.char_key_up && ((len + 1) < ArrayCount(auto_builder_state.auto_file_name)))
            {
               auto_builder_state.auto_file_name[len] = input.key_char;
               auto_builder_state.auto_file_name[len + 1] = '\0';
            }
            else if(input.key_backspace && (len > 0))
            {
               auto_builder_state.auto_file_name[len - 1] = '\0';
            }
         }
         
         if(GUIButton(&context, input, RectPosSize(1245, 20, 20, 40), NULL, NULL, auto_builder_state.auto_file_selector_open))
         {
            auto_builder_state.auto_file_selector_open = !auto_builder_state.auto_file_selector_open;
         }
         
         if(auto_builder_state.auto_file_selector_open)
         {
            rect2 block_info_bounds = RectPosSize(1100, 140, 200, 200);
            DrawRectangle(&backbuffer, 1100, 140, 200, 200, V4(0.5f, 0.0f, 0.0f, 0.5f));
            
            HANDLE find_handle;
            WIN32_FIND_DATA find_data;
            u32 file_index = 0;
            
            find_handle = FindFirstFile("autos/*.abin", &find_data);
            if(find_handle != INVALID_HANDLE_VALUE)
            {
               do
               {
                  if(GUIButton(&context, input, RectPosSize(1120, 160 + (file_index * 30), 100, 20), NULL, find_data.cFileName))
                  {
                     u32 i = 0;
                     while(i < (StringLength(find_data.cFileName) - 5))
                     {
                        auto_builder_state.auto_file_name[i] = find_data.cFileName[i];
                        i++;
                     }
                     auto_builder_state.auto_file_name[i] = '\0';
                  }
                  file_index++;
               }
               while(FindNextFile(find_handle, &find_data));
            }
         }
         
         if(GUIButton(&context, input, RectPosSize(1100, 65, 100, 20), NULL, "Save"))
         {
            DWORD autos_folder = GetFileAttributes("autos");
            if((autos_folder == INVALID_FILE_ATTRIBUTES) &&
               (autos_folder & FILE_ATTRIBUTE_DIRECTORY))
            {
               CreateDirectory("autos", NULL);
            }
            
            u64 file_size = sizeof(AutoFileHeader) + (sizeof(AutoBlock) * auto_builder_state.auto_block_count);
            void *file_data = PushSize(&generic_arena, file_size);
            char path_buffer[128];
            
            AutoFileHeader *header = (AutoFileHeader *) file_data;
            AutoBlock *blocks = (AutoBlock *) (header + 1);
            
            header->identifier = 0x44661188;
            header->auto_block_count = auto_builder_state.auto_block_count;
            
            for(u32 i = 0; i < auto_builder_state.auto_block_count; i++)
            {
               *(blocks + i) = auto_builder_state.auto_blocks[i];
            }
            
            HANDLE file_handle = CreateFileA(ConcatStrings("autos/", auto_builder_state.auto_file_name, ".abin", path_buffer),
                                             GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if(file_handle == INVALID_HANDLE_VALUE) MessageBoxA(NULL, "Error", "invalid handle", MB_OK);
            
            DWORD bytes_written = 0;
            if(WriteFile(file_handle, file_data, file_size, &bytes_written, NULL) == false) MessageBoxA(NULL, "Error", "write error", MB_OK);
            CloseHandle(file_handle);
            PopSize(&generic_arena, file_size);
         }
         
         if(GUIButton(&context, input, RectPosSize(1100, 90, 100, 20), NULL, "Open"))
         {
            char path_buffer[128];
            HANDLE file_handle = CreateFileA(ConcatStrings("autos/",auto_builder_state.auto_file_name, ".abin", path_buffer),
                                             GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            
            if(file_handle != INVALID_HANDLE_VALUE)
            {
               u64 file_size = GetFileSize(file_handle, NULL);
               void *file_data = PushSize(&generic_arena, file_size);
               
               AutoFileHeader *header = (AutoFileHeader *) file_data;
               AutoBlock *blocks = (AutoBlock *) (header + 1);
               
               DWORD bytes_written = 0;
               ReadFile(file_handle, file_data, file_size, &bytes_written, NULL);
               
               if(header->identifier == 0x44661188)
               {
                  auto_builder_state.auto_block_count = header->auto_block_count;
                  
                  for(u32 i = 0; i < auto_builder_state.auto_block_count; i++)
                  {
                     auto_builder_state.auto_blocks[i] = *(blocks + i);
                  }
               }
               else
               {
                  MessageBoxA(NULL, "Invalid file", "Error", MB_OK);
               }
               
               PopSize(&generic_arena, file_size);
            }
            
            CloseHandle(file_handle);
         }
         
         if(GUIButton(&context, input, RectPosSize(1100, 115, 100, 20), NULL, "Upload"))
         {
            //TODO
         }
         
         if(auto_builder_state.selected_block)
         {
            rect2 block_info_bounds = RectPosSize(1100, 140, 200, 200);
            DrawRectangle(&backbuffer, 1100, 140, 200, 200, V4(0.5f, 0.0f, 0.0f, 0.5f));
            DrawText(&backbuffer, &font, V2(1120, 160), auto_builder_state.selected_block->name, 20, context.font_memory);
            
            if(auto_builder_state.selected_block->type == AutoBlockType_Motor)
            {
               char number_buffer[10];
               char string_buffer[30];
               
               DrawText(&backbuffer, &font, V2(1120, 180),
                        ConcatStrings("Value: ", R32ToString(auto_builder_state.selected_block->motor.value, number_buffer), string_buffer),
                        20, context.font_memory);
               
               r32 clipped_value = (auto_builder_state.selected_block->motor.value + 1.0f) / 2.0f;
               DrawRectangle(&backbuffer, 1120, 200, 150, 20, V4(0.5f, 0.0f, 0.0f, 1.0f));
               DrawRectangle(&backbuffer, 1115 + (clipped_value * 150), 195, 30, 30, V4(0.9f, 0.0f, 0.0f, 1.0f));
               
               if(Contains(RectPosSize(1115 + (clipped_value * 150), 195, 30, 30), input.pos) && input.left_down)
               {
                  DrawRectangle(&backbuffer, 1115 + (clipped_value * 150), 195, 30, 30, V4(1.0f, 0.0f, 0.0f, 1.0f));
                  
                  if(input.pos.x > (1130 + (clipped_value * 150)))
                  {
                     auto_builder_state.selected_block->motor.value += 0.1f;
                  }
                  else if(input.pos.x < (1130 + (clipped_value * 150)))
                  {
                     auto_builder_state.selected_block->motor.value -= 0.1f;
                  }
                  
                  if(auto_builder_state.selected_block->motor.value > 1.0f)
                  {
                     auto_builder_state.selected_block->motor.value = 1.0f;
                  }
                  else if(auto_builder_state.selected_block->motor.value < -1.0f)
                  {
                     auto_builder_state.selected_block->motor.value = -1.0f;
                  }
               }
               
               DrawText(&backbuffer, &font, V2(1120, 240),
                        ConcatStrings("Time: ", R32ToString(auto_builder_state.selected_block->motor.time, number_buffer), string_buffer),
                        20, context.font_memory);
               
               if(GUIButton(&context, input, RectPosSize(1140, 280, 20, 20), NULL, "+"))
               {
                  auto_builder_state.selected_block->motor.time += 0.1f;
               }
               else if(GUIButton(&context, input, RectPosSize(1180, 280, 20, 20), NULL, "-"))
               {
                  auto_builder_state.selected_block->motor.time -= 0.1f;
               }
            }
            else if(auto_builder_state.selected_block->type == AutoBlockType_Solenoid)
            {
               if(GUIButton(&context, input, RectPosSize(1120, 180, 80, 40), NULL, "Extend", auto_builder_state.selected_block->solenoid == SolenoidState_Extend))
               {
                  auto_builder_state.selected_block->solenoid = SolenoidState_Extend;
               }
               
               if(GUIButton(&context, input, RectPosSize(1120, 225, 80, 40), NULL, "Retract", auto_builder_state.selected_block->solenoid == SolenoidState_Retract))
               {
                  auto_builder_state.selected_block->solenoid = SolenoidState_Retract;
               }
               
               if(GUIButton(&context, input, RectPosSize(1120, 270, 80, 40), NULL, "Stop", auto_builder_state.selected_block->solenoid == SolenoidState_Stop))
               {
                  auto_builder_state.selected_block->solenoid = SolenoidState_Stop;
               }
            }
            
            u32 selected_block_index = 0;
            
            for(u32 i = 0; i < auto_builder_state.auto_block_count; i++)
            {
               if(auto_builder_state.selected_block == &auto_builder_state.auto_blocks[i])
               {
                  selected_block_index = i;
               }
            }
            
            if(input.key_up && (selected_block_index > 0))
            {
               AutoBlock temp = auto_builder_state.auto_blocks[selected_block_index - 1];
               auto_builder_state.auto_blocks[selected_block_index - 1] = *auto_builder_state.selected_block;
               auto_builder_state.auto_blocks[selected_block_index] = temp;
               auto_builder_state.selected_block = &auto_builder_state.auto_blocks[selected_block_index - 1];
            }
            else if(input.key_down && ((selected_block_index + 1) < auto_builder_state.auto_block_count))
            {
               AutoBlock temp = auto_builder_state.auto_blocks[selected_block_index + 1];
               auto_builder_state.auto_blocks[selected_block_index + 1] = *auto_builder_state.selected_block;
               auto_builder_state.auto_blocks[selected_block_index] = temp;
               auto_builder_state.selected_block = &auto_builder_state.auto_blocks[selected_block_index + 1];
            }
            
            if(GUIButton(&context, input, RectPosSize(1100, 140, 10, 10), NULL, NULL))
            {
               auto_builder_state.selected_block = NULL;
            }
         }
      }
      else if(page == PageType_Robot)
      {
         DrawRectangle(&backbuffer, 280, 20, 800, 675, V4(0.5f, 0.0f, 0.0f, 0.5f));
         
         if(GUIButton(&context, input, RectPosSize(160, 20, 100, 20), NULL, "Hardware", robot_state.robot_page == RobotPageType_Hardware))
         {
            robot_state.robot_page = RobotPageType_Hardware;
         }
         
         if(GUIButton(&context, input, RectPosSize(160, 45, 100, 20), NULL, "Subsystems", robot_state.robot_page == RobotPageType_Subsystems))
         {
            robot_state.robot_page = RobotPageType_Subsystems;
         }
         
         if(robot_state.robot_page == RobotPageType_Hardware)
         {
            for(u32 i = 0; i < robot_state.robot_hardware_count; i++)
            {
               if(robot_state.selected_hardware == &robot_state.robot_hardware[i])
               {
                  DrawRectangle(&backbuffer, 285, 
                                (25 + (30 * i)), 110, 30, V4(0.5f, 0.0f, 0.0f, 0.5f));
               }
               
               if(GUIButton(&context, input, RectPosSize(290, (30 + (30 * i)), 100, 20), NULL, robot_state.robot_hardware[i].name))
               {
                  robot_state.selected_hardware = &robot_state.robot_hardware[i];
               }
            }
            
            if(robot_state.selected_hardware)
            {
               if(robot_state.selected_hardware->type == HardwareType_MotorController)
               {
                  DrawRectangle(&backbuffer, 1100, 20, 225, 400, V4(0.5f, 0.0f, 0.0f, 0.5f));
                  DrawText(&backbuffer, &font, V2(1120, 40), robot_state.selected_hardware->name, 20, context.font_memory);
                  
                  if(GUIButton(&context, input, RectPosSize(1105, 60, 100, 20), NULL, "Analog") &&
                     (robot_state.selected_hardware->control_count < ArrayCount(robot_state.selected_hardware->motor)))
                  {
                     u32 index = robot_state.selected_hardware->control_count++;
                     
                     robot_state.selected_hardware->motor[index].type = MotorControllerControlType_Analog;
                     robot_state.selected_hardware->motor[index].controller_index = 0;
                     robot_state.selected_hardware->motor[index].button_axis_index = 0;
                  }
                  
                  if(GUIButton(&context, input, RectPosSize(1210, 60, 100, 20), NULL, "Button") &&
                     (robot_state.selected_hardware->control_count < ArrayCount(robot_state.selected_hardware->motor)))
                  {
                     u32 index = robot_state.selected_hardware->control_count++;
                     
                     robot_state.selected_hardware->motor[index].type = MotorControllerControlType_Button;
                     robot_state.selected_hardware->motor[index].controller_index = 0;
                     robot_state.selected_hardware->motor[index].button_axis_index = 1;
                     robot_state.selected_hardware->motor[index].value = 0.0f;
                  }
                  
                  u32 analog_index = 0;
                  u32 button_index = 0;
                  for(u32 i = 0; i < robot_state.selected_hardware->control_count; i++)
                  {
                     u32 x = (robot_state.selected_hardware->motor[i].type == MotorControllerControlType_Analog) ? 1105 : 1210;
                     u32 y = (robot_state.selected_hardware->motor[i].type == MotorControllerControlType_Analog) ? (95 + (analog_index * 25)) : (95 + (button_index * 25));
                     
                     if(robot_state.hardware_control_selected &&
                       (robot_state.selected_hardware_control == i))
                     {
                        DrawRectangle(&backbuffer, x - 5, y - 5, 110, 30, V4(0.5f, 0.0f, 0.0f, 0.5f));
                     }
                     
                     char controller_index_buffer[64];
                     char button_axis_index_buffer[64];
                     char string_buffer[64];
                     
                     if(GUIButton(&context, input, RectPosSize(x, y, 100, 20), NULL,
                                  ConcatStrings(U32ToString(robot_state.selected_hardware->motor[i].controller_index, controller_index_buffer),
                                                " : ",
                                                U32ToString(robot_state.selected_hardware->motor[i].button_axis_index, button_axis_index_buffer),
                                                string_buffer)))
                     {
                        if(input.left_up)
                        {
                           robot_state.hardware_control_selected = true;
                           robot_state.selected_hardware_control = i;
                        }
                        else if(input.right_up)
                        {
                           for(u32 j = i + 1; j < robot_state.selected_hardware->control_count; j++)
                           {
                              robot_state.selected_hardware->motor[j - 1] = robot_state.selected_hardware->motor[j];
                           }
                           robot_state.selected_hardware->control_count--;
                           robot_state.hardware_control_selected = false;
                        }
                     }
                     
                     if(robot_state.selected_hardware->motor[i].type == MotorControllerControlType_Analog)
                     {
                        analog_index++;
                     }
                     else if(robot_state.selected_hardware->motor[i].type == MotorControllerControlType_Button)
                     {
                        button_index++;
                     }
                  }
                  
                  if(robot_state.hardware_control_selected)
                  {
                     DrawRectangle(&backbuffer, 1100, 425, 225, 225, V4(0.5f, 0.0f, 0.0f, 0.5f));
                     char index_buffer[32];
                     char string_buffer[64];
                     
                     if(GUIButton(&context, input, RectPosSize(1105, 450, 100, 20), NULL,
                        ConcatStrings("Controller: ",
                                      U32ToString(robot_state.selected_hardware->motor[robot_state.selected_hardware_control].controller_index, index_buffer),
                                      string_buffer)))
                     {
                        if(input.left_up && (robot_state.selected_hardware->motor[robot_state.selected_hardware_control].controller_index < 4))
                        {
                           robot_state.selected_hardware->motor[robot_state.selected_hardware_control].controller_index++;
                        }
                        else if(input.right_up && (robot_state.selected_hardware->motor[robot_state.selected_hardware_control].controller_index > 0))
                        {
                           robot_state.selected_hardware->motor[robot_state.selected_hardware_control].controller_index--;
                        }
                     }
                     
                     if(robot_state.selected_hardware->motor[robot_state.selected_hardware_control].type == MotorControllerControlType_Analog)
                     {
                        if(GUIButton(&context, input, RectPosSize(1105, 475, 100, 20), NULL,
                        ConcatStrings("Axis: ",
                                      U32ToString(robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index, index_buffer),
                                      string_buffer)))
                        {
                           if(input.left_up && (robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index < 20))
                           {
                              robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index++;
                           }
                           else if(input.right_up && (robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index > 0))
                           {
                              robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index--;
                           }
                        }
                     }
                     else if(robot_state.selected_hardware->motor[robot_state.selected_hardware_control].type == MotorControllerControlType_Button)
                     {
                        if(GUIButton(&context, input, RectPosSize(1105, 475, 100, 20), NULL,
                        ConcatStrings("Button: ",
                                      U32ToString(robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index, index_buffer),
                                      string_buffer)))
                        {
                           if(input.left_up && (robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index < 20))
                           {
                              robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index++;
                           }
                           else if(input.right_up && (robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index > 1))
                           {
                              robot_state.selected_hardware->motor[robot_state.selected_hardware_control].button_axis_index--;
                           }
                        }
                        
                        if(GUIButton(&context, input, RectPosSize(1105, 500, 100, 20), NULL,
                        ConcatStrings("Value: ",
                                      R32ToString(robot_state.selected_hardware->motor[robot_state.selected_hardware_control].value, index_buffer),
                                      string_buffer)))
                        {
                           if(input.left_up && (robot_state.selected_hardware->motor[robot_state.selected_hardware_control].value < 1.0f))
                           {
                              robot_state.selected_hardware->motor[robot_state.selected_hardware_control].value += 0.01f;
                           }
                           else if(input.right_up && (robot_state.selected_hardware->motor[robot_state.selected_hardware_control].value > -1.0f))
                           {
                              robot_state.selected_hardware->motor[robot_state.selected_hardware_control].value -= 0.01f;
                           }
                        }
                     }
                     
                     if(GUIButton(&context, input, RectPosSize(1100, 425, 10, 10), NULL, NULL))
                     {
                        robot_state.hardware_control_selected = false;
                     }
                  }
               }
               else if(robot_state.selected_hardware->type == HardwareType_Solenoid)
               {
                  DrawRectangle(&backbuffer, 1100, 20, 325, 400, V4(0.5f, 0.0f, 0.0f, 0.5f));
                  DrawText(&backbuffer, &font, V2(1120, 40), robot_state.selected_hardware->name, 20, context.font_memory);
                  
                  if(GUIButton(&context, input, RectPosSize(1105, 60, 100, 20), NULL, "Extend"))
                  {
                     u32 index = robot_state.selected_hardware->control_count++;
                     
                     robot_state.selected_hardware->solenoid[index].type = SolenoidControlType_Extend;
                     robot_state.selected_hardware->solenoid[index].controller_index = 0;
                     robot_state.selected_hardware->solenoid[index].button_index = 1;
                  }
                  
                  if(GUIButton(&context, input, RectPosSize(1210, 60, 100, 20), NULL, "Retract"))
                  {
                     u32 index = robot_state.selected_hardware->control_count++;
                     
                     robot_state.selected_hardware->solenoid[index].type = SolenoidControlType_Retract;
                     robot_state.selected_hardware->solenoid[index].controller_index = 0;
                     robot_state.selected_hardware->solenoid[index].button_index = 1;
                  }
                  
                  if(GUIButton(&context, input, RectPosSize(1315, 60, 100, 20), NULL, "Toggle"))
                  {
                     u32 index = robot_state.selected_hardware->control_count++;
                     
                     robot_state.selected_hardware->solenoid[index].type = SolenoidControlType_Toggle;
                     robot_state.selected_hardware->solenoid[index].controller_index = 0;
                     robot_state.selected_hardware->solenoid[index].button_index = 1;
                  }
                  
                  u32 extend_index = 0;
                  u32 retract_index = 0;
                  u32 toggle_index = 0;
                  for(u32 i = 0; i < robot_state.selected_hardware->control_count; i++)
                  {
                     u32 x = 0;
                     u32 y = 0;
                     
                     if(robot_state.selected_hardware->solenoid[i].type == SolenoidControlType_Extend)
                     {
                        x = 1105;
                        y = 95 + (extend_index++ * 25);
                     }
                     else if(robot_state.selected_hardware->solenoid[i].type == SolenoidControlType_Retract)
                     {
                        x = 1210;
                        y = 95 + (retract_index++ * 25);
                     }
                     else if(robot_state.selected_hardware->solenoid[i].type == SolenoidControlType_Toggle)
                     {
                        x = 1315;
                        y = 95 + (toggle_index++ * 25);
                     }
                     
                     if(robot_state.hardware_control_selected &&
                       (robot_state.selected_hardware_control == i))
                     {
                        DrawRectangle(&backbuffer, x - 5, y - 5, 110, 30, V4(0.5f, 0.0f, 0.0f, 0.5f));
                     }
                     
                     char controller_index_buffer[64];
                     char button_index_buffer[64];
                     char string_buffer[64];
                     
                     if(GUIButton(&context, input, RectPosSize(x, y, 100, 20), NULL,
                                  ConcatStrings(U32ToString(robot_state.selected_hardware->solenoid[i].controller_index, controller_index_buffer),
                                                " : ",
                                                U32ToString(robot_state.selected_hardware->solenoid[i].button_index, button_index_buffer),
                                                string_buffer)))
                     {
                        if(input.left_up)
                        {
                           robot_state.hardware_control_selected = true;
                           robot_state.selected_hardware_control = i;
                        }
                        else if(input.right_up)
                        {
                           for(u32 j = i + 1; j < robot_state.selected_hardware->control_count; j++)
                           {
                              robot_state.selected_hardware->solenoid[j - 1] = robot_state.selected_hardware->solenoid[j];
                           }
                           robot_state.selected_hardware->control_count--;
                           robot_state.hardware_control_selected = false;
                        }
                     }
                  }
                  
                  if(robot_state.hardware_control_selected)
                  {
                     DrawRectangle(&backbuffer, 1100, 425, 225, 225, V4(0.5f, 0.0f, 0.0f, 0.5f));
                     char index_buffer[32];
                     char string_buffer[64];
                     
                     if(GUIButton(&context, input, RectPosSize(1105, 450, 100, 20), NULL,
                        ConcatStrings("Controller: ",
                                      U32ToString(robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].controller_index, index_buffer),
                                      string_buffer)))
                     {
                        if(input.left_up && (robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].controller_index < 4))
                        {
                           robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].controller_index++;
                        }
                        else if(input.right_up && (robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].controller_index > 0))
                        {
                           robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].controller_index--;
                        }
                     }
                     
                     if(GUIButton(&context, input, RectPosSize(1105, 475, 100, 20), NULL,
                        ConcatStrings("Button: ",
                                      U32ToString(robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].button_index, index_buffer),
                                      string_buffer)))
                     {
                        if(input.left_up && (robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].button_index < 20))
                        {
                           robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].button_index++;
                        }
                        else if(input.right_up && (robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].button_index > 1))
                        {
                           robot_state.selected_hardware->solenoid[robot_state.selected_hardware_control].button_index--;
                        }
                     }
                     
                     if(GUIButton(&context, input, RectPosSize(1100, 425, 10, 10), NULL, NULL))
                     {
                        robot_state.hardware_control_selected = false;
                     }
                  }
               }
               
               if(GUIButton(&context, input, RectPosSize(1100, 20, 10, 10), NULL, NULL))
               {
                  robot_state.selected_hardware = NULL;
                  robot_state.hardware_control_selected = false;
               }
            }
         }
         else if(robot_state.robot_page == RobotPageType_Subsystems)
         {
            for(u32 i = 0; i < robot_state.robot_subsystems_count; i++)
            {
               if(robot_state.selected_subsystem == &robot_state.robot_subsystems[i])
               {
                  DrawRectangle(&backbuffer, 285, 
                                (25 + (30 * i)), 110, 30, V4(0.5f, 0.0f, 0.0f, 0.5f));
               }
               
               b32 hit = GUIButton(&context, input, RectPosSize(290, (30 + (30 * i)), 100, 20), NULL, robot_state.robot_subsystems[i].name);
               
               if(hit && input.left_up)
               {
                  robot_state.selected_subsystem = &robot_state.robot_subsystems[i];
               }
               else if(hit && input.right_up)
               {
                  for(u32 j = i + 1; j < robot_state.robot_subsystems_count; j++)
                  {
                     robot_state.robot_subsystems[j - 1] = robot_state.robot_subsystems[j];
                  }
                  robot_state.robot_subsystems_count--;
                  robot_state.selected_subsystem = NULL;
                  robot_state.subsystem_name_selected = false;
               }
            }
            
            if(GUIButton(&context, input, RectPosSize(1100, 20, 100, 20), NULL, "New Subsystem"))
            {
               robot_state.selected_subsystem = AddSubsystem((RobotSubsystem *)&robot_state.robot_subsystems, &robot_state.robot_subsystems_count);
            }
            
            if(robot_state.selected_subsystem)
            {
               DrawRectangle(&backbuffer, 1100, 45, 200, 400, V4(0.5f, 0.0f, 0.0f, 0.5f));
               
               if(GUIButton(&context, input, RectPosSize(1105, 60, 100, 20),
                            NULL, robot_state.selected_subsystem->name, robot_state.subsystem_name_selected))
               {
                  robot_state.subsystem_name_selected = !robot_state.subsystem_name_selected;
               }
               
               if(robot_state.subsystem_name_selected)
               {
                  u32 len = StringLength(robot_state.selected_subsystem->name);
                  
                  if(input.char_key_up && ((len + 1) < ArrayCount(robot_state.selected_subsystem->name)))
                  {
                     robot_state.selected_subsystem->name[len] = input.key_char;
                     robot_state.selected_subsystem->name[len + 1] = '\0';
                  }
                  else if(input.key_backspace && (len > 0))
                  {
                     robot_state.selected_subsystem->name[len - 1] = '\0';
                  }
               }
               
               if(GUIButton(&context, input, RectPosSize(1105, 95, 100, 20), NULL, GetNameFromType(robot_state.selected_subsystem->type)))
               {
                  robot_state.selected_subsystem->type = (SubsystemType)((u32)robot_state.selected_subsystem->type + 1);
                  if((u32)robot_state.selected_subsystem->type >= (u32)SubsystemType_Count) robot_state.selected_subsystem->type = (SubsystemType)0;
               }
               
               if(robot_state.selected_subsystem->type == SubsystemType_TankDrive2x2)
               {
                  char index_buffer[32];
                  char string_buffer[64];
                  
                  if(GUIButton(&context, input, RectPosSize(1105, 120, 100, 20), NULL,
                     ConcatStrings("Controller: ",
                                   U32ToString(robot_state.selected_subsystem->tank_drive2x2.controller_index, index_buffer),
                                   string_buffer)))
                  {
                     if(input.left_up && (robot_state.selected_subsystem->tank_drive2x2.controller_index < 4))
                     {
                        robot_state.selected_subsystem->tank_drive2x2.controller_index++;
                     }
                     else if(input.right_up && (robot_state.selected_subsystem->tank_drive2x2.controller_index > 0))
                     {
                        robot_state.selected_subsystem->tank_drive2x2.controller_index--;
                     }
                  }
                  
                  if(GUIButton(&context, input, RectPosSize(1210, 145, 10, 20), NULL, NULL,
                               robot_state.selected_subsystem->tank_drive2x2.invert_drive_axis))
                  {
                     robot_state.selected_subsystem->tank_drive2x2.invert_drive_axis = !robot_state.selected_subsystem->tank_drive2x2.invert_drive_axis;
                  }
                  
                  if(GUIButton(&context, input, RectPosSize(1105, 145, 100, 20), NULL,
                     ConcatStrings("Drive Axis: ",
                                   U32ToString(robot_state.selected_subsystem->tank_drive2x2.drive_axis_index, index_buffer),
                                   string_buffer)))
                  {
                     if(input.left_up && (robot_state.selected_subsystem->tank_drive2x2.drive_axis_index < 20))
                     {
                        robot_state.selected_subsystem->tank_drive2x2.drive_axis_index++;
                     }
                     else if(input.right_up && (robot_state.selected_subsystem->tank_drive2x2.drive_axis_index > 0))
                     {
                        robot_state.selected_subsystem->tank_drive2x2.drive_axis_index--;
                     }
                  }
                  
                  if(GUIButton(&context, input, RectPosSize(1210, 170, 10, 20), NULL, NULL,
                               robot_state.selected_subsystem->tank_drive2x2.invert_rotate_axis))
                  {
                     robot_state.selected_subsystem->tank_drive2x2.invert_rotate_axis = !robot_state.selected_subsystem->tank_drive2x2.invert_rotate_axis;
                  }
                  
                  if(GUIButton(&context, input, RectPosSize(1105, 170, 100, 20), NULL,
                     ConcatStrings("Rotate Axis: ",
                                   U32ToString(robot_state.selected_subsystem->tank_drive2x2.rotate_axis_index, index_buffer),
                                   string_buffer)))
                  {
                     if(input.left_up && (robot_state.selected_subsystem->tank_drive2x2.rotate_axis_index < 20))
                     {
                        robot_state.selected_subsystem->tank_drive2x2.rotate_axis_index++;
                     }
                     else if(input.right_up && (robot_state.selected_subsystem->tank_drive2x2.rotate_axis_index > 0))
                     {
                        robot_state.selected_subsystem->tank_drive2x2.rotate_axis_index--;
                     }
                  }
               }
               
               if(GUIButton(&context, input, RectPosSize(1100, 45, 10, 10), NULL, NULL))
               {
                  robot_state.selected_subsystem = NULL;
                  robot_state.subsystem_name_selected = false;
               }
            }
         }
      }
      
      //DEBUG
      if(page == PageType_Home)
      {
         char frame_time_buffer[64];
         DrawText(&backbuffer, &font, V2(600, 40), R64ToString(frame_length, frame_time_buffer), 20, context.font_memory);
      }
      
      BlitToScreen(&backbuffer, device_context, window);
      
      frame_length = GetCounter(&last_timer, timer_freq);
      if(frame_length < 33.3)
      {
         Sleep(33.3 - frame_length);
      }
   }
   
   shutdown(server_socket, SD_BOTH);
   closesocket(server_socket);
   WSACleanup();
   
   return 0;
}