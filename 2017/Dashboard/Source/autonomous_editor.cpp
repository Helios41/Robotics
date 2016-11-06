void DrawBlockSelector(layout *block_selector, AutonomousEditor *auto_editor)
{
   RenderContext *render_context = block_selector->context->render_context;
   
   Rectangle(render_context, block_selector->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, block_selector->bounds, V4(0, 0, 0, 1));
   
   v2 block_size = V2(GetSize(block_selector->bounds).x * 0.8, 40);
   v2 block_margin = V2(GetSize(block_selector->bounds).x * 0.1, 10);
   
   if(Button(block_selector, NULL, Literal("Wait"), block_size, V2(0, 0), block_margin).state)
   {
      auto_editor->grabbed_block = &auto_editor->wait_block;
   }
   
   for(u32 i = 0;
       auto_editor->selector_block_count > i;
       i++)
   {
      AutonomousBlock *block = auto_editor->selector_blocks + i;
      string text = block->hardware ? block->hardware->name : EmptyString();
      
      Assert(!block->is_wait_block);
      
      //TODO: make this cleaner
      if(_Button(POINTER_UI_ID(block), block_selector, NULL, text, block_size, V2(0, 0), block_margin).state)
      {
         auto_editor->grabbed_block = block;
      }
   }
}

void DrawEditorMenu(layout *editor_menu, AutonomousEditor *auto_editor)
{
   RenderContext *render_context = editor_menu->context->render_context;
   
   Rectangle(render_context, editor_menu->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, editor_menu->bounds, V4(0, 0, 0, 1));
   
   v2 button_size = V2(GetSize(editor_menu->bounds).x * 0.9, 40);
   v2 button_margin = V2(GetSize(editor_menu->bounds).x * 0.05, 5);
   
   Button(editor_menu, NULL, Literal("Select File"), button_size, V2(0, 0), button_margin);
   Button(editor_menu, NULL, Literal("Load"), button_size, V2(0, 0), button_margin);
   Button(editor_menu, NULL, Literal("Save"), button_size, V2(0, 0), button_margin);
   Button(editor_menu, NULL, Literal("Upload"), button_size, V2(0, 0), button_margin);
   Button(editor_menu, NULL, Literal("Simulate"), button_size, V2(0, 0), button_margin);
   Button(editor_menu, NULL, Literal("Record"), button_size, V2(0, 0), button_margin);
}

void DrawGraphView(layout *graph_view, AutonomousEditor *auto_editor)
{
   RenderContext *render_context = graph_view->context->render_context;
   rect2 bounds = graph_view->bounds;
   
   Rectangle(render_context, bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, bounds, V4(0, 0, 0, 1));
   
   Line(render_context, V2(bounds.min.x, bounds.min.y), V2(bounds.min.x, bounds.max.y), V4(0, 0, 0, 1), 3);
   Line(render_context, V2(bounds.min.x, bounds.max.y), V2(bounds.max.x, bounds.max.y), V4(0, 0, 0, 1), 3);
   
   //NOTE: draw y axis
   for(u32 i = 0;
       i < 4;
       i++)
   {
      r32 y = bounds.min.y + (GetSize(bounds).y / 4.0f) * i;
      
      Line(render_context, V2(bounds.min.x, y), V2(bounds.max.x, y), V4(0, 0, 0, 1), 1);
   }
   
   //NOTE: draw x axis
   for(u32 i = 0;
       i < 8;
       i++)
   {
      r32 x = bounds.min.x + (GetSize(bounds).x / 8.0f) * i;
      
      Line(render_context, V2(x, bounds.min.y), V2(x, bounds.max.y), V4(0, 0, 0, 1), 1);
   }
}

void DrawEditorBar(layout *editor_bar, AutonomousEditor *auto_editor)
{
   RenderContext *render_context = editor_bar->context->render_context;
   
   Rectangle(render_context, editor_bar->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, editor_bar->bounds, V4(0, 0, 0, 1));
   
   v2 button_size = V2(GetSize(editor_bar->bounds).y * 1.8, GetSize(editor_bar->bounds).y * 0.8);
   v2 button_margin = V2(GetSize(editor_bar->bounds).y * 0.1, GetSize(editor_bar->bounds).y * 0.1);
    
   ToggleSlider(editor_bar, &auto_editor->is_lua_editor, Literal("Lua"), Literal("Block"), V2(120, GetSize(editor_bar->bounds).y), V2(0, 0), V2(0, 0));
   
   Button(editor_bar, NULL, EmptyString(), button_size, V2(0, 0), button_margin);
   Button(editor_bar, NULL, EmptyString(), button_size, V2(0, 0), button_margin);
}

void DrawLuaEditor(layout *editor_panel, AutonomousEditor *auto_editor)
{
   RenderContext *render_context = editor_panel->context->render_context;
   Text(editor_panel, Literal("Lua Editor"), 20,
        V2((GetSize(editor_panel->bounds).x - GetTextWidth(render_context, Literal("Lua Editor"), 20)) / 2.0f, 0), V2(0, 5)); 
}

void DrawAutonomousBlock(ui_id id, layout *ui_layout, AutonomousBlock *block,
                         v2 element_size, v2 margin_size, AutonomousEditor *auto_editor)
{
   UIContext *context = ui_layout->context;
   RenderContext *render_context = context->render_context;
   
   element block_element = Element(ui_layout, element_size, V2(0, 0), margin_size);
   interaction_state block_interact = ClickInteraction(context, Interaction(id, ui_layout->ui_layer + 1, ui_layout->stack_layer), context->input_state.left_up,
                                                       context->input_state.left_down, Contains(block_element.bounds, context->input_state.pos));
    
   if(block_interact.became_selected)
   {
      auto_editor->selected_block = block;
   }
    
   if(block_interact.active || (block == auto_editor->selected_block))
   {
      Rectangle(render_context, block_element.bounds, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(block_interact.hot)
   {
      Rectangle(render_context, block_element.bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
      Rectangle(render_context, block_element.bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   //TODO: concat the block's data into its text
   //      eg. "Motor @ 10%" or "Wait for 10 seconds"
   string text = block->is_wait_block ? Literal("Wait") : (block->hardware ? block->hardware->name : EmptyString());
   
   TextWrapRect(render_context, block_element.bounds, text);
}

void DrawBlockList(layout *block_list, AutonomousEditor *auto_editor)
{
   UIContext *context = block_list->context;
   RenderContext *render_context = context->render_context;
   
   ui_id tab_id = POINTER_UI_ID(auto_editor);
   interaction_state block_editor_interact =
      ClickInteraction(context, Interaction(tab_id, block_list), context->input_state.left_up,
                       context->input_state.left_down, Contains(block_list->bounds, context->input_state.pos));
   
   if(block_editor_interact.hot)
   {
      RectangleOutline(render_context, block_list->bounds, V4(0.2, 0.2, 0.2, 1), 3);
   }
   else
   {
      RectangleOutline(render_context, block_list->bounds, V4(0, 0, 0, 1));
   }
   
   if(block_editor_interact.became_selected)
   {
      if(auto_editor->grabbed_block)
      {
         auto_editor->editor_blocks[auto_editor->editor_block_count] = *auto_editor->grabbed_block;
         auto_editor->editor_block_count++;
         auto_editor->grabbed_block = NULL;
      }
      else
      {
         auto_editor->selected_block = NULL;
      }
   }
   
   v2 block_size = V2(GetSize(block_list->bounds).x * 0.8, 40);
   v2 block_margin = V2(GetSize(block_list->bounds).x * 0.1, 10);
   
   for(u32 i = 0;
       auto_editor->editor_block_count > i;
       i++)
   {
      AutonomousBlock *block = auto_editor->editor_blocks + i;
      
      //TODO: make this cleaner
      DrawAutonomousBlock(POINTER_UI_ID(block), block_list, block, block_size, block_margin, auto_editor);
   }
}

void DrawBlockDetails(layout *block_details, AutonomousEditor *auto_editor)
{
   UIContext *context = block_details->context;
   RenderContext *render_context = context->render_context;
   
   AutonomousBlock *block = auto_editor->selected_block;
   
   if(block)
   {
      string text = block->is_wait_block ? Literal("Wait") : (block->hardware ? block->hardware->name : EmptyString());
      
      RectangleOutline(render_context, block_details->bounds, V4(0.2, 0.2, 0.2, 1), 3);
      Text(block_details, text, 20,
           V2((GetSize(block_details->bounds).x - GetTextWidth(render_context, text, 20)) / 2.0f, 0), V2(0, 5)); 
   
   }
   else
   {
      RectangleOutline(render_context, block_details->bounds, V4(0, 0, 0, 1));
   }
}

void DrawBlockEditor(layout *editor_panel, AutonomousEditor *auto_editor)
{
   UIContext *context = editor_panel->context;
   RenderContext *render_context = context->render_context;
   
   element title_element = Text(editor_panel, Literal("Block Editor"), 20,
                                V2((GetSize(editor_panel->bounds).x - GetTextWidth(render_context, Literal("Block Editor"), 20)) / 2.0f, 0), V2(0, 5)); 
   
   layout block_list = Panel(editor_panel, V2(GetSize(editor_panel->bounds).x * 0.25, GetSize(editor_panel->bounds).y - GetSize(title_element.margin_bounds).y), V2(0, 0), V2(0, 0)).lout;
   layout block_details = Panel(editor_panel, V2(GetSize(editor_panel->bounds).x * 0.75, GetSize(editor_panel->bounds).y - GetSize(title_element.margin_bounds).y), V2(0, 0), V2(0, 0)).lout;
    
   RectangleOutline(render_context, editor_panel->bounds, V4(0, 0, 0, 1), 3);
   
   DrawBlockList(&block_list, auto_editor);
   DrawBlockDetails(&block_details, auto_editor);
}

void DrawEditorPanel(layout *editor_panel, AutonomousEditor *auto_editor)
{
   RenderContext *render_context = editor_panel->context->render_context;
   
   Rectangle(render_context, editor_panel->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, editor_panel->bounds, V4(0, 0, 0, 1));
   
   if(auto_editor->is_lua_editor)
   {
      DrawLuaEditor(editor_panel, auto_editor);
   }
   else
   {
      DrawBlockEditor(editor_panel, auto_editor);
   }
}

void DrawAutonomousEditor(layout *auto_editor, UIContext *context, DashboardState *dashstate)
{
   v2 auto_editor_size = GetSize(auto_editor->bounds);
   
   layout block_selector = Panel(auto_editor, V2(auto_editor_size.x * 0.1, auto_editor_size.y) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   layout editor = Panel(auto_editor, V2(auto_editor_size.x * 0.8, auto_editor_size.y), V2(0, 0), V2(0, 0)).lout;
   layout editor_menu = Panel(auto_editor, V2(auto_editor_size.x * 0.1, auto_editor_size.y) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   
   RectangleOutline(context->render_context, editor.bounds, V4(0, 0, 0, 1));
   
   DrawBlockSelector(&block_selector, &dashstate->auto_editor);
   DrawEditorMenu(&editor_menu, &dashstate->auto_editor);
   
   layout graph_view = Panel(&editor, V2(GetSize(editor.bounds).x, GetSize(editor.bounds).y * 0.3) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   layout editor_bar = Panel(&editor, V2(GetSize(editor.bounds).x, GetSize(editor.bounds).y * 0.05), V2(0, 0), V2(0, 0)).lout;
   layout editor_panel = Panel(&editor, V2(GetSize(editor.bounds).x, GetSize(editor.bounds).y * 0.65), V2(0, 0), V2(0, 0)).lout;
   
   DrawGraphView(&graph_view, &dashstate->auto_editor);
   DrawEditorBar(&editor_bar, &dashstate->auto_editor);
   DrawEditorPanel(&editor_panel, &dashstate->auto_editor);
   
   if(dashstate->auto_editor.grabbed_block)
   {
      string grabbed_block_text = dashstate->auto_editor.grabbed_block->is_wait_block ? Literal("Wait") : (dashstate->auto_editor.grabbed_block->hardware ? dashstate->auto_editor.grabbed_block->hardware->name : EmptyString());
      
      if(!IsEmpty(grabbed_block_text))
      {
         context->tooltip = grabbed_block_text;
      }
   }
   
#if 0
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
	Text(&context, V2(280, 30), robot_state.robot_auto_name, 20);

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
      Text(&context, V2(1120, 160), auto_builder_state.selected_block->name, 20);
      
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
#endif
}