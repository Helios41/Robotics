Notification *AddNotification(Console *console, string name)
{
   //TODO: switch this to a memory arena & recycle allocator
   Notification *result = (Notification *) malloc(sizeof(Notification));
   *result = {};
   
   BeginTicketMutex(&console->notification_mutex);
   result->next = console->top_notification;
   console->top_notification = result;
   EndTicketMutex(&console->notification_mutex);
 
   result->name = name;
 
   return result;
}

ConsoleMessage *AddMessage(Console *console, string text, Notification *notification = NULL)
{
   //TODO: switch this to a memory arena & recycle allocator
   ConsoleMessage *result = (ConsoleMessage *) malloc(sizeof(ConsoleMessage));
   *result = {};
   
   BeginTicketMutex(&console->message_mutex);
   result->next = console->top_message;
   console->top_message = result;
   EndTicketMutex(&console->message_mutex);
 
   result->text = text;
 
   if(notification)
   {
      BeginTicketMutex(&console->notification_mutex);
      notification->count++;
      EndTicketMutex(&console->notification_mutex);
 
      result->notification = notification;
   }
 
   return result;
}   

void DrawMessageList(layout *message_list, UIContext *context, Console *console)
{
   Rectangle(context->render_context, message_list->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, message_list->bounds, V4(0, 0, 0, 1));
   
   ui_id list_id = GEN_UI_ID;
   interaction_state message_list_interact =
      ClickInteraction(context, Interaction(list_id, message_list), context->input_state.left_up,
                       context->input_state.left_down, Contains(message_list->bounds, context->input_state.pos));
   
   if(message_list_interact.became_selected)
   {
      console->selected_message = NULL;
   }
   
   v2 button_size = V2(GetSize(message_list->bounds).x * 0.8, 40);
   v2 button_margin = V2(GetSize(message_list->bounds).x * 0.1, 10);
    
   BeginTicketMutex(&console->message_mutex);
   for(ConsoleMessage *message = console->top_message;
       message;
       message = message->next)
   {
      button message_button = _Button(POINTER_UI_ID(message), message_list, NULL,
                                      message->text, console->selected_message == message, 
                                      button_size, V2(0, 0), button_margin);
      
      if(message->notification == console->selected_notification)
      {
         RectangleOutline(context->render_context, message_button.elem.bounds, V4(0, 0, 0, 1), 3);
      }
      
      if(message_button.state)
      {
         console->selected_message = (console->selected_message == message) ? NULL : message;
      }
   }
   EndTicketMutex(&console->message_mutex);
}

void DrawSelectedMessagePage(layout *selected_message_page, UIContext *context, Console *console)
{
   Rectangle(context->render_context, selected_message_page->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, selected_message_page->bounds, V4(0, 0, 0, 1));
   
   string title = Literal("PLACEHOLDER");
   
   Text(selected_message_page, title, 40,
        V2((GetSize(selected_message_page->bounds).x - GetTextWidth(context->render_context, title, 40)) / 2.0f, 0), V2(0, 5));
   Text(selected_message_page, console->selected_message->text, 20, V2(0, 0), V2(0, 5));
}

void DrawConsoleOptions(layout *console_options, UIContext *context, Console *console)
{
   Rectangle(context->render_context, console_options->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, console_options->bounds, V4(0, 0, 0, 1));
   
   u32 max_messages = 10;
   
   Text(console_options, Literal("Max Messages: "), 20, V2(0, 0), V2(0, 5)); 
   TextBox(console_options, 0, &max_messages, V2(100, 40), V2(0, 0), V2(0, 0));
   NextLine(console_options);
}

void DrawConsole(layout *console_ui, UIContext *context, DashboardState *dashstate)
{
   v2 console_ui_size = GetSize(console_ui->bounds);
   
   layout message_list = Panel(console_ui, V2(console_ui_size.x * 0.2, console_ui_size.y) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   layout selected_message_page = Panel(console_ui, V2(console_ui_size.x * 0.6, console_ui_size.y) - V2(5, 10), V2(0, 0), V2(5, 5)).lout;
   layout console_options = Panel(console_ui, V2(console_ui_size.x * 0.2, console_ui_size.y) - V2(5, 10), V2(0, 0), V2(5, 5)).lout;
   
   DrawMessageList(&message_list, context, &dashstate->console);
   DrawConsoleOptions(&console_options, context, &dashstate->console);
   
   if(dashstate->console.selected_message)
   {
      DrawSelectedMessagePage(&selected_message_page, context, &dashstate->console);
   }
}