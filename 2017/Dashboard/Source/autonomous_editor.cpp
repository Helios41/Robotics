FunctionBlock WaitBlock()
{
   FunctionBlock result = {};
   result.type = FunctionBlock_Wait;
   result.param.type = ValueBlock_Constant_Float;
   return result;
}

FunctionBlock SetBlock(RobotHardware *hardware)
{
   FunctionBlock result = {};
   result.type = FunctionBlock_Set;
   result.id = hardware->id;
   
   switch(hardware->type)
   {
      case Hardware_Motor:
         result.param.type = ValueBlock_Constant_Float;
         break;
         
      case Hardware_Solenoid:
         result.param.type = ValueBlock_Constant_Bool;
         break;
         
      case Hardware_Drive:
         result.param.type = ValueBlock_Constant_Vec2;
         break;
   }
   
   return result;
}

FunctionBlock BuiltinBlock(RobotBuiltinFunction *builtin_function)
{
   FunctionBlock result = {};
   result.type = FunctionBlock_Builtin;
   result.id = builtin_function->id;
   return result;
}

FunctionBlock VisionBlock(VisionConfig *vision_config)
{
   FunctionBlock result = {};
   result.type = FunctionBlock_Vision;
   result.id = vision_config->camera_id;
   result.param.type = ValueBlock_Constant_Uint;
   result.param.uint_param = vision_config->config_id;
   return result;
}

//TODO: pass the drive train
/*
FunctionBlock GotoBlock()
{
   FunctionBlock result = {};
   result.type = FunctionBlock_Goto;
   result.id = drive->id;
   result.param.type = ValueBlock_Constant_Vec2;
   return result;
}
*/

void DrawCreateBlock(ui_window *window, layout *window_layout, DashboardState *dashstate)
{
   UIContext *context = window_layout->context;
   v2 window_size = GetSize(window_layout->bounds);
   
   element title_text = Text(window_layout, Literal("Create Block"), 20,
                           V2((window_size.x - GetTextWidth(context->render_context, Literal("Create Block"), 20)) / 2.0f, 0),
                           V2(0, 0)); 
   NextLine(window_layout);
   
   layout scroll_bar = Panel(window_layout, V2(window_size.x * 0.1, window_size.y - GetSize(title_text.bounds).y), V2(0, 0), V2(0, 0)).lout;
   SliderBar(&scroll_bar, -100, 0, &window->scroll, GetSize(scroll_bar.bounds), V2(0, 0), V2(0, 0));
   
   layout block_list = Panel(window_layout, V2(window_size.x * 0.9, window_size.y - GetSize(title_text.bounds).y), V2(0, 0), V2(0, 0), window->scroll).lout;
   
   Robot *robot = &dashstate->robot;
   v2 button_size = V2(GetSize(block_list.bounds).x * 0.9, 30);
   CoroutineBlock *coroutine = window->coroutine;
   
   if(Button(&block_list, NULL, Literal("Wait"), button_size, V2(0, 0), V2(5, 5)).state)
   {
      coroutine->blocks[coroutine->block_count++] = WaitBlock();
      
      if(coroutine->block_count == ArrayCount(coroutine->blocks))
      {
         window->flags |= Flag_CloseRequested;
      }
   }
   NextLine(&block_list);
   
   if(Button(&block_list, NULL, Literal("Goto"), button_size, V2(0, 0), V2(5, 5)).state)
   {
      //coroutine->blocks[coroutine->block_count++] = GotoBlock();
      
      if(coroutine->block_count == ArrayCount(coroutine->blocks))
      {
         window->flags |= Flag_CloseRequested;
      }
   }
   NextLine(&block_list);
   
   for(u32 i = 0;
      i < robot->hardware_count;
      i++)
   {
      RobotHardware *curr_hardware = robot->hardware + i;
      if((curr_hardware->type == Hardware_Motor) ||
         (curr_hardware->type == Hardware_Solenoid) ||
         (curr_hardware->type == Hardware_Drive))
      {
         if(_Button(POINTER_UI_ID(curr_hardware), &block_list, NULL, curr_hardware->name, button_size, V2(0, 0), V2(5, 5)).state)
         {
            coroutine->blocks[coroutine->block_count++] = SetBlock(curr_hardware);
            
            if(coroutine->block_count == ArrayCount(coroutine->blocks))
            {
               window->flags |= Flag_CloseRequested;
            }
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
         coroutine->blocks[coroutine->block_count++] = BuiltinBlock(curr_function);
            
         if(coroutine->block_count == ArrayCount(coroutine->blocks))
         {
            window->flags |= Flag_CloseRequested;
         }
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
            
         if(coroutine->block_count == ArrayCount(coroutine->blocks))
         {
            window->flags |= Flag_CloseRequested;
         }
      }
      NextLine(&block_list);
   }
   */
}

void DrawValueBlock(ValueBlock *block, layout *window_layout)
{
   if(block->type == ValueBlock_Constant_Float)
   {
      Text(window_layout, Literal("Float Param: "), 20, V2(0, 0), V2(0, 5)); 
      TextBox(window_layout, 0, 15, &block->float_param, V2(100, 40), V2(0, 0), V2(0, 5)); 
      NextLine(window_layout);
      SliderBar(window_layout, 0, 15, &block->float_param, V2(180, 20), V2(0, 0), V2(0, 0));
   }
   else if(block->type == ValueBlock_Constant_Bool)
   {
      Text(window_layout, block->bool_param ? Literal("Bool Param: True") : Literal("Bool Param: False"),
           20, V2(0, 0), V2(0, 5)); 
      NextLine(window_layout);
      ToggleSlider(window_layout, &block->bool_param, Literal("True"), Literal("False"),
                   V2(200, 30), V2(0, 0), V2(0, 0));  
   }
   else if(block->type == ValueBlock_Constant_Vec2)
   {
      Text(window_layout, Literal("X: "), 20, V2(0, 0), V2(0, 5)); 
      TextBox(window_layout, 0, 15, &block->vec2_param.x, V2(100, 40), V2(0, 0), V2(0, 5)); 
      NextLine(window_layout);
      SliderBar(window_layout, 0, 15, &block->vec2_param.x, V2(180, 20), V2(0, 0), V2(0, 0));
      NextLine(window_layout);
      
      Text(window_layout, Literal("Y: "), 20, V2(0, 0), V2(0, 5)); 
      TextBox(window_layout, 0, 15, &block->vec2_param.y, V2(100, 40), V2(0, 0), V2(0, 5)); 
      NextLine(window_layout);
      SliderBar(window_layout, 0, 15, &block->vec2_param.y, V2(180, 20), V2(0, 0), V2(0, 0));
   }
}
b32 IsBool(ValueBlockType type)
{
   return (type == ValueBlock_Constant_Bool) ||
          (type == ValueBlock_Controller_Bool) ||
          (type == ValueBlock_Sensor_Bool);
}

b32 IsFloat(ValueBlockType type)
{
   return (type == ValueBlock_Constant_Float) ||
          (type == ValueBlock_Controller_Float) ||
          (type == ValueBlock_Sensor_Float);
}

b32 IsUint(ValueBlockType type)
{
   return type == ValueBlock_Constant_Uint;
}
   
b32 IsVec2(ValueBlockType type)
{
   return (type == ValueBlock_Constant_Vec2) ||
          (type == ValueBlock_Controller_Vec2);
}

void DrawEditBlock(ui_window *window, layout *window_layout, DashboardState *dashstate)
{
   UIContext *context = window_layout->context;
   v2 window_size = GetSize(window_layout->bounds);
   
   Text(window_layout, Literal("Edit Block"), 20,
      V2((window_size.x - GetTextWidth(context->render_context, Literal("Edit Block"), 20)) / 2.0f, 0),
      V2(0, 0)); 
   NextLine(window_layout);
     
   FunctionBlock *function = window->function;
   
   ValueBlockType param_type = function->param.type;
   ValueBlockType new_param_type = param_type;
   
   if(IsBool(param_type))
   {
      if(Button(window_layout, NULL, Literal("Constant"), (param_type == ValueBlock_Constant_Bool),
                V2(60, 20), V2(0, 0), V2(5, 5)).state)
      {
         new_param_type = ValueBlock_Constant_Bool;
      }
      
      if(Button(window_layout, NULL, Literal("Controller"), (param_type == ValueBlock_Controller_Bool),
                V2(60, 20), V2(0, 0), V2(5, 5)).state)
      {
         new_param_type = ValueBlock_Controller_Bool;
      }
      
      if(Button(window_layout, NULL, Literal("Sensor"), (param_type == ValueBlock_Sensor_Bool),
                V2(60, 20), V2(0, 0), V2(5, 5)).state)
      {
         new_param_type = ValueBlock_Sensor_Bool;
      }    
   }
   else if(IsFloat(param_type))
   {
      if(Button(window_layout, NULL, Literal("Constant"), (param_type == ValueBlock_Constant_Float),
                V2(60, 20), V2(0, 0), V2(5, 5)).state)
      {
         new_param_type = ValueBlock_Constant_Float;
      }
      
      if(Button(window_layout, NULL, Literal("Controller"), (param_type == ValueBlock_Controller_Float),
                V2(60, 20), V2(0, 0), V2(5, 5)).state)
      {
         new_param_type = ValueBlock_Controller_Float;
      }
      
      if(Button(window_layout, NULL, Literal("Sensor"), (param_type == ValueBlock_Sensor_Float),
                V2(60, 20), V2(0, 0), V2(5, 5)).state)
      {
         new_param_type = ValueBlock_Sensor_Float;
      }
      
      NextLine(window_layout);
   }
   else if(IsUint(param_type))
   {
      //NOTE: uint can only be constant ¯\_(ツ)_/¯
   }
   else if(IsVec2(param_type))
   {
      if(Button(window_layout, NULL, Literal("Constant"), (param_type == ValueBlock_Constant_Vec2),
                V2(60, 20), V2(0, 0), V2(5, 5)).state)
      {
         new_param_type = ValueBlock_Constant_Vec2;
      }
      
      if(Button(window_layout, NULL, Literal("Controller"), (param_type == ValueBlock_Controller_Vec2),
                V2(60, 20), V2(0, 0), V2(5, 5)).state)
      {
         new_param_type = ValueBlock_Controller_Vec2;
      }
      
      NextLine(window_layout);
   }
   
   if(new_param_type != param_type)
   {
      function->param = {};
      function->param.type = new_param_type;
   }  
   
   switch(function->type)
   {
      case FunctionBlock_Set:
      case FunctionBlock_Wait:
      {
         //TODO: a way to provice a range for the float param
         //TODO: provide a better name for the params?
         DrawValueBlock(&function->param, window_layout/*, Literal("Duration: ")*/);
         NextLine(window_layout);
      }
      break;
         
      case FunctionBlock_Builtin:
         //NOTE: no params as of yet, maybe one day
         break;
      
      case FunctionBlock_Vision:
         //TODO: vision callback!!!!!
         //NOTE: vision configs are setup in the vision tab
         //TODO: button to take you the vision tab
         break;
         
      /*
      case FunctionBlock_Goto:
         break;
      */
   }
   
   if(Button(window_layout, NULL, Literal("Delete"), V2(60, 20), V2(0, 0), V2(5, 5)).state)
   {
      
   }
}

void DrawEditorMenu(layout *editor_menu, AutonomousEditor *auto_editor)
{
   RenderContext *render_context = editor_menu->context->render_context;
   
   Rectangle(render_context, editor_menu->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, editor_menu->bounds, V4(0, 0, 0, 1));
   
   v2 button_size = V2(GetSize(editor_menu->bounds).x * 0.9, 40);
   v2 button_margin = V2(GetSize(editor_menu->bounds).x * 0.05, 5);
   
   //TODO: make these buttons work
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
   rect2 view_bounds = graph_view->bounds;
   rect2 graph_bounds = RectPosSize(GetCenter(view_bounds), GetSize(view_bounds) - V2(10, 10));
   
   Rectangle(render_context, view_bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, view_bounds, V4(0, 0, 0, 1));
   RectangleOutline(render_context, graph_bounds, V4(0, 0, 0, 1));
   
   //TODO: timeline rendering
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
      
      auto_editor->edit_block_window->function = block;
   }
   
   Rectangle(render_context, bounds, block_interact.hot ? V4(1, 0, 0, 0.8) : V4(1, 0, 0, 0.6));
   
   if(auto_editor->edit_block_window && 
      (auto_editor->edit_block_window->function == block))
   {
      RectangleOutline(render_context, bounds, V4(0, 0, 0, 1), 2);
   }
   
   //TODO: proper titles
   switch(block->type)
   {
      case FunctionBlock_Wait:
         Text(render_context, bounds, Literal("Wait"));
         break;
      
      case FunctionBlock_Set:
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
         
         auto_editor->create_block_window->coroutine = coroutine;
      }
   }
}

void DrawBlockEditor(layout *editor_panel, DashboardState *dashstate,
                     MemoryArena *generic_arena)
{
   UIContext *context = editor_panel->context;
   RenderContext *render_context = context->render_context;
   
   RectangleOutline(render_context, editor_panel->bounds, V4(0, 0, 0, 1));
   Rectangle(render_context, editor_panel->bounds, V4(0.3, 0.3, 0.3, 0.6));
   
   rect2 add_coroutine_bounds = RectMinSize(V2(editor_panel->bounds.max.x - 20, editor_panel->bounds.min.y), V2(20, 20));
   
   interaction_state add_coroutine_interact =
         ClickInteraction(context, Interaction(GEN_UI_ID, editor_panel), context->input_state.left_up,
                          context->input_state.left_down, Contains(add_coroutine_bounds, context->input_state.pos));
   
   Rectangle(render_context, add_coroutine_bounds, add_coroutine_interact.hot ? V4(1, 0, 0, 0.8) : V4(1, 0, 0, 0.6));
   
   if(add_coroutine_interact.became_selected)
   {
      //TODO: add coroutine
   }
   
   DrawCoroutine(&dashstate->auto_editor.coroutine, dashstate,
                 editor_panel->bounds.min + V2(5, 5), editor_panel, context);
}

void DrawAutonomousEditor(layout *auto_editor, UIContext *context, DashboardState *dashstate)
{
   v2 editor_size = GetSize(auto_editor->bounds);
   
   layout timeline_view = Panel(auto_editor, V2(editor_size.x * 0.95 - 7.5, editor_size.y * 0.4), V2(0, 0), V2(5, 0)).lout;
   layout menu_bar = Panel(auto_editor, V2(editor_size.x * 0.05 - 7.5, editor_size.y * 0.4), V2(0, 0), V2(5, 0)).lout;
   NextLine(auto_editor);
   layout block_editor = Panel(auto_editor, V2(editor_size.x, editor_size.y * 0.6) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   
   DrawGraphView(&timeline_view, &dashstate->auto_editor);
   DrawEditorMenu(&menu_bar, &dashstate->auto_editor);
   DrawBlockEditor(&block_editor, dashstate, dashstate->generic_arena);
}