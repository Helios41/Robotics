void DrawCreateBlock(ui_window *window, layout *window_layout, DashboardState *dashstate)
{
   UIContext *context = window_layout->context;
   v2 window_size = GetSize(window_layout->bounds);
   
   element title_text = Text(window_layout, Literal("Create Block"), 20,
                           V2((window_size.x - GetTextWidth(context->render_context, Literal("Create Block"), 20)) / 2.0f, 0),
                           V2(0, 0)); 
   NextLine(window_layout);
   
   layout scroll_bar = Panel(window_layout, V2(window_size.x * 0.1, window_size.y - GetSize(title_text.bounds).y), V2(0, 0), V2(0, 0)).lout;
   SliderBar(&scroll_bar, -100, 0, &window->create_block.scroll, GetSize(scroll_bar.bounds), V2(0, 0), V2(0, 0));
   
   layout block_list = Panel(window_layout, V2(window_size.x * 0.9, window_size.y - GetSize(title_text.bounds).y), V2(0, 0), V2(0, 0), window->create_block.scroll).lout;
   
   Robot *robot = &dashstate->robot;
   v2 button_size = V2(GetSize(block_list.bounds).x * 0.9, 30);
   CoroutineBlock *coroutine = window->create_block.coroutine;
   
   if(Button(&block_list, NULL, Literal("Wait"), button_size, V2(0, 0), V2(5, 5)).state)
   {
	  FunctionBlock block = {};
	  block.type = FunctionBlock_Wait;
	  block.wait.duration = 0.0f;	
      coroutine->blocks[coroutine->block_count++] = block;
   }
   NextLine(&block_list);
   
   if(Button(&block_list, NULL, Literal("Arcade Drive"), button_size, V2(0, 0), V2(5, 5)).state)
   {
	   //TODO: we removed value blocks, so how do we use controller input?
	  FunctionBlock block = {};
	  block.type = FunctionBlock_ArcadeDrive;
	  block.arcade_drive.power = 0.0f;	
	  block.arcade_drive.rotate = 0.0f;	
      coroutine->blocks[coroutine->block_count++] = block;
   }
   NextLine(&block_list);
   
   if(Button(&block_list, NULL, Literal("Set Drive Multiplier"), button_size, V2(0, 0), V2(5, 5)).state)
   {
	  FunctionBlock block = {};
	  block.type = FunctionBlock_SetDriveMultiplier;
	  block.set_drive_multiplier.value = 1.0f;	
      coroutine->blocks[coroutine->block_count++] = block;
   }
   NextLine(&block_list);
   
   for(u32 i = 0;
      i < robot->hardware_count;
      i++)
   {
      RobotHardware *curr_hardware = robot->hardware + i;
      
	  if((curr_hardware->type == Hardware_Motor) ||
		 (curr_hardware->type == Hardware_EncoderMotor) ||
		 (curr_hardware->type == Hardware_LimitMotor))
      {
         if(_Button(POINTER_UI_ID(curr_hardware), &block_list, NULL, curr_hardware->name, button_size, V2(0, 0), V2(5, 5)).state)
         {
			FunctionBlock block = {};
			block.type = FunctionBlock_SetFloat;
			block.set_float.hardware_index = i;
			block.set_float.value = 0.0f;
            coroutine->blocks[coroutine->block_count++] = block;
         }
         NextLine(&block_list);
		 
		 if(_Button(POINTER_UI_ID(curr_hardware), &block_list, NULL, curr_hardware->name, button_size, V2(0, 0), V2(5, 5)).state)
         {
			FunctionBlock block = {};
			block.type = FunctionBlock_SetMultiplier;
			block.set_multiplier.hardware_index = i;
			block.set_multiplier.value = 1.0f;
            coroutine->blocks[coroutine->block_count++] = block;
         }
         NextLine(&block_list);
      }
	  else if((curr_hardware->type == Hardware_Solenoid) ||
			  (curr_hardware->type == Hardware_Light))
	  {
		 if(_Button(POINTER_UI_ID(curr_hardware), &block_list, NULL, curr_hardware->name, button_size, V2(0, 0), V2(5, 5)).state)
         {
			FunctionBlock block = {};
			block.type = FunctionBlock_SetBool;
			block.set_bool.hardware_index = i;
			block.set_bool.op = BooleanOp_False;
            coroutine->blocks[coroutine->block_count++] = block;
		 }
         NextLine(&block_list);
	  }
   }
   
   for(u32 i = 0;
      i < robot->function_count;
      i++)
   {
      RobotBuiltinFunction *curr_function = robot->functions + i;
      if(_Button(POINTER_UI_ID(curr_function), &block_list, NULL, curr_function->name, button_size, V2(0, 0), V2(5, 5)).state)
      {
		 FunctionBlock block = {};
		 block.type = FunctionBlock_Builtin;
		 block.builtin.index = i;
         coroutine->blocks[coroutine->block_count++] = block;      
      }
      NextLine(&block_list);
   }
   
   /*
   for(u32 i = 0;
      i < vision->config_count;
      i++)
   {
      VisionConfig *curr_config = vision->configs + i;
      if(_Button(POINTER_UI_ID(curr_config), &block_list, NULL, Literal("*"), button_size, V2(0, 0), V2(5, 5)).state)
      {
         coroutine->blocks[coroutine->block_count++] = VisionBlock(curr_config);
	  }
      NextLine(&block_list);
   }
   */
   
    if(coroutine->block_count == ArrayCount(coroutine->blocks))
    {
		window->flags |= Flag_CloseRequested;
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
     
   FunctionBlock *function = window->edit_block.function;
   
   switch(function->type)
   {
	   case FunctionBlock_Wait:
	   case FunctionBlock_SetFloat:
       case FunctionBlock_SetBool:
	   case FunctionBlock_SetMultiplier:
	   case FunctionBlock_Builtin:
	   case FunctionBlock_Vision:
	   case FunctionBlock_ArcadeDrive:
	   case FunctionBlock_SetDriveMultiplier:
	      break;
   }
   
   if(Button(window_layout, NULL, Literal("Delete"), V2(60, 20), V2(0, 0), V2(5, 5)).state)
   {
      
   }
}

void DrawFunctionBlock(FunctionBlock *block, rect2 bounds, layout *editor_panel, DashboardState *dashstate)
{
   UIContext *context = editor_panel->context;
   RenderContext *render_context = context->render_context;
   AutonomousEditor *auto_editor = &dashstate->auto_editor;
   
   interaction_state block_interact =
      ClickInteraction(context, Interaction(POINTER_UI_ID(block), editor_panel), context->input_state.left_up,
                     context->input_state.left_down, Contains(bounds, context->input_state.pos));
   
   if(block_interact.became_selected)
   {
      if(!auto_editor->edit_block_window)
      {
         auto_editor->edit_block_window = AddWindow(dashstate, V2(250, 300), context->input_state.pos, WindowType_EditBlock);
      }
      
      auto_editor->edit_block_window->edit_block.function = block;
   }
   
   Rectangle(render_context, bounds, block_interact.hot ? V4(1, 0, 0, 0.8) : V4(1, 0, 0, 0.6));
   
   if(auto_editor->edit_block_window && 
      (auto_editor->edit_block_window->edit_block.function == block))
   {
      RectangleOutline(render_context, bounds, V4(0, 0, 0, 1), 2);
   }
   
   //TODO: proper titles
   switch(block->type)
   {
      case FunctionBlock_Wait:
         Text(render_context, bounds, Literal("Wait"));
         break;
      
      case FunctionBlock_SetFloat:
         Text(render_context, bounds, Literal("*"));
         break;
         
      case FunctionBlock_Builtin:
         Text(render_context, bounds, Literal("*"));
         break;
         
      case FunctionBlock_Vision:
         Text(render_context, bounds, Literal("Vision"));
         break;
   
      default:
         break;
   }
}

void DrawCoroutine(CoroutineBlock *coroutine, DashboardState *dashstate,
                   v2 pos, layout *editor_panel, UIContext *context)
{
   AutonomousEditor *auto_editor = &dashstate->auto_editor;
   RenderContext *render_context = context->render_context;
   
   v2 function_block_size = V2(120, 40);
   u32 block_count = coroutine->block_count;
   
   b32 draw_ghost_block = coroutine->block_count < ArrayCount(coroutine->blocks);
   
   Rectangle(render_context, RectMinSize(pos, V2(100, 10)), V4(0, 0, 0, 1));
   Rectangle(render_context, RectMinSize(pos + V2(0, 10), V2(10, function_block_size.y * (draw_ghost_block ? block_count + 1 : block_count))), V4(0, 0, 0, 1));
   Rectangle(render_context, RectMinSize(pos + V2(0, function_block_size.y * (draw_ghost_block ? block_count + 1 : block_count) + 10), V2(100, 10)), V4(0, 0, 0, 1));
   
   for(u32 i = 0;
       i < block_count;
       i++)
   {
      FunctionBlock *curr_block = coroutine->blocks + i;
      rect2 block_bounds = RectMinSize(pos + V2(10, 10 + function_block_size.y * i), function_block_size);
      
      DrawFunctionBlock(curr_block, block_bounds, editor_panel, dashstate);
   }
   
   if(draw_ghost_block)
   {
      rect2 ghost_block_bounds = RectMinSize(pos + V2(10, 10 + function_block_size.y * block_count), function_block_size);
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
}