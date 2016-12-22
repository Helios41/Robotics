#include "ui_renderer.cpp"

enum ui_window_type
{
   WindowType_DisplaySettings,
   WindowType_NetworkSettings,
   WindowType_VideoStream,
   WindowType_FileSelector
};

struct network_settings
{
   string connect_to;
   b32 is_mdns;
   b32 reconnect;
};

struct ui_window
{
   ui_window_type type;
   v2 pos;
   v2 size;
   struct ui_window *next;
   
   union
   {
      network_settings *net_settings;
   };
};

enum DashboardPage
{
   Page_Home,
   Page_AutonomousEditor,
   Page_Robot,
   Page_Vision,
   Page_Console
};

enum RobotHardwareType
{
   Hardware_Motor,
   Hardware_Solenoid,
   Hardware_Drive
};

enum SolenoidState
{
   Solenoid_Extended,
   Solenoid_Retracted
};

union RobotHardwareState
{
   r32 motor;
   SolenoidState solenoid;
   struct
   {
      r32 forward;
      r32 rotate;
   };
};  

struct RobotHardwareSample
{
   RobotHardwareState state;
   u64 timestamp;
};

struct RobotHardware
{
   RobotHardwareType type;
   u32 id;
   string name;
   
   u32 sample_count;
   u32 at_sample;
   RobotHardwareSample *samples;
};

struct Robot
{
   RobotHardware *hardware;
   u32 hardware_count;
   
   string name;
   RobotHardware *selected_hardware;
};

struct AutonomousBlock
{
   b32 is_wait_block;
   
   union
   {
      struct
      {
         RobotHardware *hardware;
         RobotHardwareState state;
      };
      
      r32 wait_duration;
   };
};

struct AutonomousEditor
{
   b32 is_lua_editor;
   
   AutonomousBlock editor_blocks[20];
   u32 editor_block_count;
   
   AutonomousBlock *selected_block;
   
   b32 block_grabbed;
   AutonomousBlock grabbed_block;
};

struct Notification
{
   Notification *next;
   string name;
   u32 count;
};

struct ConsoleMessage
{
   ConsoleMessage *next;
   Notification *notification;
   
   string text;
};

struct Console
{
   //TODO: notification/message arena
   ticket_mutex message_mutex;
   ticket_mutex notification_mutex;
   
   ConsoleMessage *top_message;
   ConsoleMessage *selected_message;
   
   Notification *top_notification;
   Notification *selected_notification;
};

struct DashboardState
{
   MemoryArena *generic_arena;
   
   DashboardPage page;
   ui_window *first_window;
   b32 connected;
   network_settings net_settings;
   
   ui_window *network_settings_window;
   ui_window *display_settings_window;
   
   Robot robot;
   AutonomousEditor auto_editor;
   Console console;
};

ui_window *AddWindow(DashboardState *dashstate, v2 size, v2 pos, ui_window_type type)
{
   ui_window *new_window = (ui_window *) malloc(sizeof(ui_window));
   *new_window = {};
   new_window->size = size;
   new_window->pos = pos;
   new_window->type = type;      
   new_window->next = dashstate->first_window;
   dashstate->first_window = new_window;
   
   if(new_window->type == WindowType_DisplaySettings)
   {
      
   }
   else if(new_window->type == WindowType_NetworkSettings)
   {
      new_window->net_settings = &dashstate->net_settings;
   }
   else if(new_window->type == WindowType_VideoStream)
   {
      
   }
   else if(new_window->type == WindowType_FileSelector)
   {
      
   }
   
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

#include "autonomous_editor.cpp"
#include "robot.cpp"
#include "console.cpp"

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
      dashstate->console.selected_notification = NULL;
   }
   
   Rectangle(&notification_bar, dashstate->connected ? V4(1.0f, 1.0f, 1.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 1.0f),
             top_bar_element_size, V2(0, 0), top_bar_element_margin);
             
   BeginTicketMutex(&dashstate->console.notification_mutex);
   for(Notification *notification = dashstate->console.top_notification;
       notification;
       notification = notification->next)
   {
      button notification_button = _Button(POINTER_UI_ID(notification), &notification_bar, NULL,
                                           notification->name, dashstate->console.selected_notification == notification,
                                           top_bar_element_size, V2(0, 0), top_bar_element_margin);
      
      if(notification_button.state)
      {
         dashstate->console.selected_notification = (dashstate->console.selected_notification == notification) ? NULL : notification;
      }
   }
   EndTicketMutex(&dashstate->console.notification_mutex);
             
   if(Button(&settings_bar, NULL, EmptyString(), top_bar_element_size, V2(0, 0), top_bar_element_margin).state)
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
   
   if(Button(&settings_bar, NULL, EmptyString(), top_bar_element_size, V2(0, 0), top_bar_element_margin).state)
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
   
   if(Button(left_bar, &context->assets->home, EmptyString(), (dashstate->page == Page_Home),
             small_button_size, V2(0, 0), small_button_margin).state)
   {
      dashstate->page = Page_Home;
   }
   
   if(Button(left_bar, &context->assets->gear, EmptyString()/*, fullscreen*/,
             small_button_size, V2(0, 0), small_button_margin).state)
   {
      /*
      fullscreen = !fullscreen;
      SetFullscreen(fullescreen);
      */
   }
   
   Button(left_bar, NULL, EmptyString(), small_button_size, V2(0, 0), small_button_margin);
   Button(left_bar, NULL, EmptyString(), small_button_size, V2(0, 0), small_button_margin);
   Button(left_bar, NULL, EmptyString(), small_button_size, V2(0, 0), small_button_margin);
   
   if(Button(left_bar, NULL, Literal("Autonomous Builder Of Autonomous Autonomousness"), (dashstate->page == Page_AutonomousEditor),
             V2(120, 40), V2(0, 0), V2(5, 5)).state)
   {
      dashstate->page = Page_AutonomousEditor;
   }
   
   if(Button(left_bar, NULL, Literal("Robot"), (dashstate->page == Page_Robot),
             V2(120, 40), V2(0, 0), V2(5, 5)).state)
   {
      dashstate->page = Page_Robot;
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
            
         case WindowType_VideoStream:
            window_name = Literal("Video");
            break;
            
         case WindowType_FileSelector:
            window_name = Literal("File Selector");
            break;
      }
      
      //TODO: make this cleaner
      if(_Button(POINTER_UI_ID(op_window), right_bar, NULL, window_name, V2(bar_size.x * 0.9, 40), V2(0, 0), V2(bar_size.x * 0.05, 5)).state)
      {
         if(op_window->type == WindowType_DisplaySettings)
         {
            dashstate->display_settings_window = NULL;
         }
         else if(op_window->type == WindowType_NetworkSettings)
         {
            dashstate->network_settings_window = NULL;
         }
         
         RemoveWindow(dashstate, op_window);
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
   
   if(dashstate->connected)
   {
      /*
      char text_buffer[512];

      if(robot_state.connected)
      {	
         Text(&context, V2(180, 130),
            ConcatStrings("Connected To ", robot_state.name, text_buffer),
            20);
      }
      else
      {
         Text(&context, V2(180, 130),
            ConcatStrings("Connected To ", SERVER_ADDR, text_buffer),
            20);
      }
      */
   }
   else
   {
      Text(&connect_panel, Literal("Not Connected"), 20, V2(0, 0), V2(5, 5));
   }
   
   NextLine(&connect_panel);
   
   if(Button(&connect_panel, NULL, Literal("Reconnect"), V2(100, 40), V2(0, 0), V2(0, 0)).state)
   {
      /*
      shutdown(server_socket, SD_BOTH);
      closesocket(server_socket);
      server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      
      server.sin_family = AF_INET;
      server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
      server.sin_port = htons(SERVER_PORT);
      
      connected = (connect(server_socket, (struct sockaddr *)&server, sizeof(server)) == 0);
      */
   }
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
      
   }
   else if(dashstate->page == Page_Console)
   {
      DrawConsole(center_area, context, dashstate);
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

void DrawNetworkSettings(layout *window_layout, network_settings *net_settings)
{
   Text(window_layout, Literal("Network Settings"), 20, V2(0,0), V2(0, 0));
   TextBox(window_layout, net_settings->connect_to, V2(GetSize(window_layout->bounds).x * 0.9, 40), V2(0, 0), V2(0, 0));
   ToggleSlider(window_layout, &net_settings->is_mdns, Literal("mDNS"), Literal("IP"), V2(120, 20), V2(0, 0), V2(0, 0));
   
   if(Button(window_layout, NULL, Literal("Reconnect"), V2(120, 20), V2(0, 0), V2(0, 0)).state)
   {
      
   }
}

void DrawWindowTab(layout *tab_layout, ui_window *window)
{
   Rectangle(tab_layout->context->render_context, tab_layout->bounds, V4(0.3f, 0.3f, 0.3f, 1.0f));
   
   v2 tab_button_size = V2(GetSize(tab_layout->bounds).y, GetSize(tab_layout->bounds).y);
   Rectangle(tab_layout, V4(0, 0, 0, 0), tab_button_size, V2(0, 0), V2(0, 0));
   if(Button(tab_layout, NULL, EmptyString(), tab_button_size, V2(0, 0), V2(0, 0)).state)
   {
      
   }
}

void DrawWindow(ui_window *window, UIContext *context, u32 stack_layer)
{
   rect2 window_bounds = RectMinSize(window->pos, window->size);
   Rectangle(context->render_context, window_bounds, V4(0.7f, 0.7f, 0.7f, 1.0f));
   
   ui_id tab_id = POINTER_UI_ID(window);
   
   rect2 tab_bounds = RectPosSize(window->pos, V2(10, 10));
   interaction_state tab_interact = ClickInteraction(context, Interaction(tab_id, 0, stack_layer, context), context->input_state.left_up,
                                                     context->input_state.left_down, Contains(tab_bounds, context->input_state.pos));
   
   if(tab_interact.selected)
   {
      layout tab_layout = Layout(RectMinSize(window->pos - V2(7, 7), V2(window->size.x * 0.8, 14)), context, stack_layer);
      DrawWindowTab(&tab_layout, window);
   }
   else
   {
      Rectangle(context->render_context, tab_bounds, V4(0.0f, 0.0f, 0.0f, 1.0f));
   }
   
   if(tab_interact.active)
   {
      window->pos = context->input_state.pos;
   }
   
   layout window_layout = Layout(window_bounds, context, stack_layer);
   
   if(window->type == WindowType_DisplaySettings)
   {
      DrawDisplaySettings(&window_layout);
   }
   else if(window->type == WindowType_NetworkSettings)
   {
      DrawNetworkSettings(&window_layout, window->net_settings);
   }
   else if(window->type == WindowType_VideoStream)
   {
      
   }
   else if(window->type == WindowType_FileSelector)
   {
      
   }
}

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
      DrawWindow(curr_window, context, stack_layer);
      stack_layer++;
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