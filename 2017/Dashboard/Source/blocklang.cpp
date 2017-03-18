void DrawCreateBlock(ui_window *window, layout *window_layout, DashboardState *dashstate)
{
	UIContext *context = window_layout->context;
	v2 window_size = GetSize(window_layout->bounds);
   
	element title_text = Text(window_layout, Literal("Create Block"), 20,
							  V2((window_size.x - GetTextWidth(context->render_context, Literal("Create Block"), 20)) / 2.0f, 0),
							  V2(0, 0)); 
	NextLine(window_layout);
   
	layout scroll_bar = Panel(window_layout, V2(window_size.x * 0.1, window_size.y - GetSize(title_text.bounds).y), V2(0, 0), V2(0, 0)).lout;
	SliderBar(&scroll_bar, -1000, 0, &window->create_block.scroll, GetSize(scroll_bar.bounds), V2(0, 0), V2(0, 0));
   
	layout block_list = Panel(window_layout, V2(window_size.x * 0.9, window_size.y - GetSize(title_text.bounds).y), V2(0, 0), V2(0, 0), window->create_block.scroll).lout;
   
	Robot *robot = &dashstate->robot;
	v2 button_size = V2(GetSize(block_list.bounds).x * 0.9, 30);
	CoroutineBlock *coroutine = window->create_block.coroutine;
   
	FunctionBlock block = {};
	b32 block_added = false;
   
	if(Button(&block_list, NULL, Literal("Wait"), button_size, V2(0, 0), V2(5, 5)).state)
	{
		block.type = FunctionBlock_Wait;
		block.wait.duration = 0.0f;	
		block_added = true;
	}
	NextLine(&block_list);
   
	if(Button(&block_list, NULL, Literal("Set Arcade Drive"), button_size, V2(0, 0), V2(5, 5)).state)
	{
		block.type = FunctionBlock_ArcadeDriveConst;
		block.arcade_drive_const.power = 0.0f;	
		block.arcade_drive_const.rotate = 0.0f;	
		block_added = true;
	}
	NextLine(&block_list);
	
	if(Button(&block_list, NULL, Literal("Control Arcade Drive"), button_size, V2(0, 0), V2(5, 5)).state)
	{
		block.type = FunctionBlock_ArcadeDriveController;
		block.arcade_drive_controller.power_axis_index = 0;	
		block.arcade_drive_controller.rotate_axis_index = 1;
		block_added = true;	
	}
	NextLine(&block_list);
	
	if(Button(&block_list, NULL, Literal("Set Drive Multiplier"), button_size, V2(0, 0), V2(5, 5)).state)
	{
		block.type = FunctionBlock_SetDriveMultiplier;
		block.set_drive_multiplier.value = 1.0f;
		block_added = true;	
	}
	NextLine(&block_list);
	
	if(Button(&block_list, NULL, Literal("Drive To Distance"), button_size, V2(0, 0), V2(5, 5)).state)
	{
		block.type = FunctionBlock_DriveDistance;
		block.drive_distance.left_distance = 0.0f;	
		block.drive_distance.right_distance = 0.0f;	
		block_added = true;
	}
	NextLine(&block_list);
	
	for(u32 i = 0;
		i < robot->hardware_count;
		i++)
	{
		RobotHardware *curr_hardware = robot->hardware + i;
	
		if((curr_hardware->type == Hardware_Motor) ||
		   (curr_hardware->type == Hardware_EncoderMotor))
		{
			if(_Button(POINTER_UI_ID(curr_hardware), &block_list, NULL, curr_hardware->name, button_size, V2(0, 0), V2(5, 5)).state)
			{
				block.type = FunctionBlock_SetFloatConst;
				block.set_float_const.hardware_index = i;
				block.set_float_const.value = 0.0f;
				block_added = true;
			}
			NextLine(&block_list);
		
			if(_Button(POINTER_UI_ID(curr_hardware), &block_list, NULL, curr_hardware->name, button_size, V2(0, 0), V2(5, 5)).state)
			{
				block.type = FunctionBlock_SetFloatController;
				block.set_float_controller.hardware_index = i;
				block.set_float_controller.is_op = false;
				block.set_float_controller.axis_index = 0;
				block_added = true;
			}
			NextLine(&block_list);
		
			if(_Button(POINTER_UI_ID(curr_hardware), &block_list, NULL, curr_hardware->name, button_size, V2(0, 0), V2(5, 5)).state)
			{
				block.type = FunctionBlock_SetMultiplier;
				block.set_multiplier.hardware_index = i;
				block.set_multiplier.value = 1.0f;
				block_added = true;
			}
			NextLine(&block_list);
		}
		else if(curr_hardware->type == Hardware_Solenoid)
		{
			if(_Button(POINTER_UI_ID(curr_hardware), &block_list, NULL, curr_hardware->name, button_size, V2(0, 0), V2(5, 5)).state)
			{
				block.type = FunctionBlock_SetBool;
				block.set_bool.hardware_index = i;
				block.set_bool.op = BooleanOp_False;
				block_added = true;
			}
			NextLine(&block_list);
		}
	}
   
	if(block_added)
	{
		FunctionBlockLink *new_block_link = (FunctionBlockLink *) malloc(sizeof(FunctionBlockLink));
		new_block_link->next = NULL;
		new_block_link->block = block;
		
		if(coroutine->first_block)
		{
			FunctionBlockLink *last_block = coroutine->first_block;
			while(last_block->next)
			{
				last_block = last_block->next;
			}
			last_block->next = new_block_link;
		}
		else
		{
			coroutine->first_block = new_block_link;
		}
	}
}

void DrawEditBlock(ui_window *window, layout *window_layout, DashboardState *dashstate)
{
   UIContext *context = window_layout->context;
   v2 window_size = GetSize(window_layout->bounds);
   
   Text(window_layout, Literal("Edit Block"), 20,
      V2((window_size.x - GetTextWidth(context->render_context, Literal("Edit Block"), 20)) / 2.0f, 0),
      V2(0, 0)); 
   NextLine(window_layout);
     
   FunctionBlockLink *function_link = window->edit_block.function;
   
   TemporaryMemoryArena temp_memory = BeginTemporaryMemory(dashstate->generic_arena);
   
	switch(function_link->block.type)
	{
		case FunctionBlock_Wait:
		{
			Text(window_layout, Concat(Literal("Wait for "), ToString(function_link->block.wait.duration, &temp_memory), Literal(" seconds"), &temp_memory), 20, V2(0, 0), V2(0, 5));
			NextLine(window_layout);
			SliderBar(window_layout, 0, 15, &function_link->block.wait.duration, V2(GetSize(window_layout->bounds).x, 20), V2(0, 0), V2(0, 0));
			NextLine(window_layout);
			TextBox(window_layout, 0, 15, &function_link->block.wait.duration, V2(GetSize(window_layout->bounds).x, 20), V2(0, 0), V2(0, 0));
		}
		break;
		
		case FunctionBlock_SetFloatConst:
		{
			RobotHardware *motor = dashstate->robot.hardware + function_link->block.set_float_const.hardware_index;
			string text = Concat(Literal("Set "), motor->name,
								 Literal(" to "), ToString(function_link->block.set_float_const.value, &temp_memory),
								 (motor->type == Hardware_EncoderMotor) ? Literal("RPM") : Literal(""), &temp_memory);
			Text(window_layout, text, 20, V2(0, 0), V2(0, 5));
			NextLine(window_layout);
			
			if(motor->type == Hardware_EncoderMotor)
			{
				TextBox(window_layout, &function_link->block.set_float_const.value, V2(GetSize(window_layout->bounds).x, 20), V2(0, 0), V2(0, 0));
			}
			else
			{
				SliderBar(window_layout, -1, 1, &function_link->block.set_float_const.value, V2(GetSize(window_layout->bounds).x, 20), V2(0, 0), V2(0, 0));
				NextLine(window_layout);
				TextBox(window_layout, -1, 1, &function_link->block.set_float_const.value, V2(GetSize(window_layout->bounds).x, 20), V2(0, 0), V2(0, 0));
			}
		}
		break;
		
		case FunctionBlock_SetFloatController:
		{
			
		}
		break;
		
		case FunctionBlock_SetMultiplier:
		{
			
		}
		break;
		
		case FunctionBlock_SetBool:
		{
			
		}
		break;
		
		case FunctionBlock_ArcadeDriveConst:
		{
			
		}
		break;
		
		case FunctionBlock_ArcadeDriveController:
		{
			
		}
		break;
		
		case FunctionBlock_SetDriveMultiplier:
		{
			
		}
		break;
		
		case FunctionBlock_DriveDistance:
		{
			Text(window_layout, Literal("Left Distance"), 20, V2(0, 0), V2(0, 5));
			NextLine(window_layout);
			TextBox(window_layout, &function_link->block.drive_distance.left_distance, V2(GetSize(window_layout->bounds).x, 20), V2(0, 0), V2(0, 0));
			NextLine(window_layout);
			
			Text(window_layout, Literal("Right Distance"), 20, V2(0, 0), V2(0, 5));
			NextLine(window_layout);
			TextBox(window_layout, &function_link->block.drive_distance.right_distance, V2(GetSize(window_layout->bounds).x, 20), V2(0, 0), V2(0, 0));
		}
		break;
		
		case FunctionBlock_GotoPosition:
			break;
	}
   
	if(Button(window_layout, NULL, Literal("Delete"), V2(60, 20), V2(0, 0), V2(5, 5)).state)
	{
		if(window->edit_block.coroutine->first_block == window->edit_block.function)
		{
			window->edit_block.coroutine->first_block = window->edit_block.function->next;
		}
		else
		{
			FunctionBlockLink *parent_link = window->edit_block.coroutine->first_block;
			while(parent_link)
			{
				if(parent_link->next == window->edit_block.function)
				{
					break;
				}
				parent_link = parent_link->next;
			}
			parent_link->next = window->edit_block.function->next;
		}
		
		free(window->edit_block.function);
		window->flags |= Flag_CloseRequested;
	}
	
	EndTemporaryMemory(temp_memory);
}

void DrawFunctionBlock(FunctionBlockLink *block_link, rect2 bounds, layout *editor_panel, DashboardState *dashstate, CoroutineBlock *coroutine)
{
   UIContext *context = editor_panel->context;
   RenderContext *render_context = context->render_context;
   AutonomousEditor *auto_editor = &dashstate->auto_editor;
   
   interaction_state block_interact =
      ClickInteraction(context, Interaction(POINTER_UI_ID(block_link), editor_panel), context->input_state.left_up,
                     context->input_state.left_down, Contains(bounds, context->input_state.pos));
   
   if(block_interact.became_selected)
   {
      if(!auto_editor->edit_block_window)
      {
         auto_editor->edit_block_window = AddWindow(dashstate, V2(250, 300), context->input_state.pos, WindowType_EditBlock);
      }
      
      auto_editor->edit_block_window->edit_block.function = block_link;
	  auto_editor->edit_block_window->edit_block.coroutine = coroutine;
   }
   
   v4 identifier_color = V4(0, 0, 0, 1);
   
   Rectangle(render_context, RectMinSize(bounds.min, V2(GetSize(bounds).x, GetSize(bounds).x / 2)), identifier_color);
   Rectangle(render_context, RectMinSize(bounds.min + V2(0, (GetSize(bounds).x / 2)), V2(GetSize(bounds).x, GetSize(bounds).y - (GetSize(bounds).x / 2))),
			 block_interact.hot ? V4(1, 0, 0, 0.8) : V4(1, 0, 0, 0.6));
   
   if(auto_editor->edit_block_window && 
      (auto_editor->edit_block_window->edit_block.function == block_link))
   {
      RectangleOutline(render_context, bounds, V4(0, 0, 0, 1), 2);
   }
   
	TemporaryMemoryArena temp_memory = BeginTemporaryMemory(dashstate->generic_arena);
	string title = Literal("???");
   
	switch(block_link->block.type)
	{
		case FunctionBlock_Wait:
			title = Concat(Literal("Wait "), ToString(block_link->block.wait.duration, &temp_memory), &temp_memory);
			break;
      
		case FunctionBlock_SetFloatConst:
			title = Literal("Set Motor ");
			break;
   
		case FunctionBlock_SetFloatController:
			title = Literal("Control Motor ");
			break;
   
		default:
			break;
	}
   
    Text/*Vertical*/(render_context, bounds, title);
	EndTemporaryMemory(temp_memory);
}

void DrawCoroutine(CoroutineBlock *coroutine, DashboardState *dashstate,
                   v2 pos, layout *editor_panel, UIContext *context)
{
	AutonomousEditor *auto_editor = &dashstate->auto_editor;
	RenderContext *render_context = context->render_context;
   
	v2 function_block_size = V2(40, 120);
	u32 block_count = 0;
   
	FunctionBlockLink *last_block = coroutine->first_block;
	while(last_block) { last_block = last_block->next; block_count++; }
   
   Rectangle(render_context, RectMinSize(pos, V2(10, 100)), V4(0, 0, 0, 1));
   Rectangle(render_context, RectMinSize(pos + V2(10, 0), V2(function_block_size.x * (block_count + 1), 10)), V4(0, 0, 0, 1));
   Rectangle(render_context, RectMinSize(pos + V2(function_block_size.x * (block_count + 1) + 10, 0), V2(10, 100)), V4(0, 0, 0, 1));
	
	u32 i = 0;   
	for(FunctionBlockLink *curr_block_link = coroutine->first_block;
		curr_block_link;
		curr_block_link = curr_block_link->next)
	{
		rect2 block_bounds = RectMinSize(pos + V2(10 + function_block_size.x * i++, 10), function_block_size);
		DrawFunctionBlock(curr_block_link, block_bounds, editor_panel, dashstate, coroutine);
	}
	
	rect2 ghost_block_bounds = RectMinSize(pos + V2(10 + function_block_size.x * block_count, 10), function_block_size);
	interaction_state ghost_block_interact =
		ClickInteraction(context, Interaction(GEN_UI_ID, editor_panel), context->input_state.left_up,
						 context->input_state.left_down, Contains(ghost_block_bounds, context->input_state.pos));
	
	Rectangle(render_context, ghost_block_bounds, ghost_block_interact.hot ? V4(1, 0, 0, 0.8) : V4(1, 0, 0, 0.6));
	
	if(ghost_block_interact.became_selected)
	{
		if(!auto_editor->create_block_window)
		{
			auto_editor->create_block_window = AddWindow(dashstate, V2(200, 100), context->input_state.pos, WindowType_CreateBlock);
		}
	
		auto_editor->create_block_window->create_block.coroutine = coroutine;
	}
}