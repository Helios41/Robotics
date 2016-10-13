#include "ui_renderer.cpp"

void DrawTopBar(UIContext *context)
{
   layout top_bar = TopBar(context, 15);
   Rectangle(context->render_context, top_bar.bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   
   //TODO: store this in some state/context
   b32 connected = false;
   
   element_definition top_bar_element_def = ElementDefPixel(V2(2.5f, 2.5f), V2(0, 0), V2(10, 10));
   
   Rectangle(&top_bar, connected ? V4(1.0f, 1.0f, 1.0f, 1.0f) : V4(0.0f, 0.0f, 0.0f, 1.0f), top_bar_element_def.element_spec,
             top_bar_element_def.padding_spec, top_bar_element_def.margin_spec);
             
   Rectangle(&top_bar, V4(0.2f, 0.2f, 0.2f, 1.0f), top_bar_element_def.element_spec,
             top_bar_element_def.padding_spec, top_bar_element_def.margin_spec);
}

void DrawLeftBar(UIContext *context)
{
   layout left_bar = LeftBar(context, 140);
   Rectangle(context->render_context, left_bar.bounds, V4(0.2f, 0.2f, 0.2f, 0.7f));
   
   element_definition left_bar_small_button_def = ElementDefPixel(V2(5, 5), V2(0, 0), V2(40, 40));

   if(Button(&left_bar, &context->assets->home, NULL/*, (page == PageType_Home)*/,
             left_bar_small_button_def.element_spec, left_bar_small_button_def.padding_spec, 
             left_bar_small_button_def.margin_spec))
   {
      //page = PageType_Home;
   }
   
   if(Button(&left_bar, &context->assets->gear, NULL/*, fullscreen*/,
             left_bar_small_button_def.element_spec, left_bar_small_button_def.padding_spec, 
             left_bar_small_button_def.margin_spec))
   {
      /*
      fullscreen = !fullscreen;
      SetFullscreen(fullescreen);
      */
   }
   
   Button(&left_bar, NULL, NULL,
          left_bar_small_button_def.element_spec, left_bar_small_button_def.padding_spec, 
          left_bar_small_button_def.margin_spec);
   Button(&left_bar, NULL, NULL,
          left_bar_small_button_def.element_spec, left_bar_small_button_def.padding_spec, 
          left_bar_small_button_def.margin_spec);
   Button(&left_bar, NULL, NULL,
          left_bar_small_button_def.element_spec, left_bar_small_button_def.padding_spec, 
          left_bar_small_button_def.margin_spec);
   
   element_definition left_bar_large_button_def = ElementDefPixel(V2(5, 5), V2(0, 0), V2(120, 40));

   if(Button(&left_bar, NULL, "Autonomous Builder"/*, (page == PageType_Auto)*/,
             left_bar_large_button_def.element_spec, left_bar_large_button_def.padding_spec, 
             left_bar_large_button_def.margin_spec))
   {
      //page = PageType_Auto;
   }
   
   if(Button(&left_bar, NULL, "Robot"/*, (page == PageType_Robot)*/,
             left_bar_large_button_def.element_spec, left_bar_large_button_def.padding_spec, 
             left_bar_large_button_def.margin_spec))
   {
      //page = PageType_Robot;
   }
   
   if(Button(&left_bar, NULL, "Console"/*, (page == PageType_Console)*/,
             left_bar_large_button_def.element_spec, left_bar_large_button_def.padding_spec, 
             left_bar_large_button_def.margin_spec))
   {
      //page = PageType_Console;
   }
}

void DrawRightBar(UIContext *context)
{
   layout right_bar = RightBar(context, 40);
   Rectangle(context->render_context, right_bar.bounds, V4(0.2f, 0.2f, 0.2f, 0.7f));
}

void DrawCenterArea(UIContext *context)
{
   layout center_area = CenterArea(context);
   Rectangle(context->render_context, center_area.bounds, V4(0.1f, 0.3f, 0.3f, 0.5f));
   
   //if(page == PageType_Home)
   {
      //size = V2(800, 85)
      //margin = V2(0, 10)
      layout banner_panel = Panel(&center_area, ElementSizePercent(V2(80, 100)), ElementSizePercent(V2(100, 100)), ElementSizePercent(V2(100, 20)));
      Rectangle(context->render_context, banner_panel.bounds, V4(0.5f, 0.0f, 0.0f, 0.5f));
      Text(&banner_panel, "CN Robotics", 40); //margin = V2(0, 5)
      
      //size = V2(400, 255)
      //margin = V2(10, 10)
      layout connect_panel = Panel(&center_area, ElementSizePercent(V2(100, 100)), ElementSizePercent(V2(100, 100)), ElementSizePercent(V2(40, 30)));
      Rectangle(context->render_context, connect_panel.bounds, V4(0.5f, 0.0f, 0.0f, 0.5f));
      RectangleOutline(context->render_context, connect_panel.bounds, V4(0.0f, 0.0f, 0.0f, 1.0f), 3);
      Text(&connect_panel, "Connection", 20); //margin = V2(0, 5)
      
      //TODO: store this in some state/context
      b32 connected = false;
      
      if(connected)
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
         Text(&connect_panel, "Not Connected", 20); //margin = V2(5, 5)
      }
      
      NextLine(&connect_panel);
      
      //size = V2(100, 40)
      //margin = V2(5, 5)
      if(Button(&connect_panel, NULL, "Reconnect", ElementSizePercent(V2(100, 100)), ElementSizePercent(V2(100, 100)), ElementSizePixel(V2(100, 40))))
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
}

void DrawDashboardUI(UIContext *context)
{
   Rectangle(context->render_context, RectPosSize(V2(0, 0), context->window_size), V4(1.0f, 1.0f, 1.0f, 1.0f));
   //TODO: replace this with 3d spinning gear logo 
   Bitmap(context->render_context, &context->assets->logo, 
          V2((context->window_size.x / 2) - (context->assets->logo.width / 2), (context->window_size.y / 2) - (context->assets->logo.height / 2)));
    
   DrawTopBar(context);
   DrawLeftBar(context);
   DrawRightBar(context);
   DrawCenterArea(context);
}