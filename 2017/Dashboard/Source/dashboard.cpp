#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv/cv.h"

#include "ui_renderer.cpp"

enum ui_window_type
{
   WindowType_DisplaySettings,
   WindowType_NetworkSettings,
   //WindowType_FileSelector,
   WindowType_CreateBlock,
   WindowType_EditBlock
};

struct network_settings
{
   string connect_to;
   b32 is_mdns;
};

enum ui_window_flags
{
   Flag_CloseRequested = (1 << 0),
   Flag_TabExpanded = (1 << 1),
   Flag_Undecorated = (1 << 2)
};

struct ui_window
{
   ui_window_type type;
   v2 pos;
   v2 size;
   struct ui_window *next;
   
   v2 move_start_pos;
   
   u32 flags;
   
   union
   {
      network_settings *net_settings;
	  struct
	  {
	     struct FunctionBlockLink *function;
		 struct CoroutineBlock *coroutine;
	  } edit_block;
	  
	  struct
	  {
	     struct CoroutineBlock *coroutine;
		 r32 scroll;
	  } create_block;
   };
};

enum DashboardPage
{
   Page_Home,
   Page_AutonomousEditor,
   Page_Robot,
   Page_Vision,
   Page_Console,
   Page_Teleop
};

struct RobotHardware
{
   RobotHardwareType type;
   string name;
   
   u32 at_sample;
   RobotHardwareSample samples[256];
};

struct Robot
{
   b32 connected;
   string name;
   
   RobotHardware *hardware;
   u32 hardware_count;
   
   RobotHardware *selected_hardware;
   
   //NOTE: drive train hardware info
   b32 drive_encoder;
   u32 at_sample;
   RobotDriveSample samples[512];
   
	struct
	{
		b32 has_autonomous;
		string autonomous_name;
	} uploaded_state;
};

struct FunctionBlockLink
{
	FunctionBlock block;
	FunctionBlockLink *next;
};

struct CoroutineBlock
{
   FunctionBlockLink *first_block;
};

struct AutonomousEditor
{
   ui_window *create_block_window;
   ui_window *edit_block_window;
   
   string name;
   CoroutineBlock coroutine;
   
   b32 is_recording;
   r32 recording_timer_start;
   u64 earliest_sample_timestamp;
};

enum ConsoleCategory
{
	Category_None = 0,
	Category_Vision,
	Category_Network,
	Category_Debug,
	Category_RobotDebug,
	Category_RobotAutonomousDebug
};

struct ConsoleMessage
{
   ConsoleMessage *next;
   string text;
   ConsoleCategory category;
};

struct Console
{
   ConsoleMessage *top_message;
   ConsoleMessage *selected_message;
   
   ConsoleCategory selected_category;
};

struct ControllerType
{
	string name;
	
	u32 button_count;
	string *buttons;
	
	u32 axis_count;
	string *axes;
};

//TODO: send axis and button count per controller?
struct Control
{
	ControlType type;
	u8 button_or_axis;
	CoroutineBlock coroutine;
};

struct ControlLink
{
	Control control;
	ControlLink *next;
};

struct DashboardState
{
   MemoryArena *generic_arena;
   struct NetworkState *net_state;
   
   u64 latest_sample_timestamp;
   
   b32 competition_mode;
   DashboardPage page;
   ui_window *first_window;
   network_settings net_settings;
   
   ui_window *network_settings_window;
   ui_window *display_settings_window;
   
   Robot robot;
   AutonomousEditor auto_editor;
   Console console;
  
   u32 controller_type_count;
   ControllerType *controller_types;
   
	struct
	{	
		cv::VideoCapture *camera;
		s32 brightness;
		rect2 top_reference;
		rect2 bottom_reference;
		u32 tracks_per_second;
		b32 enabled;
		r32 turret_speed;
		
		b32 target_hit;
		rect2 top_target;
		rect2 bottom_target;
		
		r32 left_limit;
		r32 right_limit;
		
		b32 frame_grab_success;
		cv::Mat *grabbed_frame;
		
		r32 sweep_speed;
	
		r32 last_track_time;
		r32 movement;
		
		r32 shooter_threshold;
	} vision;
};

ui_window *AddWindow(DashboardState *dashstate, v2 size, v2 pos, ui_window_type type, u32 flags = 0)
{
   ui_window *new_window = (ui_window *) malloc(sizeof(ui_window));
   *new_window = {};
   new_window->size = size;
   new_window->pos = pos;
   new_window->type = type;
   new_window->flags = flags;
   new_window->next = dashstate->first_window;
   dashstate->first_window = new_window;
   
   if(new_window->type == WindowType_DisplaySettings)
   {
      
   }
   else if(new_window->type == WindowType_NetworkSettings)
   {
      new_window->net_settings = &dashstate->net_settings;
   }
   /*
   else if(new_window->type == WindowType_VideoStream)
   {
      
   }
   else if(new_window->type == WindowType_FileSelector)
   {
      
   }
   */
   
   return new_window;
}

void RemoveWindow(DashboardState *dashstate, ui_window *window)
{
   if(window == dashstate->first_window)
   {
      dashstate->first_window = window->next;
   }
   else
   {   
      for(ui_window *curr_window = dashstate->first_window;
          curr_window;
          curr_window = curr_window->next)
      {
         if(curr_window->next == window)
         {
            curr_window->next = window->next;
            break;
         }
      }
   }
   
   free(window);
}

void NetworkReconnect(struct NetworkState *net_state, network_settings *net_settings);
void UploadAutonomous(struct NetworkState *net_state, AutonomousEditor *auto_builder, u8 slot);
//UploadControls
void RequestUploadedState(struct NetworkState *net_state);
void SendSetFloat(struct NetworkState *net_state, u32 hardware_index, r32 value);
void SendSetMultiplier(struct NetworkState *net_state, u32 hardware_index, r32 multiplier);
void SendArcadeDrive(struct NetworkState *net_state, r32 power, r32 rotate);
void SendSetDriveMultiplier(struct NetworkState *net_state, r32 multiplier);

#include "blocklang.cpp"

#include "console.cpp"
#include "robot.cpp"
#include "autonomous_editor.cpp"
#include "vision.cpp"
#include "teleop.cpp"

void ConsoleCategoryButton(layout *panel, DashboardState *dashstate,
						   ConsoleCategory category, string name,
						   v2 size, v2 margin)
{
	button notification_button =
		_Button(POINTER_UI_ID(category), panel, NULL,
                name, dashstate->console.selected_category == category,
                size, V2(0, 0), margin);
      
    if(notification_button.state)
    {
		dashstate->console.selected_category =
			(dashstate->console.selected_category == category) ? Category_None : category;
    }
}

void DrawTopBar(layout *top_bar, UIContext *context, DashboardState *dashstate)
{
   Rectangle(context->render_context, top_bar->bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   v2 top_bar_size = GetSize(top_bar->bounds);
   
   v2 top_bar_element_size = V2((top_bar_size.y * 2.0f) / 3.0f, (top_bar_size.y * 2.0f) / 3.0f);
   v2 top_bar_element_margin = V2(top_bar_size.y / 6.0f, top_bar_size.y / 6.0f);
   
   r32 settings_bar_width = top_bar_element_size.x * 2 + top_bar_element_margin.x * 3;
   
   layout notification_bar = Panel(top_bar, V2(top_bar_size.x - settings_bar_width, top_bar_size.y), V2(0, 0), V2(0, 0)).lout;
   layout settings_bar = Panel(top_bar, V2(settings_bar_width, top_bar_size.y), V2(0, 0), V2(0, 0)).lout;
   RectangleOutline(context->render_context, settings_bar.bounds, V4(0.0f, 0.0f, 0.0f, 1.0f), 3);
   
   ui_id notification_bar_id = GEN_UI_ID;
   interaction_state notification_bar_interact =
      ClickInteraction(context, Interaction(notification_bar_id, &notification_bar), context->input_state.left_up,
                       context->input_state.left_down, Contains(notification_bar.bounds, context->input_state.pos));
   
   if(notification_bar_interact.became_selected)
   {
      dashstate->console.selected_category = Category_None;
   }
   
   Rectangle(&notification_bar, dashstate->robot.connected ? V4(1.0f, 1.0f, 1.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 1.0f),
             top_bar_element_size, V2(0, 0), top_bar_element_margin);
             
   ConsoleCategoryButton(&notification_bar, dashstate,
						 Category_Vision, Literal("Vision"),
						 top_bar_element_size, top_bar_element_margin);
             
   ConsoleCategoryButton(&notification_bar, dashstate,
						 Category_Network, Literal("Network"),
						 top_bar_element_size, top_bar_element_margin);
						 
   ConsoleCategoryButton(&notification_bar, dashstate,
						 Category_Debug, Literal("Debug"),
						 top_bar_element_size, top_bar_element_margin);
			 
   ConsoleCategoryButton(&notification_bar, dashstate,
						 Category_RobotDebug, Literal("Robot Debug"),
						 top_bar_element_size, top_bar_element_margin);
						 
   ConsoleCategoryButton(&notification_bar, dashstate,
						 Category_RobotAutonomousDebug, Literal("Robot Autonomous Debug"),
						 top_bar_element_size, top_bar_element_margin);
						 
   if(Button(&settings_bar, NULL, Literal("Display Settings"), top_bar_element_size, V2(0, 0), top_bar_element_margin).state)
   {
      if(!dashstate->display_settings_window)
      {
         dashstate->display_settings_window =
            AddWindow(dashstate, V2(200, 100), V2(100, 100), WindowType_DisplaySettings);
      }
      else
      {
         dashstate->display_settings_window->pos = V2(100, 100);
      }
   }
   
   if(Button(&settings_bar, NULL, Literal("Network Settings"), top_bar_element_size, V2(0, 0), top_bar_element_margin).state)
   {
      if(!dashstate->network_settings_window)
      {
         dashstate->network_settings_window =
            AddWindow(dashstate, V2(200, 100), V2(250, 100), WindowType_NetworkSettings);
      }
      else
      {
         dashstate->network_settings_window->pos = V2(250, 100);
      }
   }
}

void DrawLeftBar(layout *left_bar, UIContext *context, DashboardState *dashstate)
{
   Rectangle(context->render_context, left_bar->bounds, V4(0.2f, 0.2f, 0.2f, 0.7f));

   v2 left_bar_size = GetSize(left_bar->bounds);
   
   v2 small_button_size = V2(Min(left_bar_size.x * 0.8f, 40), Min(left_bar_size.x * 0.8f, 40));
   v2 small_button_margin = V2(Min(left_bar_size.x * 0.1f, 5), Min(left_bar_size.x * 0.1f, 5));
   
   if(Button(left_bar, &context->assets->home, Literal("Home"), (dashstate->page == Page_Home),
             small_button_size, V2(0, 0), small_button_margin).state)
   {
      dashstate->page = Page_Home;
   }
   
   if(Button(left_bar, &context->assets->competition, Literal("Competition Mode"),
             small_button_size, V2(0, 0), small_button_margin).state)
   {
      //NOTE: this button makes the dashboard go into competition mode
      dashstate->competition_mode = true;
   }
   
   Button(left_bar, NULL, EmptyString(), small_button_size, V2(0, 0), small_button_margin);
   Button(left_bar, NULL, EmptyString(), small_button_size, V2(0, 0), small_button_margin);
   Button(left_bar, NULL, EmptyString(), small_button_size, V2(0, 0), small_button_margin);
   
   if(Button(left_bar, NULL, Literal("Autonomous Builder Of Autonomous Autonomousness"), (dashstate->page == Page_AutonomousEditor),
             V2(120, 40), V2(0, 0), V2(5, 5)).state)
   {
      dashstate->page = Page_AutonomousEditor;
   }
   
   //if(dashstate->robot.connected)
   {
      if(Button(left_bar, NULL, Literal("Robot"), (dashstate->page == Page_Robot),
                V2(120, 40), V2(0, 0), V2(5, 5)).state)
      {
         dashstate->page = Page_Robot;
      }
   }
   
   if(Button(left_bar, NULL, Literal("Vision"), (dashstate->page == Page_Vision),
             V2(120, 40), V2(0, 0), V2(5, 5)).state)
   {
      dashstate->page = Page_Vision;
   }
   
   if(Button(left_bar, NULL, Literal("Console"), (dashstate->page == Page_Console),
             V2(120, 40), V2(0, 0), V2(5, 5)).state)
   {
      dashstate->page = Page_Console;
   }
   
   if(Button(left_bar, NULL, Literal("Teleop"), (dashstate->page == Page_Teleop),
             V2(120, 40), V2(0, 0), V2(5, 5)).state)
   {
      dashstate->page = Page_Teleop;
   }
}

void DrawRightBar(layout *right_bar, UIContext *context, DashboardState *dashstate)
{
   Rectangle(context->render_context, right_bar->bounds, V4(0.2f, 0.2f, 0.2f, 0.7f));
   v2 bar_size = GetSize(right_bar->bounds);
   
   for(ui_window *curr_window = dashstate->first_window;
       curr_window;)
   {
      ui_window *op_window = curr_window;
      curr_window = curr_window->next;
      
      string window_name = EmptyString();
      
      switch(op_window->type)
      {
         case WindowType_DisplaySettings:
            window_name = Literal("Display Settings");
            break;
            
         case WindowType_NetworkSettings:
            window_name = Literal("Network Settings");
            break;
            
         /*
         case WindowType_VideoStream:
            window_name = Literal("Video");
            break;
            
         case WindowType_FileSelector:
            window_name = Literal("File Selector");
            break;
         */
      }
      
      //TODO: make this cleaner
      if(_Button(POINTER_UI_ID(op_window), right_bar, NULL, window_name, V2(bar_size.x * 0.9, 40), V2(0, 0), V2(bar_size.x * 0.05, 5)).state)
      {
         op_window->flags |= Flag_CloseRequested;
      }
   }
}

void DrawHome(layout *center_area, UIContext *context, DashboardState *dashstate)
{
   v2 center_area_size = GetSize(center_area->bounds);
   
   //size = V2(800, 85)
   //margin = V2(0, 10)
   layout banner_panel = Panel(center_area, V2(center_area_size.x * 0.8f, center_area_size.y * 0.2f), V2(0, 0), V2(center_area_size.x * 0.1f, 0)).lout;
   Rectangle(context->render_context, banner_panel.bounds, V4(0.5f, 0.0f, 0.0f, 0.5f));
   Text(&banner_panel, Literal("CN Robotics"), 40,
        V2((GetSize(banner_panel.bounds).x - GetTextWidth(context->render_context, Literal("CN Robotics"), 40)) / 2.0f, 0), V2(0, 5)); 
   
   //size = V2(400, 255)
   //margin = V2(10, 10)
   layout connect_panel = Panel(center_area, V2(center_area_size.x * 0.4f, center_area_size.y * 0.3f), V2(0, 0), V2(0, 0)).lout;
   Rectangle(context->render_context, connect_panel.bounds, V4(0.5f, 0.0f, 0.0f, 0.5f));
   RectangleOutline(context->render_context, connect_panel.bounds, V4(0.0f, 0.0f, 0.0f, 1.0f), 3);
   Text(&connect_panel, Literal("Connection"), 20, V2(0, 0), V2(0, 5));
   
   if(dashstate->robot.connected)
   {
      TemporaryMemoryArena temp_arena = BeginTemporaryMemory(dashstate->generic_arena);
      
      Text(&connect_panel, Concat(Literal("Connected To "), dashstate->robot.name, &temp_arena),
           20, V2(0, 0), V2(0, 5));
           
      EndTemporaryMemory(temp_arena);
   }
   else
   {
      Text(&connect_panel, Literal("Not Connected"), 20, V2(0, 0), V2(5, 5));
   }
   
   NextLine(&connect_panel);
   
   if(Button(&connect_panel, NULL, Literal("Reconnect"), V2(100, 40), V2(0, 0), V2(0, 0)).state)
   {
      NetworkReconnect(dashstate->net_state, &dashstate->net_settings);
   }
   
   //TODO: info panel (name, year, game name, etc...)
   
   //TODO: diagnostic panel (button that runs a system test)
}

void DrawCenterArea(layout *center_area, UIContext *context, DashboardState *dashstate)
{  
   if(dashstate->page == Page_Home)
   {
      DrawHome(center_area, context, dashstate);
   }
   else if(dashstate->page == Page_AutonomousEditor)
   {
      DrawAutonomousEditor(center_area, context, dashstate);
   }
   else if(dashstate->page == Page_Robot)
   {
      DrawRobot(center_area, context, dashstate);
   }
   else if(dashstate->page == Page_Vision)
   {
      DrawVision(center_area, context, dashstate);
   }
   else if(dashstate->page == Page_Console)
   {
      DrawConsole(center_area, context, dashstate);
   }
   else if(dashstate->page == Page_Teleop)
   {
      DrawTeleop(center_area, context, dashstate);
   }
}

void DrawBackground(UIContext *context)
{
   Rectangle(context->render_context, RectMinSize(V2(0, 0), context->window_size), V4(1.0f, 1.0f, 1.0f, 1.0f));
   
   //TODO: replace this with 3d spinning gear logo 
   Bitmap(context->render_context, &context->assets->logo, 
          V2((context->window_size.x / 2) - (context->assets->logo.width / 2), (context->window_size.y / 2) - (context->assets->logo.height / 2)));
}

void DrawDisplaySettings(layout *window_layout)
{
   Text(window_layout, Literal("Display Settings"), 20, V2(0, 0), V2(0, 0));
}

void DrawNetworkSettings(layout *window_layout, DashboardState *dashstate)
{
   network_settings *net_settings = &dashstate->net_settings;
   
   Text(window_layout, Literal("Network Settings"), 20, V2(0,0), V2(0, 0));
   TextBox(window_layout, net_settings->connect_to, V2(GetSize(window_layout->bounds).x * 0.9, 40), V2(0, 0), V2(0, 0));
   ToggleSlider(window_layout, &net_settings->is_mdns, Literal("mDNS"), Literal("IP"), V2(120, 20), V2(0, 0), V2(0, 0));
   
   if(Button(window_layout, NULL, Literal("Reconnect"), V2(120, 20), V2(0, 0), V2(0, 0)).state)
   {
      NetworkReconnect(dashstate->net_state, &dashstate->net_settings);
   }
}

void DrawWindowTab(layout *tab_layout, ui_window *window)
{
   Rectangle(tab_layout->context->render_context, tab_layout->bounds, V4(0.3f, 0.3f, 0.3f, 1.0f));
   
   v2 tab_button_size = V2(GetSize(tab_layout->bounds).y, GetSize(tab_layout->bounds).y);
   Rectangle(tab_layout, V4(0, 0, 0, 0), tab_button_size, V2(0, 0), V2(0, 0));
   
   if(Button(tab_layout, NULL, Literal("Close"), tab_button_size, V2(0, 0), V2(0, 0)).state)
   {
      window->flags |= Flag_CloseRequested;
   }
}

void DrawWindow(ui_window *window, DashboardState *dashstate, UIContext *context, u32 stack_layer)
{
   rect2 window_bounds = RectMinSize(window->pos, window->size);
   Rectangle(context->render_context, window_bounds, V4(0.7f, 0.7f, 0.7f, 1.0f));
   
   rect2 tab_bounds = RectPosSize(window->pos, V2(10, 10));
   interaction_state tab_interact = ClickInteraction(context, Interaction(POINTER_UI_ID(window), 0, stack_layer, context), context->input_state.left_up,
                                                     context->input_state.left_down, Contains(tab_bounds, context->input_state.pos));
   
   if(tab_interact.became_active)
   {
      window->move_start_pos = context->input_state.pos;
   }
   
   if(tab_interact.became_selected &&
      (Distance(window->move_start_pos, context->input_state.pos) < 10))
   {
      window->flags = ((~window->flags) & Flag_TabExpanded) | ((~Flag_TabExpanded) & window->flags);
   }
   
   if(window->flags & Flag_TabExpanded)
   {
      layout tab_layout = Layout(RectMinSize(window->pos - V2(7, 7), V2(window->size.x * 0.8, 14)), context, stack_layer);
      DrawWindowTab(&tab_layout, window);
   }
   else
   {
      Rectangle(context->render_context, tab_bounds, V4(0.0f, 0.0f, 0.0f, 1.0f));
   }
   
   if(tab_interact.active &&
      (Distance(window->move_start_pos, context->input_state.pos) > 10))
   {
      window->pos = context->input_state.pos;
   }
   
   rect2 resize_tab_bounds = RectPosSize(window->pos + window->size, V2(10, 10));
   interaction_state resize_tab_interact =
      ClickInteraction(context, Interaction(POINTER_UI_ID(window), 0, stack_layer, context), context->input_state.left_up,
                                            context->input_state.left_down, Contains(resize_tab_bounds, context->input_state.pos));
                                          
   if(resize_tab_interact.active)
   {
      window->size = context->input_state.pos - window->pos;
   }
   
   Rectangle(context->render_context, resize_tab_bounds, V4(0.0f, 0.0f, 0.0f, 1.0f));
   
   layout window_layout = Layout(window_bounds, context, stack_layer);
   
   if(window->type == WindowType_DisplaySettings)
   {
      DrawDisplaySettings(&window_layout);
   }
   else if(window->type == WindowType_NetworkSettings)
   {
      DrawNetworkSettings(&window_layout, dashstate);
   }
   /*
   else if(window->type == WindowType_FileSelector)
   {
      
   }
   */
   /*
   else if(window->type == WindowType_DashboardUI)
   {
      
   }
   else if(window->type == WindowType_Teleop)
   {
      
   }
   */
   else if(window->type == WindowType_CreateBlock)
   {
      DrawCreateBlock(window, &window_layout, dashstate);
   }
   else if(window->type == WindowType_EditBlock)
   {
      DrawEditBlock(window, &window_layout, dashstate);
   }
}

/**
TODO

RESTRUCTURE THIS TO MAKE IT JUST ANOTHER WINDOW
   -everything is a window
   -make the dash ui just a fullscreened window
*/

//TODO: stop passing UIContext to everything
void DrawDashboardUI(UIContext *context, DashboardState *dashstate)
{
   context->tooltip = EmptyString();
   
   layout root_layout = Layout(RectMinSize(V2(0, 0), context->window_size), context, 0);
   v2 layout_size = GetSize(root_layout.bounds);
   
   v2 top_bar_size = V2(layout_size.x, Min(layout_size.y * 0.025f, 15));
   v2 left_bar_size = V2(Min(layout_size.x * 0.2f, 140), layout_size.y - top_bar_size.y);
   v2 right_bar_size = V2(layout_size.x * 0.05f, layout_size.y - top_bar_size.y);
   v2 center_area_size = V2(layout_size.x - left_bar_size.x - right_bar_size.x, layout_size.y - top_bar_size.y);
   
   layout top_bar = Panel(&root_layout, top_bar_size, V2(0, 0), V2(0, 0)).lout;
   layout left_bar = Panel(&root_layout, left_bar_size, V2(0, 0), V2(0, 0)).lout;
   layout center_area = Panel(&root_layout, center_area_size, V2(0, 0), V2(0, 0)).lout;
   layout right_bar = Panel(&root_layout, right_bar_size, V2(0, 0), V2(0, 0)).lout;
   
   DrawBackground(context);
   DrawTopBar(&top_bar, context, dashstate);
   DrawLeftBar(&left_bar, context, dashstate);
   DrawCenterArea(&center_area, context, dashstate);
   DrawRightBar(&right_bar, context, dashstate);
   
   u32 stack_layer = 1;
   for(ui_window *curr_window = dashstate->first_window;
       curr_window;
       curr_window = curr_window->next)
   {
      DrawWindow(curr_window, dashstate, context, stack_layer);
      stack_layer++;
   }
   
   //NOTE: loop that closes windows that requested a close
   for(ui_window *curr_window = dashstate->first_window;
       curr_window;)
   {
      ui_window *op_window = curr_window;
      curr_window = curr_window->next;
      
      if(op_window->flags & Flag_CloseRequested)
      {
         if(op_window->type == WindowType_DisplaySettings)
         {
            dashstate->display_settings_window = NULL;
         }
         else if(op_window->type == WindowType_NetworkSettings)
         {
            dashstate->network_settings_window = NULL;
         }
         else if(op_window->type == WindowType_CreateBlock)
         {
            dashstate->auto_editor.create_block_window = NULL;
         }
         else if(op_window->type == WindowType_EditBlock)
         {
            dashstate->auto_editor.edit_block_window = NULL;
         }
         
         RemoveWindow(dashstate, op_window);
      }
   }
   
   /**
      BEGIN UI STUFF
   */
   if(!IsEmpty(context->tooltip))
   {
      v2 tooltip_size = GetTextSize(context->render_context, context->tooltip, 20);
      rect2 tooltip_bounds = RectMinSize(context->input_state.pos, tooltip_size);
      
      if(tooltip_bounds.max.x > root_layout.bounds.max.x)
      {
         tooltip_bounds = RectMinSize(context->input_state.pos - V2(tooltip_size.x, 0), tooltip_size);
      }
      
      Rectangle(context->render_context, tooltip_bounds, V4(0.2, 0.2, 0.2, 0.4));
      Text(context->render_context, tooltip_bounds.min, context->tooltip, 20);
   }
   
   if(!context->hot_element_refreshed)
   {
      context->hot_element = NULL_INTERACTION;
   }
   context->hot_element_refreshed = false;
   
   if(!context->active_element_refreshed)
   {
      context->active_element = NULL_INTERACTION;
   }
   context->active_element_refreshed = false;
   
   if(!context->selected_element_refreshed)
   {
      context->selected_element = NULL_INTERACTION;
   }
   context->selected_element_refreshed = false;
   
   if(context->input_state.left_up && (context->became_selected == NULL_UI_ID))
   {
      context->selected_element = NULL_INTERACTION;
   }
   context->became_selected = NULL_UI_ID;
   /**
      END UI STUFF
   */
}