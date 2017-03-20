struct auto_save_file_header
{
	char name[32];
	u32 block_count;
};

void SaveAutoFile(AutonomousEditor *auto_builder, MemoryArena *generic_arena)
{
	TemporaryMemoryArena temp_memory = BeginTemporaryMemory(generic_arena);
	
	char *file_name = ToCString(Concat(auto_builder->name, Literal(".abin2"), &temp_memory), &temp_memory);
	FILE *auto_save_file = fopen(file_name, "wb");
	
	if(auto_save_file)
	{
		u32 block_count = 0;
		FunctionBlockLink *last_link = auto_builder->coroutine.first_block;
		while(last_link){ last_link = last_link->next; block_count++; };
		
		auto_save_file_header save_file_header = {}; 
		CopyTo(auto_builder->name, String(save_file_header.name, ArrayCount(save_file_header.name))); 
		save_file_header.block_count = block_count;
		
		fwrite(&save_file_header, sizeof(save_file_header), 1, auto_save_file);
		
		FunctionBlock *function_blocks = (FunctionBlock *) malloc(sizeof(FunctionBlock) * block_count);
		
		u32 i = 0;
		for(FunctionBlockLink *curr_block_link = auto_builder->coroutine.first_block;
			curr_block_link;
			curr_block_link = curr_block_link->next)
		{
			function_blocks[i++] = curr_block_link->block;
		}
		
		fwrite(function_blocks, sizeof(FunctionBlock) * block_count, 1, auto_save_file);
		fclose(auto_save_file);
	}
	
	EndTemporaryMemory(temp_memory);
}

void LoadAutoFile(AutonomousEditor *auto_builder, MemoryArena *generic_arena)
{
	TemporaryMemoryArena temp_memory = BeginTemporaryMemory(generic_arena);
	
	char *file_name = ToCString(Concat(auto_builder->name, Literal(".abin2"), &temp_memory), &temp_memory);
	FILE *auto_save_file = fopen(file_name, "rb");
	
	if(auto_save_file)
	{
		auto_save_file_header save_file_header = {};
		fread(&save_file_header, sizeof(save_file_header), 1, auto_save_file);
	
		FunctionBlock *function_blocks = (FunctionBlock *) malloc(sizeof(FunctionBlock) * save_file_header.block_count);		
		fread(function_blocks, sizeof(FunctionBlock) * save_file_header.block_count, 1, auto_save_file);
		
		for(u32 i = 0;
			i < save_file_header.block_count;
			i++)
		{
			AddBlock(&auto_builder->coroutine, function_blocks[i]);
		}
		
		fclose(auto_save_file);
	}
	
	EndTemporaryMemory(temp_memory);
}

void DrawEditorMenu(layout *editor_menu, AutonomousEditor *auto_editor, NetworkState *net_state, DashboardState *dashstate)
{
   RenderContext *render_context = editor_menu->context->render_context;
   
   Rectangle(render_context, editor_menu->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, editor_menu->bounds, V4(0, 0, 0, 1));
   
   v2 button_size = V2(GetSize(editor_menu->bounds).x * 0.9, 40);
   v2 button_margin = V2(GetSize(editor_menu->bounds).x * 0.05, 5);
   
   //TODO: make these buttons work
   if(Button(editor_menu, NULL, Literal("Open"), button_size, V2(0, 0), button_margin).state)
   {
	   LoadAutoFile(auto_editor, dashstate->generic_arena);	   
   }
   
   if(Button(editor_menu, NULL, Literal("Save"), button_size, V2(0, 0), button_margin).state)
   {
	   SaveAutoFile(auto_editor, dashstate->generic_arena);
   }
   
   if(Button(editor_menu, NULL, Literal("Upload"), button_size, V2(0, 0), button_margin).state)
   {
	   UploadAutonomous(net_state, auto_editor);
   }
   
	if(Button(editor_menu, NULL, Literal("Record"), auto_editor->is_recording, button_size, V2(0, 0), button_margin).state)
	{
		auto_editor->is_recording = !auto_editor->is_recording;
	   
		if(auto_editor->is_recording)
		{
			auto_editor->recording_timer_start = editor_menu->context->curr_time;
			auto_editor->earliest_sample_timestamp = dashstate->latest_sample_timestamp;
		}
   }
   
   if(Button(editor_menu, NULL, Literal("Clear"), button_size, V2(0, 0), button_margin).state)
   {
	   FunctionBlockLink *block_link = auto_editor->coroutine.first_block;
	   
	   while(block_link)
	   {
		   FunctionBlockLink *curr_block_link = block_link;
		   block_link = block_link->next;
		   DeleteBlock(&auto_editor->coroutine, curr_block_link);
	   }
	   
		if(dashstate->auto_editor.edit_block_window)
		{
			dashstate->auto_editor.edit_block_window->flags |= Flag_CloseRequested;
		}
   }
}

void DrawGraphView(layout *graph_view, AutonomousEditor *auto_editor, MemoryArena *generic_arena)
{
	RenderContext *render_context = graph_view->context->render_context;
	rect2 view_bounds = graph_view->bounds;
	rect2 graph_bounds = RectPosSize(GetCenter(view_bounds), GetSize(view_bounds) - V2(10, 10));
   
	Rectangle(render_context, view_bounds, V4(0.3, 0.3, 0.3, 0.6));
	RectangleOutline(render_context, view_bounds, V4(0, 0, 0, 1));
	RectangleOutline(render_context, graph_bounds, V4(0, 0, 0, 1));
 
	if(auto_editor->is_recording)
	{ 
		r32 time_elapsed = graph_view->context->curr_time - auto_editor->recording_timer_start;
		TemporaryMemoryArena temp_memory = BeginTemporaryMemory(generic_arena);

		Text(render_context, graph_bounds.min, Concat(Literal("Time Elapsed: "), ToString(time_elapsed, &temp_memory), &temp_memory), 20);
   
		EndTemporaryMemory(temp_memory);
		
		if(time_elapsed >= 15)
		{
			auto_editor->is_recording = false;
		}
	}
	
	//TODO: timeline rendering
}

void DrawBlockEditor(layout *editor_panel, DashboardState *dashstate,
                     MemoryArena *generic_arena)
{
   UIContext *context = editor_panel->context;
   RenderContext *render_context = context->render_context;
   
   RectangleOutline(render_context, editor_panel->bounds, V4(0, 0, 0, 1));
   Rectangle(render_context, editor_panel->bounds, V4(0.3, 0.3, 0.3, 0.6));
   
   DrawCoroutine(&dashstate->auto_editor.coroutine, dashstate,
                 editor_panel->bounds.min + V2(5, 5), editor_panel, context);
}

void DrawAutonomousEditor(layout *auto_editor, UIContext *context, DashboardState *dashstate)
{
	RenderContext *render_context = context->render_context;
	v2 editor_size = GetSize(auto_editor->bounds);
	b32 was_recording = dashstate->auto_editor.is_recording;
   
	layout timeline_view = Panel(auto_editor, V2(editor_size.x * 0.95 - 7.5, editor_size.y * 0.5), V2(0, 0), V2(5, 0)).lout;
	layout menu_bar = Panel(auto_editor, V2(editor_size.x * 0.05 - 7.5, editor_size.y * 0.5), V2(0, 0), V2(5, 0)).lout;
	NextLine(auto_editor);
 
	panel title_box = Panel(auto_editor, V2(editor_size.x, editor_size.y * 0.1) - V2(10, 10), V2(0, 0), V2(5, 5));
	Rectangle(render_context, title_box.elem.bounds, V4(0.3, 0.3, 0.3, 0.6));
	TextBox(&title_box.lout, dashstate->auto_editor.name, GetSize(title_box.elem.bounds), V2(0, 0), V2(0, 0));
	
	layout block_editor = Panel(auto_editor, V2(editor_size.x, editor_size.y * 0.4) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   
	DrawGraphView(&timeline_view, &dashstate->auto_editor, dashstate->generic_arena);
	DrawEditorMenu(&menu_bar, &dashstate->auto_editor, dashstate->net_state, dashstate);
	DrawBlockEditor(&block_editor, dashstate, dashstate->generic_arena);
	
	if(was_recording && !dashstate->auto_editor.is_recording)
	{
		Robot *robot = &dashstate->robot;
		u32 do_not_include[] = {0 /*turntable*/, 1 /*shooter*/, 3 /*indexer*/, 5 /*shooter swivel*/, 9 /*pot*/};
		
		TemporaryMemoryArena temp_memory = BeginTemporaryMemory(dashstate->generic_arena);
		
		earliest_sample_result earliest_sample = GetEarliestSampleFromAll(&dashstate->robot, dashstate->auto_editor.earliest_sample_timestamp, do_not_include, ArrayCount(do_not_include));
		if(earliest_sample.success)
		{
			u64 earliest_sample_timestamp = GetTimestamp(earliest_sample);
			u64 latest_sample_timestamp = dashstate->latest_sample_timestamp;
			
			std::vector<robot_sample> *sample_list = ListSamplesFromTo(&dashstate->robot, earliest_sample_timestamp, latest_sample_timestamp,
																	   do_not_include, ArrayCount(do_not_include));
			
			for(u32 i = 0;
				i < sample_list->size();
				i++)
			{
				robot_sample curr = (*sample_list)[i];
				
				if(!curr.is_drive_sample)
				{
					FunctionBlock block = {};
					
					if(robot->hardware[curr.hardware.index].type == Hardware_Solenoid)
					{
						block.type = FunctionBlock_SetBool;
						block.set_bool.hardware_index = curr.hardware.index;
						block.set_bool.op = curr.hardware.sample.solenoid ? BooleanOp_True : BooleanOp_False;
					}
					else if((robot->hardware[curr.hardware.index].type == Hardware_Motor) ||
							(robot->hardware[curr.hardware.index].type == Hardware_EncoderMotor))
					{
						block.type = FunctionBlock_SetFloatConst;
						block.set_float_const.hardware_index = i;
						block.set_float_const.value = curr.hardware.sample.motor;
					}
					
					AddBlock(&dashstate->auto_editor.coroutine, block);
					
					if((i + 1) < sample_list->size())
					{
						robot_sample next_sample = (*sample_list)[i + 1];
						u64 timestamp_diff = GetTimestamp(next_sample) - GetTimestamp(curr);
						
						FunctionBlock wait_block = {};
						wait_block.type = FunctionBlock_Wait;
						wait_block.wait.duration = (r32)timestamp_diff / 1000.0f;
					
						AddBlock(&dashstate->auto_editor.coroutine, wait_block);	
					}
				}
				else
				{
					b32 turn_motion = Abs(curr.drive_sample.left - curr.drive_sample.right) > 1000;
					
					r32 left_acc = curr.drive_sample.left;
					u32 left_inc = 1;
					
					r32 right_acc = curr.drive_sample.right;
					u32 right_inc = 1;
					
					u64 end_sample_timestamp = GetTimestamp(curr);
					u32 samples_consumed = 0;
					
					for(u32 j = (i + 1);
						j < sample_list->size();
						j++)
					{
						robot_sample j_sample = (*sample_list)[j];
						b32 run_ended = true;
						
						if(j_sample.is_drive_sample)
						{
							b32 j_sample_turn = Abs(j_sample.drive_sample.left - j_sample.drive_sample.right) > 1000;
							run_ended = (j_sample_turn != turn_motion);
							
							if(!run_ended)
							{
								left_acc += j_sample.drive_sample.left;
								left_inc++;
								
								right_acc += j_sample.drive_sample.right;
								right_inc++;
								
								end_sample_timestamp = GetTimestamp(j_sample);
								samples_consumed++;
							}
						}
						
						if(run_ended)
							break;
					}
					
					FunctionBlock drive_block = {};
						
					if(turn_motion)
					{
					
					}
					else
					{
						drive_block.type = FunctionBlock_DriveDistance;
						drive_block.drive_distance.left_distance = (((left_acc / left_inc) * (r32)(end_sample_timestamp - GetTimestamp(curr))) / -1173.0f) / 1173.0f;
						drive_block.drive_distance.right_distance = (((right_acc / right_inc) * (r32)(end_sample_timestamp - GetTimestamp(curr))) / -1173.0f) / 1173.0f;
						
						AddBlock(&dashstate->auto_editor.coroutine, drive_block);	
					}
					
					i += samples_consumed;
				}
			}
			
			string auto_compile_msg = Concat(Literal("Compiling Autonomous From "), ToString(earliest_sample_timestamp, &temp_memory),
											 Literal(" to "), ToString(latest_sample_timestamp, &temp_memory), Literal(" Count = "),
											 ToString(sample_list->size(), &temp_memory), Literal("C"),
											 &temp_memory);
			
			string message = String((char *) malloc(sizeof(char) * auto_compile_msg.length), auto_compile_msg.length);
			CopyTo(auto_compile_msg, message);
			
			AddMessage(&dashstate->console, message);
			delete sample_list;
		}
		
		EndTemporaryMemory(temp_memory);
	}
}