#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>

#include "dashboard.h"
#include "packet_definitions.h"
#include "dashboard.cpp"
#include "network_win32.cpp"

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

//TODO: make a "FreeEntireFile"
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

void SetFullscreen(b32 state)
{
   //TODO: fix this
   /*
   if(state)
   {
      DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
      DWORD dwRemove = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
      DWORD dwNewStyle = dwStyle & ~dwRemove;
      
      SetWindowLong(window, GWL_STYLE, dwNewStyle);
      SetWindowPos(window, NULL, 0, 0, 0, 0,
                   SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                 
      SetWindowPos(window, NULL, 0, 0, 
                   GetDeviceCaps(device_context, HORZRES), window_size.y, SWP_FRAMECHANGED);
   }
   else
   {
      SetWindowLong(window, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
      SetWindowPos(window, NULL, 0, 0, window_size.x, window_size.y, SWP_FRAMECHANGED);
   }
   */
}

void UpdateInputState(InputState *input, HWND window, b32 update_mouse)
{
   if(update_mouse)
   {
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(window, &p);
   
      input->pos.x = p.x;
      input->pos.y = p.y;
   
      input->left_up = false;
      input->right_up = false;
   }
   
   input->char_key_up = false;
   input->key_backspace = false;
   input->key_enter = false;
   input->key_up = false;
   input->key_down = false;
   input->key_left = false;
   input->key_right = false;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{  
   WNDCLASSEX window_class = {};
   
   window_class.cbSize = sizeof(window_class);
   window_class.style = CS_OWNDC;
   window_class.lpfnWndProc = WindowMessageEvent;
   window_class.hInstance = hInstance;
   window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
   window_class.lpszClassName = "WindowClass";
   
   RegisterClassExA(&window_class);
   
   HWND window =  window = CreateWindowExA(WS_EX_CLIENTEDGE, "WindowClass", "Dashboard V-I Lost Count",
                                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                           1440, 759, NULL, NULL,
                                           hInstance, NULL);
   
   HANDLE hIcon = LoadImageA(0, "icon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
   if(hIcon)
   {
       SendMessageA(window, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
       SendMessageA(window, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

       SendMessageA(GetWindow(window, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
       SendMessageA(GetWindow(window, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM) hIcon);
   }
   else
   {
      MessageBox(window, "Error", "Icon Not Found", 0);
   }
   
   HDC device_context = GetDC(window);
   
   PIXELFORMATDESCRIPTOR pixel_format = {sizeof(pixel_format), 1};
   pixel_format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   pixel_format.iPixelType = PFD_TYPE_RGBA;
   pixel_format.cColorBits = 32;

   s32 pixel_format_index = ChoosePixelFormat(device_context, &pixel_format);
   SetPixelFormat(device_context, pixel_format_index, &pixel_format);

   HGLRC gl_context = wglCreateContext(device_context);
   wglMakeCurrent(device_context, gl_context);
   
   UIAssets ui_assets = {};
   ui_assets.logo = LoadImage("logo.bmp");
   ui_assets.home = LoadImage("home.bmp");
   ui_assets.gear = LoadImage("gear.bmp");
   ui_assets.eraser = LoadImage("eraser.bmp");
   ui_assets.competition = LoadImage("competition.bmp");
   
   InputState input = {};
   b32 fullscreen = false;
   b32 connected = false;
   r64 timer_freq = GetCounterFrequency();
   s64 last_timer = 0;
   r64 frame_length = 0.0;
   
   WSADATA winsock_data = {};
   WSAStartup(MAKEWORD(2, 2), &winsock_data);

   //TODO: move state initialization out of the platform layer & organize it
   MemoryArena generic_arena = {};
   InitMemoryArena(&generic_arena, VirtualAlloc(0, Megabyte(64), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE), Megabyte(64));
   
   RenderContext context = InitRenderContext(&generic_arena, Megabyte(12));
   
   UIContext ui_context = {};
   ui_context.render_context = &context;
   ui_context.assets = &ui_assets;
   
   RobotHardware test_hardware[] = 
   {
      {Hardware_Motor, 0, Literal("Test Motor")},
      {Hardware_Solenoid, 1, Literal("Test Solenoid")},
      {Hardware_Drive, 2, Literal("Test Drive")}
   };
   
   DashboardState dashstate = {};
   
   dashstate.robot.hardware = test_hardware;
   dashstate.robot.hardware_count = ArrayCount(test_hardware);
   dashstate.robot.name = Literal("Bob Ross");
   
   dashstate.net_settings.connect_to = String((char *) malloc(sizeof(char) * 30), 30);
   Clear(dashstate.net_settings.connect_to);
   CopyTo(Literal("roborio-4618-frc.local") /*Literal("chimera.local")*/, dashstate.net_settings.connect_to);
   dashstate.net_settings.is_mdns = true;
   
   volatile b32 running = true;
   volatile char *connect_to = NULL;
   volatile b32 use_mdns = false;
   volatile b32 reconnect = false;
   volatile b32 connect_status = false;
   
   net_main_params net_params = {};
   net_params.running = &running;
   net_params.connect_to = connect_to;
   net_params.use_mdns = &use_mdns;
   net_params.reconnect = &reconnect;
   net_params.connect_status = &connect_status;
   net_params.dashstate = &dashstate;
   
   HANDLE network_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)NetMain, &net_params, 0, NULL);
   
   RequestReconnect(&dashstate.net_settings, &net_params);
   
   Notification *vision_notification =
      AddNotification((Console *) &dashstate.console, Literal("Vision"));
   
   AddMessage((Console *) &dashstate.console,
              Literal("Vision System Not Present XD"), vision_notification);
   
   b32 update_mouse = true;
   
   ShowWindow(window, nCmdShow);
   UpdateWindow(window);
   
   //connected = HandleNetwork(server_socket, &auto_builder_state, &robot_state, &console_state);
   GetCounter(&last_timer, timer_freq);
   while(running)
   {
      UpdateInputState(&input, window, update_mouse);
      
      MSG msg = {};
      while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
      {
         switch(msg.message)
         {            
            case WM_LBUTTONUP:
               if(update_mouse)
               {
                  input.left_down = false;
                  input.left_up = true;
               }
               break;
         
            case WM_LBUTTONDOWN:
               if(update_mouse)
               {
                  input.left_down = true;
                  input.left_up = false;
               }
               break;
               
            case WM_RBUTTONUP:
               if(update_mouse)
               {
                  input.right_down = false;
                  input.right_up = true;
               }
               break;
         
            case WM_RBUTTONDOWN:
               if(update_mouse)
               {
                  input.right_down = true;
                  input.right_up = false;
               }
               break;   
            
            case WM_KEYUP:
            {
               if(msg.wParam == VK_BACK)
               {
                  input.key_backspace = true;
               }
               else if (msg.wParam == VK_RETURN)
               {
                  input.key_enter = true;
               }
               else if(msg.wParam == VK_UP)
               {
                  input.key_up = true;
               }
               else if(msg.wParam == VK_DOWN)
               {
                  input.key_down = true;
               }
               else if(msg.wParam == VK_LEFT)
               {
                  input.key_left = true;
               }
               else if(msg.wParam == VK_RIGHT)
               {
                  input.key_right = true;
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
                  
                  if(c == 'm')
                  {
                     update_mouse = !update_mouse;
                  }
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
      
      RECT client_rect = {};
      GetClientRect(window, &client_rect);
      v2 window_size = V2(client_rect.right, client_rect.bottom);
      
      ui_context.window_size = window_size;
      ui_context.input_state = input;
      
      DrawDashboardUI(&ui_context, &dashstate);
     
      char frame_time_buffer[64];
      Rectangle(&context, RectMinSize(V2(200, 40), V2(200, 20)), V4(0, 0, 0, 0.5));
      Text(&context, V2(200, 40), Literal(R64ToString(frame_length, frame_time_buffer)), 20);

      char ui_time_buffer[64];
      Rectangle(&context, RectMinSize(V2(200, 80), V2(200, 20)), V4(0, 0, 0, 0.5));
      Text(&context, V2(200, 80), Literal(R64ToString(ui_context.curr_time, frame_time_buffer)), 20);
      
      RenderUI(&context, window_size);
      SwapBuffers(device_context);
      
      //connected = HandleNetwork(server_socket, &auto_builder_state, &robot_state, &console_state);

      frame_length = GetCounter(&last_timer, timer_freq);
      ui_context.curr_time += frame_length / 1000.0f;
      if(frame_length < 33.3)
      {
         Sleep(33.3 - frame_length);
      }
   }
   
   /*
   shutdown(server_socket, SD_BOTH);
   closesocket(server_socket);
   */
   WSACleanup();
 
   WaitForSingleObject(network_thread, INFINITE);

   return 0;
}