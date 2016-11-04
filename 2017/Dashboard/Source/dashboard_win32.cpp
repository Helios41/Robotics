#include "dashboard.h"

#include "network_win32.cpp"
#include <windows.h>

#include "packet_definitions.h"
#include "dashboard.cpp"

enum PageType
{
   PageType_Home,
   PageType_Auto,
   PageType_Robot,
   PageType_Console
};

//TODO: conditionals for sensors
enum AutoBlockType
{
   AutoBlockType_Motor,
   AutoBlockType_Solenoid,
   AutoBlockType_Drive
};

enum SolenoidState
{
   SolenoidState_Extended,
   SolenoidState_Retracted,
   SolenoidState_Stopped
};

struct MotorState
{
   r32 value;
   r32 time;
};

struct DriveState
{
	r32 forward_value;
	r32 rotate_value;
	r32 time;
};

struct AutoBlockPreset
{
   char name[16];
   u32 hwid;
   AutoBlockType type;
};

struct AutoBlock
{
   char name[16];
   u32 hwid;
   AutoBlockType type;
   
   union
   {
      MotorState motor;
      SolenoidState solenoid;
	  DriveState drive;
   };
};

enum HardwareType
{
   HardwareType_MotorController,
   HardwareType_Solenoid,
   HardwareType_FloatSensor,
   HardwareType_BoolSensor,
   HardwareType_Drive,
   HardwareType_Camera
};

struct RobotHardware
{
   char name[16];
   u32 hwid;
   HardwareType type;
   
   bool got_first_recorded_update;
   float last_recorded_update_time;

   union
   {
	   SolenoidState solenoid_state;
	   r32 motor_value;
	   r32 float_sensor_value;
	   b32 bool_sensor_value;

	   struct
	   {
		   r32 forward_value;
		   r32 rotate_value;
	   };

	   LoadedBitmap *camera_frame;
   };
};

struct AutoBuilderState
{
   AutoBlock auto_blocks[64];
   u32 auto_block_count;
   
   AutoBlockPreset auto_block_presets[32]; 
   u32 auto_block_preset_count;
   
   AutoBlock *selected_block;
   
   char auto_file_name[32];
   b32 auto_file_name_selected;

   b32 auto_file_selector_open;
   b32 recording_robot;

   char text_box1_buffer[16];
   b32 text_box1_selected;

   char text_box2_buffer[16];
   b32 text_box2_selected;

   char text_box3_buffer[16];
   b32 text_box3_selected;
};

struct RobotState
{
   b32 connected;
   char name[32];
   
   b32 has_autonomous;
   char robot_auto_name[32];

   RobotHardware robot_hardware[32];
   u32 robot_hardware_count;
      
   RobotHardware *selected_hardware;
};

enum ConsoleMessageType
{
	ConsoleMessageType_Invalid,
	ConsoleMessageType_ClientMessage,
	ConsoleMessageType_ServerMessage,
	ConsoleMessageType_ClientPacket,
	ConsoleMessageType_ServerPacket,
	ConsoleMessageType_HardwareUpdate
};

struct ConsoleMessage
{
	ConsoleMessageType type;
	char message[64];
};

struct ConsoleState
{
	ConsoleMessage messages[25];
	int message_index;
	bool track_hardware;
};

#pragma pack(push, 1)
struct AutoFileHeader
{
   u32 identifier;
   u32 auto_block_count;
};
#pragma pack(pop)

//#define ENABLE_TESTING_HARDWARE
//#define CAMERA_ENABLED

//#define SERVER_ADDR "10.46.18.11"
#define SERVER_ADDR "10.46.18.21"
//#define SERVER_ADDR "169.254.71.73"
#define SERVER_PORT 8089

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

/*
b32 GUIButton(RenderContext *context, InputState input, rect2 bounds, LoadedBitmap *icon, char *text)
{
   b32 hot = Contains(bounds, input.pos);
   
   if(hot && (input.left_down || input.right_down))
   {
	  Rectangle(context, bounds, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(hot)
   {
	  Rectangle(context, bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
	  Rectangle(context, bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   v2 bounds_size = GetSize(bounds);
   
   if(bounds_size.x == bounds_size.y)
   {
	  Bitmap(context, icon, bounds.min);
      //Text(context,V2(bounds.min.x + 10, bounds.min.y + 10), text, 20);
   }
   else
   {
	  Bitmap(context, icon, V2(bounds.min.x + 5, bounds.min.y));
      //Text(context, V2(bounds.min.x + 10, bounds.min.y + 10), text, 20);
   }
   
   return hot && (input.left_up || input.right_up);
}

b32 GUIButton(RenderContext *context, InputState input, rect2 bounds, LoadedBitmap *icon, char *text, b32 triggered)
{
   v2 bounds_size = GetSize(bounds);
   b32 result = GUIButton(context, input, bounds, icon, text);
   
   Rectangle(context, RectPosSize(bounds.min.x, bounds.min.y, 5,
	         (bounds_size.x == bounds_size.y) ? 5 : bounds_size.y),
	         triggered ? V4(0.0f, 0.0f, 0.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 0.0f));

   return result;
}
*/

b32 AutoBuilderBlock(RenderContext *context, AutoBlock block, v2 pos, InputState input)
{
   b32 hot = Contains(RectPosSize(pos.x, pos.y, 100, 20), input.pos);
   
   if(hot && (input.left_down || input.right_down))
   {
	  Rectangle(context, RectPosSize(pos.x - 2, pos.y - 2, 104, 24), V4(0.1f, 0.1f, 0.1f, 1.0f));
   }
   else if(hot)
   {
	  Rectangle(context, RectPosSize(pos.x - 2, pos.y - 2, 104, 24), V4(0.25f, 0.25f, 0.25f, 1.0f));
   }
   
   Rectangle(context, RectPosSize(pos.x, pos.y, 100, 20), V4(0.5f, 0.0f, 0.0f, 1.0f));
   //Text(context, V2(pos.x, pos.y), block.name, 20);
   
   return hot && (input.left_up || input.right_up);
}

/*
void TextBox(RenderContext *context, InputState input, rect2 bounds, char *text_buffer, u32 buffer_size, b32 *active)
{
	if (*active)
	{
		u32 len = StringLength(text_buffer);

		if (input.char_key_up && ((len + 1) < buffer_size))
		{
			text_buffer[len] = input.key_char;
			text_buffer[len + 1] = '\0';
		}
		else if (input.key_backspace && (len > 0))
		{
			text_buffer[len - 1] = '\0';
		}
	}

	v2 bounds_size = GetSize(bounds);
	if (GUIButton(context, input, bounds, NULL, text_buffer))
	{
		*active = !(*active);
	}

	Rectangle(context, RectPosSize(bounds.min.x, bounds.min.y, 5,
	       	  (bounds_size.x == bounds_size.y) ? 5 : bounds_size.y),
		      (*active) ? V4(0.0f, 0.0f, 0.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 0.0f));

}
*/

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

u32 GetHardwareIndex(RobotState *robot_state, u32 hw_id, u32 hw_type)
{
	for (u32 i = 0; i < robot_state->robot_hardware_count; i++)
	{
		bool correct_type = (((hw_type == HARDWARE_TYPE_MOTOR_CONTROLLER) && (robot_state->robot_hardware[i].type == HardwareType_MotorController)) ||
							 ((hw_type == HARDWARE_TYPE_SOLENOID) && (robot_state->robot_hardware[i].type == HardwareType_Solenoid)) ||
							 ((hw_type == HARDWARE_TYPE_FLOAT_SENSOR) && (robot_state->robot_hardware[i].type == HardwareType_FloatSensor)) ||
							 ((hw_type == HARDWARE_TYPE_BOOL_SENSOR) && (robot_state->robot_hardware[i].type == HardwareType_BoolSensor)) ||
							 ((hw_type == HARDWARE_TYPE_DRIVE) && (robot_state->robot_hardware[i].type == HardwareType_Drive)) ||
							 ((hw_type == HARDWARE_TYPE_CAMERA) && (robot_state->robot_hardware[i].type == HardwareType_Camera)));

		if ((robot_state->robot_hardware[i].hwid == hw_id) && correct_type)
		{
			return i;
		}
	}

	char hw_index_buffer[32];
	char error_message_buffer[512];
	MessageBoxA(NULL, ConcatStrings("Invalid Hardware Index: ", U32ToString(hw_id, hw_index_buffer), error_message_buffer), "Net Error", MB_OK);

	return 0;
}

void SendAutonomousPacket(SOCKET socket_id, AutoBuilderState *auto_builder_state, MemoryArena *mem_arena)
{
	u32 packet_size = sizeof(send_autonomous_header) + (auto_builder_state->auto_block_count * sizeof(auto_operation));
	void *buffer = PushSize(mem_arena, packet_size);

	send_autonomous_header *header = (send_autonomous_header *)buffer;
	auto_operation *operations = (auto_operation *)(header + 1);

	header->header.packet_size = packet_size;
	header->header.packet_type = PACKET_TYPE_SEND_AUTONOMOUS;
	header->auto_operation_count = auto_builder_state->auto_block_count;
	StringCopy(auto_builder_state->auto_file_name, header->name);

	u32 auto_op_count = 0;
	for (u32 i = 0; i < auto_builder_state->auto_block_count; i++)
	{
		u32 auto_op_index = 0;
		if ((auto_builder_state->auto_blocks[i].type == AutoBlockType_Motor) ||
			(auto_builder_state->auto_blocks[i].type == AutoBlockType_Solenoid) ||
			(auto_builder_state->auto_blocks[i].type == AutoBlockType_Drive))
		{
			auto_op_index = auto_op_count++;
			operations[auto_op_index].hw_id = auto_builder_state->auto_blocks[i].hwid;
		}

		if (auto_builder_state->auto_blocks[i].type == AutoBlockType_Motor)
		{
			operations[auto_op_index].hw_type = HARDWARE_TYPE_MOTOR_CONTROLLER;
			operations[auto_op_index].motor_value = auto_builder_state->auto_blocks[i].motor.value;
			operations[auto_op_index].time = auto_builder_state->auto_blocks[i].motor.time;
		}
		else if (auto_builder_state->auto_blocks[i].type == AutoBlockType_Solenoid)
		{
			operations[auto_op_index].hw_type = HARDWARE_TYPE_SOLENOID;
			operations[auto_op_index].time = 0.0f;
			
			switch (auto_builder_state->auto_blocks[i].solenoid)
			{
				case SolenoidState_Extended:
					operations[auto_op_index].solenoid_value = SOLENOID_STATE_EXTENDED;
					break;

				case SolenoidState_Retracted:
					operations[auto_op_index].solenoid_value = SOLENOID_STATE_RETRACTED;
					break;

				case SolenoidState_Stopped:
					operations[auto_op_index].solenoid_value = SOLENOID_STATE_STOPPED;
					break;
			}
		}
		else if(auto_builder_state->auto_blocks[i].type == AutoBlockType_Drive)
		{
			operations[auto_op_index].hw_type = HARDWARE_TYPE_DRIVE;
			operations[auto_op_index].forward_value = auto_builder_state->auto_blocks[i].drive.forward_value;
			operations[auto_op_index].rotate_value = auto_builder_state->auto_blocks[i].drive.rotate_value;
			operations[auto_op_index].time = auto_builder_state->auto_blocks[i].drive.time;
		}
	}

	send(socket_id, (char *)buffer, packet_size, 0);
	PopSize(mem_arena, packet_size);
}

void PutConsoleMessage(ConsoleState *console_state, ConsoleMessageType type, char *message)
{
	u32 message_index = 0;
	if ((console_state->message_index + 1) > ArrayCount(console_state->messages))
	{
		console_state->message_index = 0;
	}
	else
	{
		message_index = console_state->message_index++;
	}

	console_state->messages[message_index].type = type;
	StringCopy(message, console_state->messages[message_index].message);
}

void HandleHardwareUpdate(RobotState *robot_state, AutoBuilderState *auto_builder_state, ConsoleState *console_state, hardware_update_header *header, u32 packet_size)
{
	u32 hardware_index = GetHardwareIndex(robot_state, header->hw_id, header->hw_type);

	if (console_state->track_hardware)
	{
		PutConsoleMessage(console_state, ConsoleMessageType_HardwareUpdate, robot_state->robot_hardware[hardware_index].name);
	}

	if (auto_builder_state->recording_robot)
	{
		if ((header->hw_type == HARDWARE_TYPE_MOTOR_CONTROLLER) ||
			(header->hw_type == HARDWARE_TYPE_SOLENOID) ||
			(header->hw_type == HARDWARE_TYPE_DRIVE))
		{
			robot_state->robot_hardware[hardware_index].got_first_recorded_update = true;

			AutoBlockPreset preset = {};
			for (u32 i = 0; i < auto_builder_state->auto_block_preset_count; i++)
			{
				if (auto_builder_state->auto_block_presets[i].hwid == header->hw_id)
				{
					preset = auto_builder_state->auto_block_presets[i];
				}
			}

			AutoBlock *block = AddAutoBlock(auto_builder_state->auto_blocks, &auto_builder_state->auto_block_count, preset);
			
			if (block->type == AutoBlockType_Motor)
			{
				block->motor.value = header->motor_value;
				block->motor.time = 0.0f; //TODO
			}
			else if (block->type == AutoBlockType_Solenoid)
			{
				if(header->solenoid_value == SOLENOID_STATE_EXTENDED)
				{
					block->solenoid = SolenoidState_Extended;
				}
				else if (header->solenoid_value == SOLENOID_STATE_RETRACTED)
				{
					block->solenoid = SolenoidState_Retracted;
				}
				else if (header->solenoid_value == SOLENOID_STATE_STOPPED)
				{
					block->solenoid = SolenoidState_Stopped;
				}
			}
			else if (block->type == AutoBlockType_Drive)
			{
				block->drive.forward_value = header->forward_value;
				block->drive.rotate_value = header->rotate_value;
				block->drive.time = 0.0f; //TODO
			}
		}
	}

	if (header->hw_type == HARDWARE_TYPE_MOTOR_CONTROLLER)
	{
		robot_state->robot_hardware[hardware_index].motor_value = header->motor_value;
	}
	else if (header->hw_type == HARDWARE_TYPE_SOLENOID)
	{
		if (header->solenoid_value == SOLENOID_STATE_EXTENDED)
		{
			robot_state->robot_hardware[hardware_index].solenoid_state = SolenoidState_Extended;
		}
		else if (header->solenoid_value == SOLENOID_STATE_RETRACTED)
		{
			robot_state->robot_hardware[hardware_index].solenoid_state = SolenoidState_Retracted;
		}
		else if (header->solenoid_value == SOLENOID_STATE_STOPPED)
		{
			robot_state->robot_hardware[hardware_index].solenoid_state = SolenoidState_Stopped;
		}
	}
	else if (header->hw_type == HARDWARE_TYPE_FLOAT_SENSOR)
	{
		robot_state->robot_hardware[hardware_index].float_sensor_value = header->float_sensor_value;
	}
	else if (header->hw_type == HARDWARE_TYPE_BOOL_SENSOR)
	{
		robot_state->robot_hardware[hardware_index].bool_sensor_value = header->bool_sensor_value;
	}
	else if (header->hw_type == HARDWARE_TYPE_DRIVE)
	{
		robot_state->robot_hardware[hardware_index].forward_value = header->forward_value;
		robot_state->robot_hardware[hardware_index].rotate_value = header->rotate_value;
	}
	else if (header->hw_type == HARDWARE_TYPE_CAMERA)
	{
		//JPEGLoadedBitmap((void *)(header + 1), (packet_size - sizeof(hardware_update_header)), robot_state->robot_hardware[hardware_index].camera_frame);
	}
}

void HandlePacket(SOCKET socket_id, AutoBuilderState *auto_builder_state, RobotState *robot_state, ConsoleState *console_state, u8 *buffer, s32 buffer_size)
{
	generic_packet_header *generic_header = (generic_packet_header *)buffer;

	if (generic_header->packet_type == PACKET_TYPE_HARDWARE_DETAILS)
	{
		hardware_details_header *hardware_details = (hardware_details_header *)generic_header;
		hardware_component *hardware_components = (hardware_component *)(hardware_details + 1);

		u32 auto_block_count = 0;
		for (u32 i = 0; i < hardware_details->hw_count; i++)
		{
			u32 auto_block_index = 0;
			if ((hardware_components[i].hw_type == HARDWARE_TYPE_MOTOR_CONTROLLER) ||
				(hardware_components[i].hw_type == HARDWARE_TYPE_SOLENOID) ||
				(hardware_components[i].hw_type == HARDWARE_TYPE_DRIVE))
			{
				auto_block_index = auto_block_count++;
				auto_builder_state->auto_block_presets[auto_block_index].hwid = hardware_components[i].hw_id;
				StringCopy(hardware_components[i].name, auto_builder_state->auto_block_presets[auto_block_index].name);
			}

			robot_state->robot_hardware[i].hwid = hardware_components[i].hw_id;
			StringCopy(hardware_components[i].name, robot_state->robot_hardware[i].name);

			if (hardware_components[i].hw_type == HARDWARE_TYPE_MOTOR_CONTROLLER)
			{
				auto_builder_state->auto_block_presets[auto_block_index].type = AutoBlockType_Motor;
				robot_state->robot_hardware[i].type = HardwareType_MotorController;
			}
			else if (hardware_components[i].hw_type == HARDWARE_TYPE_SOLENOID)
			{
				auto_builder_state->auto_block_presets[auto_block_index].type = AutoBlockType_Solenoid;
				robot_state->robot_hardware[i].type = HardwareType_Solenoid;
			}
			else if (hardware_components[i].hw_type == HARDWARE_TYPE_FLOAT_SENSOR)
			{
				robot_state->robot_hardware[i].type = HardwareType_FloatSensor;
			}
			else if (hardware_components[i].hw_type == HARDWARE_TYPE_BOOL_SENSOR)
			{
				robot_state->robot_hardware[i].type = HardwareType_BoolSensor;
			}
			else if (hardware_components[i].hw_type == HARDWARE_TYPE_DRIVE)
			{
				auto_builder_state->auto_block_presets[auto_block_index].type = AutoBlockType_Drive;
				robot_state->robot_hardware[i].type = HardwareType_Drive;
			}
			else if (hardware_components[i].hw_type == HARDWARE_TYPE_CAMERA)
			{
            /*
				robot_state->robot_hardware[i].type = HardwareType_Camera;
				robot_state->robot_hardware[i].camera_frame = new LoadedBitmap; //TODO remove new

				robot_state->robot_hardware[i].camera_frame->height = 350;
				robot_state->robot_hardware[i].camera_frame->width = 425;
				robot_state->robot_hardware[i].camera_frame->pixels = (u32 *) malloc(350 * 425 * sizeof(u32)); //TODO remove malloc
				robot_state->robot_hardware[i].camera_frame->gl_texture = 0;

				DrawRectangle(robot_state->robot_hardware[i].camera_frame, 0, 0, 
							  robot_state->robot_hardware[i].camera_frame->height,
							  robot_state->robot_hardware[i].camera_frame->width,
							  V4(0.0f, 0.0f, 0.0f, 1.0f));
            */
			}
		}

		auto_builder_state->auto_block_preset_count = auto_block_count;
		robot_state->robot_hardware_count = hardware_details->hw_count;
	}
	else if (generic_header->packet_type == PACKET_TYPE_HARDWARE_UPDATE)
	{
		hardware_update_header *hardware_update = (hardware_update_header *)generic_header;
		HandleHardwareUpdate(robot_state, auto_builder_state, console_state, hardware_update, buffer_size);
	}
	else if (generic_header->packet_type == PACKET_TYPE_WELCOME)
	{
		welcome_header *welcome = (welcome_header *)generic_header;
		robot_state->connected = true;
		StringCopy(welcome->name, robot_state->name);
	}
	else if (generic_header->packet_type == PACKET_TYPE_DEBUG)
	{
		debug_header *dbg = (debug_header *)generic_header;
		ConsoleMessageType message_type = ConsoleMessageType_ServerMessage;

		if (dbg->type == DEBUG_TYPE_MESSAGE)
		{
			message_type = ConsoleMessageType_ServerMessage;
		}
		else if (dbg->type == DEBUG_TYPE_PACKET)
		{
			message_type = ConsoleMessageType_ServerPacket;
		}

		PutConsoleMessage(console_state, message_type, dbg->text);
	}
	else if (generic_header->packet_type == PACKET_TYPE_RETURN_AUTONOMOUS_STATE)
	{
		return_autonomous_state_header *header = (return_autonomous_state_header *)generic_header;
		robot_state->has_autonomous = header->has_autonomous;
		StringCopy(header->name, robot_state->robot_auto_name);
	}
}

b32 HandleNetwork(SOCKET socket_id, AutoBuilderState *auto_builder_state, RobotState *robot_state, ConsoleState *console_state)
{
	b32 connected = false;
	b32 had_packet = true;

	while (had_packet)
	{
		u8 packet_buffer[4000];
		s32 data_length = recv(socket_id, (char *)packet_buffer, 4000, 0);
		int error_code = WSAGetLastError();
		connected = ((error_code == 0) || (error_code == WSAEWOULDBLOCK));
		had_packet = (data_length != SOCKET_ERROR);

		if (connected && (data_length != SOCKET_ERROR))
		{
			u8 *buffer = packet_buffer;
			s32 buffer_size = data_length;
			
			while (buffer_size > 0)
			{
				generic_packet_header *packet_header = (generic_packet_header *)buffer;
				
				if (packet_header->packet_size <= buffer_size)
					HandlePacket(socket_id, auto_builder_state, robot_state, console_state, buffer, data_length);

				buffer += packet_header->packet_size;
				buffer_size -= packet_header->packet_size;
			}
		}
	}

	return connected;
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

HDC SetupWindow(HINSTANCE hInstance, int nCmdShow, HWND *window)
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
   
   ShowWindow(*window, nCmdShow);
   UpdateWindow(*window);
   
   return GetDC(*window);
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
   }
   
   input->left_up = false;
   input->right_up = false;
   
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
   
   volatile b32 running = true;
   volatile b32 reconnect = true;

   net_main_params net_params = {};
   net_params.running = &running;
   net_params.reconnect = &reconnect;

   HANDLE network_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)NetMain, &net_params, 0, NULL);

   HWND window = {};
   HDC device_context = SetupWindow(hInstance, nCmdShow, &window);
   
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
   PageType page = PageType_Home;
   MSG msg = {};
   r64 timer_freq = GetCounterFrequency();
   s64 last_timer = 0;
   r64 frame_length = 0.0;
   
   WSADATA winsock_data = {};
   WSAStartup(MAKEWORD(2, 2), &winsock_data);

   SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   b32 is_nonblocking = true;
   ioctlsocket(server_socket, FIONBIO, (u_long *)&is_nonblocking);

   struct sockaddr_in server;
   server.sin_family = AF_INET;
   
   //server.sin_addr.s_addr = inet_addr(robot_addresses[0]);
   server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
   server.sin_port = htons(SERVER_PORT);
   
   connect(server_socket, (struct sockaddr *)&server, sizeof(server));

   MemoryArena generic_arena = {};
   InitMemoryArena(&generic_arena, VirtualAlloc(0, Megabyte(64), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE), Megabyte(64));
   
   RenderContext context = InitRenderContext(&generic_arena, Megabyte(12));
   
   AutoBuilderState auto_builder_state = {};
   StringCopy("unnamed", auto_builder_state.auto_file_name);
   
   RobotState robot_state = {};
   ConsoleState console_state = {};
   
   UIContext ui_context = {};
   ui_context.render_context = &context;
   ui_context.assets = &ui_assets;
   
   AutonomousBlock test_blocks[] = 
   {
      {Literal("Test1")},
      {Literal("Test2")}
   };
   
   DashboardState dashstate = {};
   dashstate.auto_editor.selector_blocks = test_blocks;
   dashstate.auto_editor.selector_block_count = ArrayCount(test_blocks);
   
   b32 update_mouse = true;
   
   connected = HandleNetwork(server_socket, &auto_builder_state, &robot_state, &console_state);
   GetCounter(&last_timer, timer_freq);
   while(running)
   {
      UpdateInputState(&input, window, update_mouse);
      
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
      
      //ui_context.hot_element = NULL_UI_ID;
      //ui_context.active_element = NULL_UI_ID;
      
      DrawDashboardUI(&ui_context, &dashstate);
      
      /*
      if(page == PageType_Home)
      {
         
      }
      else if(page == PageType_Auto)
      {
         rect2 sandbox_bounds = RectPosSize(280, 20, 800, 675);
         Rectangle(&context, sandbox_bounds, V4(0.5f, 0.0f, 0.0f, 0.5f));
         r32 total_time = 0.0f;

         for(u32 i = 0; i < auto_builder_state.auto_block_count; i++)
         {
            if(auto_builder_state.selected_block == &auto_builder_state.auto_blocks[i])
            {
               Rectangle(&context, RectPosSize((sandbox_bounds.min.x + 5), 
                         (sandbox_bounds.min.y + 5 + (30 * i)), 110, 30), V4(0.5f, 0.0f, 0.0f, 0.5f));
            }
			
            b32 hit = AutoBuilderBlock(&context, auto_builder_state.auto_blocks[i],
                                       V2(sandbox_bounds.min.x + 10, sandbox_bounds.min.y + 10 + (30 * i)),
                                       input);
			
			if (auto_builder_state.auto_blocks[i].type == AutoBlockType_Motor)
			{
				total_time += auto_builder_state.auto_blocks[i].motor.time;
			}
			else if (auto_builder_state.auto_blocks[i].type == AutoBlockType_Drive)
			{
				total_time += auto_builder_state.auto_blocks[i].drive.time;
			}

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

		 Rectangle(&context, RectPosSize(240, 20, 20, 20), (robot_state.has_autonomous ? V4(0.0f, 1.0f, 0.0f, 1.0f) : V4(1.0f, 0.0f, 0.0f, 1.0f)));
		 //Text(&context, V2(280, 30), robot_state.robot_auto_name, 20);

		 char total_time_buffer[32];
		 GUIButton(&context, input, RectPosSize(160, 20, 70, 20), NULL, R32ToString(total_time, total_time_buffer));

         for(u32 i = 0;
             i < auto_builder_state.auto_block_preset_count;
             ++i)
         {
            b32 hit = GUIButton(&context, input, RectPosSize(160, (50 + (i * 25)), 100, 20), NULL, auto_builder_state.auto_block_presets[i].name);
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
         
		 if (GUIButton(&context, input, RectPosSize(1270, 20, 20, 20), NULL, NULL, auto_builder_state.recording_robot))
		 {
			 auto_builder_state.recording_robot = !auto_builder_state.recording_robot;

			 for (u32 i = 0; i < robot_state.robot_hardware_count; i++)
			 {
				 robot_state.robot_hardware[i].got_first_recorded_update = false;
			 }
		 }

         if(GUIButton(&context, input, RectPosSize(1245, 20, 20, 40), NULL, NULL, auto_builder_state.auto_file_selector_open))
         {
            auto_builder_state.auto_file_selector_open = !auto_builder_state.auto_file_selector_open;
         }
         
         if(auto_builder_state.auto_file_selector_open)
         {
            rect2 block_info_bounds = RectPosSize(1100, 140, 200, 200);
            Rectangle(&context, RectPosSize(1100, 140, 200, 200), V4(0.5f, 0.0f, 0.0f, 0.5f));
            
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
			 SendAutonomousPacket(server_socket, &auto_builder_state, &generic_arena);
         }
         
         if(auto_builder_state.selected_block)
         {
            rect2 block_info_bounds = RectPosSize(1100, 140, 200, 200);
            Rectangle(&context, RectPosSize(1100, 140, 200, 250), V4(0.5f, 0.0f, 0.0f, 0.5f));
            //Text(&context, V2(1120, 160), auto_builder_state.selected_block->name, 20);
            
            if(auto_builder_state.selected_block->type == AutoBlockType_Motor)
            {
               char number_buffer[10];
               char string_buffer[30];
               
               Text(&context, V2(1120, 180),
                        ConcatStrings("Value: ", R32ToString(auto_builder_state.selected_block->motor.value, number_buffer), string_buffer),
                        20);
               
			   TextBox(&context, input, RectPosSize(1120, 205, 100, 20), auto_builder_state.text_box1_buffer,
				       ArrayCount(auto_builder_state.text_box1_buffer), &auto_builder_state.text_box1_selected);
			   
			   if (auto_builder_state.text_box1_selected && input.key_enter)
			   {
				   auto_builder_state.selected_block->motor.value = StringToR32(auto_builder_state.text_box1_buffer);
			   }

			   if (auto_builder_state.selected_block->motor.value > 1.0f)
			   {
				   auto_builder_state.selected_block->motor.value = 1.0f;
			   }
			   else if (auto_builder_state.selected_block->motor.value < -1.0f)
			   {
				   auto_builder_state.selected_block->motor.value = -1.0f;
			   }

               Text(&context, V2(1120, 240),
                        ConcatStrings("Time: ", R32ToString(auto_builder_state.selected_block->motor.time, number_buffer), string_buffer),
                        20);
               
			   TextBox(&context, input, RectPosSize(1120, 265, 100, 20), auto_builder_state.text_box2_buffer,
					   ArrayCount(auto_builder_state.text_box2_buffer), &auto_builder_state.text_box2_selected);

			   if (auto_builder_state.text_box2_selected && input.key_enter)
			   {
				   auto_builder_state.selected_block->motor.time = StringToR32(auto_builder_state.text_box2_buffer);
			   }
			   
			   if (auto_builder_state.selected_block->motor.time < 0.0f)
			   {
				   auto_builder_state.selected_block->motor.time = 0.0f;
			   }
            }
            else if(auto_builder_state.selected_block->type == AutoBlockType_Solenoid)
            {
               if(GUIButton(&context, input, RectPosSize(1120, 180, 80, 40), NULL, "Extend", auto_builder_state.selected_block->solenoid == SolenoidState_Extended))
               {
                  auto_builder_state.selected_block->solenoid = SolenoidState_Extended;
               }
               
               if(GUIButton(&context, input, RectPosSize(1120, 225, 80, 40), NULL, "Retract", auto_builder_state.selected_block->solenoid == SolenoidState_Retracted))
               {
                  auto_builder_state.selected_block->solenoid = SolenoidState_Retracted;
               }
               
               if(GUIButton(&context, input, RectPosSize(1120, 270, 80, 40), NULL, "Stop", auto_builder_state.selected_block->solenoid == SolenoidState_Stopped))
               {
                  auto_builder_state.selected_block->solenoid = SolenoidState_Stopped;
               }
            }
			else if (auto_builder_state.selected_block->type == AutoBlockType_Drive)
			{
				char number_buffer[10];
				char string_buffer[30];

				Text(&context, V2(1120, 180),
					ConcatStrings("Forward: ", R32ToString(auto_builder_state.selected_block->drive.forward_value, number_buffer), string_buffer),
					20);
            
				TextBox(&context, input, RectPosSize(1120, 205, 100, 20), auto_builder_state.text_box1_buffer,
					ArrayCount(auto_builder_state.text_box1_buffer), &auto_builder_state.text_box1_selected);

				if (auto_builder_state.text_box1_selected && input.key_enter)
				{
					auto_builder_state.selected_block->drive.forward_value = StringToR32(auto_builder_state.text_box1_buffer);
				}

				if (auto_builder_state.selected_block->drive.forward_value > 1.0f)
				{
					auto_builder_state.selected_block->drive.forward_value = 1.0f;
				}
				else if (auto_builder_state.selected_block->drive.forward_value < -1.0f)
				{
					auto_builder_state.selected_block->drive.forward_value = -1.0f;
				}

				Text(&context, V2(1120, 240),
					ConcatStrings("Rotate: ", R32ToString(auto_builder_state.selected_block->drive.rotate_value, number_buffer), string_buffer),
					20);

				TextBox(&context, input, RectPosSize(1120, 265, 100, 20), auto_builder_state.text_box2_buffer,
					ArrayCount(auto_builder_state.text_box2_buffer), &auto_builder_state.text_box2_selected);

				if (auto_builder_state.text_box2_selected && input.key_enter)
				{
					auto_builder_state.selected_block->drive.rotate_value = StringToR32(auto_builder_state.text_box2_buffer);
				}

				if (auto_builder_state.selected_block->drive.rotate_value > 1.0f)
				{
					auto_builder_state.selected_block->drive.rotate_value = 1.0f;
				}
				else if (auto_builder_state.selected_block->drive.rotate_value < -1.0f)
				{
					auto_builder_state.selected_block->drive.rotate_value = -1.0f;
				}

				Text(&context, V2(1120, 320),
					ConcatStrings("Time: ", R32ToString(auto_builder_state.selected_block->drive.time, number_buffer), string_buffer),
					20);

				TextBox(&context, input, RectPosSize(1120, 345, 100, 20), auto_builder_state.text_box3_buffer,
					ArrayCount(auto_builder_state.text_box3_buffer), &auto_builder_state.text_box3_selected);

				if (auto_builder_state.text_box3_selected && input.key_enter)
				{
					auto_builder_state.selected_block->drive.time = StringToR32(auto_builder_state.text_box3_buffer);
				}

				if (auto_builder_state.selected_block->drive.time < 0.0f)
				{
					auto_builder_state.selected_block->drive.time = 0.0f;
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
               auto_builder_state.text_box1_selected = false;
               auto_builder_state.text_box2_selected = false;
               auto_builder_state.text_box3_selected = false;
            }
         }
      }
      else if(page == PageType_Robot)
      {
         Rectangle(&context, RectPosSize(280, 20, 600, 675), V4(0.5f, 0.0f, 0.0f, 0.5f));
         
		 if (robot_state.connected)
		 {
			 GUIButton(&context, input, RectPosSize(160, 20, 100, 30), NULL, robot_state.name);
		 }

         for(u32 i = 0; i < robot_state.robot_hardware_count; i++)
         {
            if(robot_state.selected_hardware == &robot_state.robot_hardware[i])
            {
               Rectangle(&context, RectPosSize(285, (25 + (30 * i)), 110, 30), V4(0.5f, 0.0f, 0.0f, 0.5f));
            }
            
		    char display_text_buffer[512];
		    if(robot_state.robot_hardware[i].type == HardwareType_MotorController)
		    {
		 	   char motor_value_buffer[32];
		 	   ConcatStrings(robot_state.robot_hardware[i].name, " ", R32ToString(robot_state.robot_hardware[i].motor_value, motor_value_buffer), display_text_buffer);
		    }
		    else if(robot_state.robot_hardware[i].type == HardwareType_Solenoid)
		    {
		 	   if(robot_state.robot_hardware[i].solenoid_state == SolenoidState_Extended)
		 	   {
		 		   ConcatStrings(robot_state.robot_hardware[i].name, " Extended", display_text_buffer);
		 	   }
		 	   else if(robot_state.robot_hardware[i].solenoid_state == SolenoidState_Retracted)
		 	   {
		 		   ConcatStrings(robot_state.robot_hardware[i].name, " Retracted", display_text_buffer);
		 	   }
		 	   else if(robot_state.robot_hardware[i].solenoid_state == SolenoidState_Stopped)
		 	   {
		 		   ConcatStrings(robot_state.robot_hardware[i].name, " Stopped", display_text_buffer);
		 	   }
		    }
		    else if (robot_state.robot_hardware[i].type == HardwareType_FloatSensor)
		    {
		 	   char sensor_value_buffer[32];
		 	   ConcatStrings(robot_state.robot_hardware[i].name, " ", R32ToString(robot_state.robot_hardware[i].float_sensor_value, sensor_value_buffer), display_text_buffer);
		    }
		    else if (robot_state.robot_hardware[i].type == HardwareType_BoolSensor)
		    {
		 	   ConcatStrings(robot_state.robot_hardware[i].name, " ", robot_state.robot_hardware[i].bool_sensor_value ? "True" : "False", display_text_buffer);
		    }
			else if (robot_state.robot_hardware[i].type == HardwareType_Drive)
			{
			   char forward_buffer[32];
			   char rotate_buffer[32];

			   R32ToString(robot_state.robot_hardware[i].forward_value, forward_buffer);
			   R32ToString(robot_state.robot_hardware[i].rotate_value, rotate_buffer);

			   ConcatStrings(robot_state.robot_hardware[i].name, " ", forward_buffer, " : ", rotate_buffer, display_text_buffer);
			}
		    else
		    {
		 	   StringCopy(robot_state.robot_hardware[i].name, display_text_buffer);
		    }
         
            if(GUIButton(&context, input, RectPosSize(290, (30 + (30 * i)), 100, 20), NULL, display_text_buffer))
            {
               robot_state.selected_hardware = &robot_state.robot_hardware[i];
            }
         }
         
         if(robot_state.selected_hardware)
         {
			Rectangle(&context, RectPosSize(900, 20, 450, 400), V4(0.5f, 0.0f, 0.0f, 0.5f));
			//Text(&context, V2(920, 40), robot_state.selected_hardware->name, 20);

            if(robot_state.selected_hardware->type == HardwareType_MotorController)
            {
               
            }
            else if(robot_state.selected_hardware->type == HardwareType_Solenoid)
            {

            }
			else if (robot_state.selected_hardware->type == HardwareType_Drive)
			{
				
			}
			else if (robot_state.selected_hardware->type == HardwareType_Camera)
			{
				if(robot_state.selected_hardware->camera_frame != NULL)
					Bitmap(&context, robot_state.selected_hardware->camera_frame, V2(915, 60));
			}
            
            if(GUIButton(&context, input, RectPosSize(900, 20, 10, 10), NULL, NULL))
            {
               robot_state.selected_hardware = NULL;
            }
        }
      }
	  else if (page == PageType_Console)
	  {
		  Rectangle(&context, RectPosSize(280, 20, 800, 675), V4(0.5f, 0.0f, 0.0f, 0.5f));

		  if (GUIButton(&context, input, RectPosSize(1090, 20, 20, 20), NULL, NULL, console_state.track_hardware))
		  {
			  console_state.track_hardware = !console_state.track_hardware;
		  }

		  if (GUIButton(&context, input, RectPosSize(1090, 45, 20, 20), NULL, NULL))
		  {
			  console_state.message_index = 0;

			  for (int i = 0; i < ArrayCount(console_state.messages); i++)
			  {
				  console_state.messages[i].type = ConsoleMessageType_Invalid;
			  }
		  }

		  for (int i = 0; i < ArrayCount(console_state.messages); i++)
		  {
			  if (console_state.messages[i].type != ConsoleMessageType_Invalid)
			  {
				  char *message_type_string = "Invalid";

				  switch (console_state.messages[i].type)
				  {
					  case ConsoleMessageType_ClientMessage:
						  message_type_string = "Client";
						  break;

					  case ConsoleMessageType_ServerMessage:
						  message_type_string = "Server";
						  break;

					  case ConsoleMessageType_ClientPacket:
						  message_type_string = "CPacket";
						  break;

					  case ConsoleMessageType_ServerPacket:
						  message_type_string = "SPacket";
						  break;

					  case ConsoleMessageType_HardwareUpdate:
						  message_type_string = "Hardware";
						  break;
				  }

				  char message_buffer[128];
				  ConcatStrings("[", message_type_string, "]:", console_state.messages[i].message, message_buffer);
				  //Text(&context, V2(300, 40 + (i * 25)), message_buffer, 22);
			  }
		  }
	  }
     */
      
#if 0
      char frame_time_buffer[64];
      Text(&backbuffer, &font, V2(600, 40), R64ToString(frame_length, frame_time_buffer), 20, context.font_memory);
#endif

      RenderUI(&context, window_size);
      SwapBuffers(device_context);
      
      connected = HandleNetwork(server_socket, &auto_builder_state, &robot_state, &console_state);

      frame_length = GetCounter(&last_timer, timer_freq);
      if(frame_length < 33.3)
      {
         Sleep(33.3 - frame_length);
      }
   }
   
   shutdown(server_socket, SD_BOTH);
   closesocket(server_socket);
   WSACleanup();
 
   WaitForSingleObject(network_thread, INFINITE);

   return 0;
}