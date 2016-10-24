void DrawAutonomousEditor(layout *top_bar, UIContext *context, DashboardState *dashstate)
{
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