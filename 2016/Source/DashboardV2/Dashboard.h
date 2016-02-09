#ifndef DASHBOARD_H_
#define DASHBOARD_H_

#include "stb_truetype.h"

#define Kilobyte(BYTE) BYTE * 1024
#define Megabyte(BYTE) Kilobyte(BYTE) * 1024
#define Gigabyte(BYTE) Megabyte(BYTE) * 1024
#define Terabyte(BYTE) Gigabyte(BYTE) * 1024
#define Assert(condition) if(!(condition)){*(u8 *)0 = 0;}
#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

/**
TODO:
   -save robot state stuff (hardware configs & subsystems)
   -networking
   
*/

/**
NOTE:
   -Chrome locks C:/Windows/Fonts/arial.ttf while open, blocking programs from using it
   -
   
*/

union v4
{
   struct
   {
      r32 r;
      r32 g;
      r32 b;
      r32 a;
   };
   
   struct
   {
      r32 x;
      r32 y;
      r32 z;
      r32 w;
   };
   
   r32 vs[4];
};

inline v4 V4(r32 x, r32 y, r32 z, r32 w)
{
   v4 result = {};
   
   result.x = x;
   result.y = y;
   result.z = z;
   result.w = w;
   
   return result;
}

union v2
{
   struct
   {
      r32 u;
      r32 v;
   };
   
   struct
   {
      r32 x;
      r32 y;
   };
   
   r32 vs[2];
};

inline v2 V2(r32 x, r32 y)
{
   v2 result = {};
   
   result.x = x;
   result.y = y;
   
   return result;
}

struct rect2
{
   v2 min;
   v2 max;
};

inline rect2 RectPosSize(r32 x, r32 y, r32 width, r32 height)
{
   rect2 result = {};
   
   result.min.x = x;
   result.min.y = y;
   result.max.x = x + width;
   result.max.y = y + height;
   
   return result;
}

inline rect2 RectMinMax(r32 minx, r32 miny, r32 maxx, r32 maxy)
{
   rect2 result = {};
   
   result.min.x = minx;
   result.min.y = miny;
   result.max.x = maxx;
   result.max.y = maxy;
   
   return result;
}

inline b32 Contains(rect2 rect, v2 vec)
{
   b32 result = (vec.x > rect.min.x) && 
                (vec.x < rect.max.x) &&
                (vec.y > rect.min.y) &&
                (vec.y < rect.max.y);
                
   return result;
}

inline v2 RectGetSize(rect2 rect)
{
   return V2(rect.max.x - rect.min.x, rect.max.y - rect.min.y);
}

inline s32 RoundR32ToS32(r32 real)
{
   s32 result = (s32)(real + 0.5f);
   return result;
}

struct LoadedBitmap
{
   u32 width;
   u32 height;
   u32 *pixels;
};

struct RenderContext
{
   LoadedBitmap *target;
   stbtt_fontinfo *font_info;
};

struct LoadedFont
{
   LoadedBitmap bitmap;
   v2 offset;
};

enum PageType
{
   PageType_Home,
   PageType_Auto,
   PageType_Robot
};

struct EntireFile
{
   void *contents;
   u64 length;
};

struct InputState
{
   b32 left_down;
   b32 left_up;
   b32 right_down;
   b32 right_up;
   v2 pos;
   b32 char_key_up;
   char key_char;
   b32 key_backspace;
   b32 key_up;
   b32 key_down;
};

struct MemoryArena
{
   size_t size;
   size_t used;
   void *memory;
};

//TODO: conditionals for sensors
enum AutoBlockType
{
   AutoBlockType_Motor,
   AutoBlockType_Solenoid
};

enum SolenoidState
{
   SolenoidState_Extend,
   SolenoidState_Retract,
   SolenoidState_Stop
};

struct MotorState
{
   r32 value;
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
   };
};

enum HardwareType
{
   HardwareType_MotorController,
   HardwareType_Solenoid
};

enum MotorControllerControlType
{
   MotorControllerControlType_Analog,
   MotorControllerControlType_Button
};

struct MotorControllerControl
{
   u32 controller_index;
   u32 button_axis_index;
   MotorControllerControlType type;
   r32 value;
};

enum SolenoidControlType
{
   SolenoidControlType_Extend,
   SolenoidControlType_Retract,
   SolenoidControlType_Toggle
};

struct SolenoidControl
{
   u32 controller_index;
   u32 button_index;
   SolenoidControlType type;
};

struct RobotHardware
{
   char name[16];
   u32 hwid;
   HardwareType type;
   u32 control_count;
   
   union
   {
      MotorControllerControl motor[8];
      SolenoidControl solenoid[8];
   };
};

enum SubsystemType
{
   SubsystemType_TankDrive2x2,
   SubsystemType_TEST,
   SubsystemType_Count
};

//TODO: replace inversion with multiplier
struct TankDrive2x2Control
{
   u32 front_left_hwid;
   u32 back_left_hwid;
   u32 front_right_hwid;
   u32 back_right_hwid;
   
   u32 controller_index;
   
   u32 drive_axis_index;
   b32 invert_drive_axis;
   u32 rotate_axis_index;
   b32 invert_rotate_axis;
};

struct RobotSubsystem
{
   char name[16];
   SubsystemType type;
   
   union
   {
      TankDrive2x2Control tank_drive2x2;
   };
};

enum RobotPageType
{
   RobotPageType_Hardware,
   RobotPageType_Subsystems
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
};

struct RobotState
{
   RobotPageType robot_page;
   
   RobotHardware robot_hardware[32];
   u32 robot_hardware_count;
   
   RobotSubsystem robot_subsystems[8];
   u32 robot_subsystems_count;
   
   RobotHardware *selected_hardware;
   b32 hardware_control_selected;
   u32 selected_hardware_control;
   
   RobotSubsystem *selected_subsystem;
   b32 subsystem_name_selected;
};

#pragma pack(push, 1)
struct AutoFileHeader
{
   u32 identifier;
   u32 auto_block_count;
};
#pragma pack(pop)

#endif