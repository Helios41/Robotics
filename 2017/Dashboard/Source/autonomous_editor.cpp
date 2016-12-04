AutonomousBlock WaitBlock(r32 wait_duration)
{
   AutonomousBlock result = {};
   result.is_wait_block = true;
   result.wait_duration = wait_duration;
   return result;
}

AutonomousBlock MotorBlock(RobotHardware *hardware, r32 motor_state)
{
   AutonomousBlock result = {};
   result.hardware = hardware;
   result.motor_state = motor_state;
   return result;
}

AutonomousBlock SolenoidBlock(RobotHardware *hardware, SolenoidState solenoid_state)
{
   AutonomousBlock result = {};
   result.hardware = hardware;
   result.solenoid_state = solenoid_state;
   return result;
}

AutonomousBlock DriveBlock(RobotHardware *hardware, r32 forward_value, r32 rotate_value)
{
   AutonomousBlock result = {};
   result.hardware = hardware;
   result.drive_state.forward_value = forward_value;
   result.drive_state.rotate_value = rotate_value;
   return result;
}

void DrawBlockSelector(layout *block_selector, AutonomousEditor *auto_editor, Robot *robot)
{
   UIContext *context = block_selector->context;
   RenderContext *render_context = context->render_context;
   
   Rectangle(render_context, block_selector->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, block_selector->bounds, V4(0, 0, 0, 1));
   
   ui_id block_selector_id = GEN_UI_ID;
   interaction_state block_selector_interact =
      ClickInteraction(context, Interaction(block_selector_id, block_selector), context->input_state.left_up,
                       context->input_state.left_down, Contains(block_selector->bounds, context->input_state.pos));
   
   if(block_selector_interact.became_selected)
   {
      auto_editor->block_grabbed = false;
      auto_editor->grabbed_block = {};
   }
   
   v2 block_size = V2(GetSize(block_selector->bounds).x * 0.8, 40);
   v2 block_margin = V2(GetSize(block_selector->bounds).x * 0.1, 10);
   
   if(Button(block_selector, NULL, Literal("Wait"), block_size, V2(0, 0), block_margin).state)
   {
      auto_editor->grabbed_block = WaitBlock(0);
      auto_editor->block_grabbed = true;
   }
   
   for(u32 i = 0;
       robot->hardware_count > i;
       i++)
   {
      RobotHardware *hardware = robot->hardware + i;
      
      //TODO: clean this up
      if(hardware->type == Hardware_Motor)
      {
         if(_Button(POINTER_UI_ID(hardware), block_selector, NULL, hardware->name, block_size, V2(0, 0), block_margin).state)
         {
            auto_editor->grabbed_block = MotorBlock(hardware, 0);
            auto_editor->block_grabbed = true;
         }
      }
      else if(hardware->type == Hardware_Solenoid)
      {
         if(_Button(POINTER_UI_ID(hardware), block_selector, NULL, /*Concat(Literal("Extend "),*/ hardware->name/*)*/, block_size, V2(0, 0), block_margin).state)
         {
            auto_editor->grabbed_block = SolenoidBlock(hardware, Solenoid_Extended);
            auto_editor->block_grabbed = true;
         }
         
         if(_Button(POINTER_UI_ID(hardware), block_selector, NULL, /*Concat(Literal("Retract "),*/ hardware->name/*)*/, block_size, V2(0, 0), block_margin).state)
         {
            auto_editor->grabbed_block = SolenoidBlock(hardware, Solenoid_Retracted);
            auto_editor->block_grabbed = true;
         }
      }
      else if(hardware->type == Hardware_Drive)
      {
         if(_Button(POINTER_UI_ID(hardware), block_selector, NULL, hardware->name, block_size, V2(0, 0), block_margin).state)
         {
            auto_editor->grabbed_block = DriveBlock(hardware, 0, 0);
            auto_editor->block_grabbed = true;
         }
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

char lua_src_temp[] = "motor.set(1.0)\nmotor.set(0.0)\nsolenoid.set(Extended)";
string lua_src = Literal(lua_src_temp);

void DrawLuaEditor(layout *editor_panel, AutonomousEditor *auto_editor)
{
   RenderContext *render_context = editor_panel->context->render_context;
   element title_element = Text(editor_panel, Literal("Lua Editor"), 20,
                                V2((GetSize(editor_panel->bounds).x - GetTextWidth(render_context, Literal("Lua Editor"), 20)) / 2.0f, 0), V2(0, 5)); 
  
   v2 src_editor_size = V2(GetSize(editor_panel->bounds).x, GetSize(editor_panel->bounds).y - GetSize(title_element.margin_bounds).y);
   TextBox(editor_panel, lua_src, src_editor_size, V2(0, 0), V2(0, 0));
}

interaction_state AutonomousBlockUILogic(ui_id id, layout *ui_layout, AutonomousBlock *block,
                                         rect2 element_bounds, AutonomousEditor *auto_editor)
{
   UIContext *context = ui_layout->context;
   interaction_state block_interact = ClickInteraction(context, Interaction(id, ui_layout->ui_layer + 1, ui_layout->stack_layer, context), context->input_state.left_up,
                                                       context->input_state.left_down, Contains(element_bounds, context->input_state.pos));
    
   if(block_interact.became_selected)
   {
      auto_editor->selected_block = block;
   }
   
   return block_interact;
}

void DrawAutonomousBlock(RenderContext *render_context, AutonomousBlock *block,
                         rect2 element_bounds, AutonomousEditor *auto_editor,
                         b32 is_active, b32 is_hot)
{   
   if(is_active)
   {
      Rectangle(render_context, element_bounds, V4(1.0f, 0.0f, 0.75f, 1.0f));
   }
   else if(is_hot)
   {
      Rectangle(render_context, element_bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   }
   else
   {
      Rectangle(render_context, element_bounds, V4(0.5f, 0.0f, 0.0f, 1.0f));
   }
   
   //TODO: concat the block's data into its text
   //      eg. "Motor @ 10%" or "Wait for 10 seconds"
   string text = block->is_wait_block ? Literal("Wait") : (block->hardware ? block->hardware->name : EmptyString());
   
   TextLabel(render_context, element_bounds, text, 20);
   
   if(block == auto_editor->selected_block)
   {
      Rectangle(render_context, RectMinSize(element_bounds.min, V2(5, GetSize(element_bounds).y)),
               V4(0.0f, 0.0f, 0.0f, 1.0f));
   }
}

r32 debug_scroll = 0;

//TODO: ability to remove blocks, do this by grabbing a block and dropping it into the selection panel
void DrawBlockList(layout *block_list_full, AutonomousEditor *auto_editor)
{
   UIContext *context = block_list_full->context;
   RenderContext *render_context = context->render_context;
   v2 block_list_full_size = GetSize(block_list_full->bounds);
   
   r32 block_height = 40;
   r32 block_margin_height = block_height / 4.0f;
   
   r32 max_blocks = (block_list_full_size.y - block_margin_height) / (block_height + block_margin_height);
   u32 overflow_blocks = (u32) Max(RoundR32ToS32(auto_editor->editor_block_count - max_blocks), 0);
   
   if(overflow_blocks > 0)
   {
      layout scroll_bar = Panel(block_list_full, V2(block_list_full_size.x * 0.1, block_list_full_size.y), V2(0, 0), V2(0, 0)).lout;
      r32 scrollbar_bounds = (overflow_blocks * (block_height + block_margin_height));
      SliderBar(&scroll_bar, -scrollbar_bounds, 0, &debug_scroll, GetSize(scroll_bar.bounds), V2(0, 0), V2(0, 0));
   }
   
   layout block_list = Panel(block_list_full, V2(block_list_full_size.x * (overflow_blocks > 0 ? 0.9 : 1), block_list_full_size.y), V2(0, 0), V2(0, 0), debug_scroll).lout;
  
   RectangleOutline(render_context, block_list.bounds, V4(0.2, 0.2, 0.2, 1), 3);
   
   ui_id list_id = GEN_UI_ID;
   interaction_state block_editor_interact =
      ClickInteraction(context, Interaction(list_id, &block_list), context->input_state.left_up,
                       context->input_state.left_down, Contains(block_list.bounds, context->input_state.pos));
   
   if(block_editor_interact.hot)
   {
      RectangleOutline(render_context, block_list.bounds, V4(0.2, 0.2, 0.2, 1), 3);
   }
   else
   {
      RectangleOutline(render_context, block_list.bounds, V4(0, 0, 0, 1));
   }
   
   v2 block_size = V2(GetSize(block_list.bounds).x * 0.8, block_height);
   v2 block_margin = V2(GetSize(block_list.bounds).x * 0.1, block_margin_height);
   
   rect2 grabbed_block_rect = auto_editor->block_grabbed ? RectMinSize(context->input_state.pos, block_size) :
                                                           RectPosSize(V2(0, 0), V2(0, 0));
   
   if(auto_editor->block_grabbed)
   {
      DrawAutonomousBlock(render_context, &auto_editor->grabbed_block,
                          grabbed_block_rect, auto_editor, false, false);
   }
   
   u32 grabbed_block_hover_index = auto_editor->editor_block_count;
   
   for(u32 i = 0;
       auto_editor->editor_block_count > i;
       i++)
   {
      AutonomousBlock *block = auto_editor->editor_blocks + i;
      layout temp_block_list = block_list;
      element block_element;
      
      block_element = Element(&temp_block_list, block_size, V2(0, 0), block_margin);
      NextLine(&temp_block_list);
      
      if(Intersects(block_element.bounds, grabbed_block_rect))
      {
         grabbed_block_hover_index = i;
         Element(&block_list, block_size, V2(0, 0), block_margin);
         NextLine(&block_list);
         
         block_element = Element(&block_list, block_size, V2(0, 0), block_margin);
         NextLine(&block_list);
      }
      else
      {
         block_list = temp_block_list;
      }
      
      interaction_state block_interact = AutonomousBlockUILogic(POINTER_UI_ID(block), &block_list, block, block_element.bounds, auto_editor);
      
      if(!auto_editor->block_grabbed && block_interact.active &&
         (Distance(context->active_element.start_pos, context->input_state.pos) > 10))
      {
         auto_editor->block_grabbed = true;
         auto_editor->grabbed_block = *block;
        
         //remove block from list
      }
      
      DrawAutonomousBlock(render_context, block, block_element.bounds, auto_editor, block_interact.active, block_interact.hot);
   }
   
   if(block_editor_interact.became_selected)
   {
      if(auto_editor->block_grabbed)
      {
         //TODO: dynamically expand this?
         Assert(ArrayCount(auto_editor->editor_blocks) > (auto_editor->editor_block_count + 1));
         auto_editor->editor_block_count++;
         
         if(auto_editor->selected_block)
         {
            u32 selected_block_index = (u32)(auto_editor->selected_block - auto_editor->editor_blocks);
            
            if(selected_block_index >= grabbed_block_hover_index)
            {
               selected_block_index++;
            }
            
            auto_editor->selected_block = auto_editor->editor_blocks + selected_block_index;
         }
         
         for(u32 i = (auto_editor->editor_block_count - 1);
             i > grabbed_block_hover_index;
             i--)
         {
            auto_editor->editor_blocks[i] = auto_editor->editor_blocks[i - 1];
         }
         
         auto_editor->editor_blocks[grabbed_block_hover_index] = auto_editor->grabbed_block;
         
         //TODO: insert lua src for block that was just added
         
         auto_editor->block_grabbed = false;
         auto_editor->grabbed_block = {};
      }
      else
      {
         auto_editor->selected_block = NULL;
      }
   }
}

void DrawBlockDetails(layout *block_details, AutonomousEditor *auto_editor)
{
   UIContext *context = block_details->context;
   RenderContext *render_context = context->render_context;
   
   AutonomousBlock *block = auto_editor->selected_block;
   
   if(block)
   {
      string text = block->is_wait_block ? Literal("Wait") : block->hardware->name;
      
      RectangleOutline(render_context, block_details->bounds, V4(0.2, 0.2, 0.2, 1), 3);
      Text(block_details, text, 20,
           V2((GetSize(block_details->bounds).x - GetTextWidth(render_context, text, 20)) / 2.0f, 0), V2(0, 5)); 
      
      if(block->is_wait_block)
      {
         Text(block_details, Literal("Duration: "), 20, V2(0, 0), V2(0, 5)); 
         TextBox(block_details, 0, 15, &block->wait_duration, V2(100, 40), V2(0, 0), V2(0, 0)); 
         NextLine(block_details);
         SliderBar(block_details, 0, 15 /*max auto mode length???*/, &block->wait_duration, V2(180, 20), V2(0, 0), V2(0, 0));
      }
      else
      {
         if(block->hardware->type == Hardware_Motor)
         {
            Text(block_details, Literal("State: "), 20, V2(0, 0), V2(0, 5)); 
            TextBox(block_details, -1, 1, &block->motor_state, V2(100, 40), V2(0, 0), V2(0, 0)); 
            NextLine(block_details);
            SliderBar(block_details, -1, 1, &block->motor_state, V2(180, 20), V2(0, 0), V2(0, 0));
         }
         else if(block->hardware->type == Hardware_Solenoid)
         {
            b32 is_extended = (block->solenoid_state == Solenoid_Extended);
            
            Text(block_details, is_extended ? Literal("State: Extended") : Literal("State: Retracted"), 20, V2(0, 0), V2(0, 5));
            NextLine(block_details);
            
            ToggleSlider(block_details, &is_extended, Literal("Extended"), Literal("Retracted"), V2(200, 30), V2(0, 0), V2(0, 0));   
            block->solenoid_state = is_extended ? Solenoid_Extended : Solenoid_Retracted;
         }
         else if(block->hardware->type == Hardware_Drive)
         {
            Text(block_details, Literal("Forward: "), 20, V2(0, 0), V2(0, 5)); 
            TextBox(block_details, -1, 1, &block->drive_state.forward_value, V2(100, 40), V2(0, 0), V2(0, 0)); 
            NextLine(block_details);
            SliderBar(block_details, -1, 1, &block->drive_state.forward_value, V2(180, 20), V2(0, 0), V2(0, 0));
            
            NextLine(block_details);
            
            Text(block_details, Literal("Rotate: "), 20, V2(0, 0), V2(0, 5)); 
            TextBox(block_details, -1, 1, &block->drive_state.rotate_value, V2(100, 40), V2(0, 0), V2(0, 0)); 
            NextLine(block_details);
            SliderBar(block_details, -1, 1, &block->drive_state.rotate_value, V2(180, 20), V2(0, 0), V2(0, 0));
         }
      }
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
   
   DrawBlockSelector(&block_selector, &dashstate->auto_editor, &dashstate->robot);
   DrawEditorMenu(&editor_menu, &dashstate->auto_editor);
   
   layout graph_view = Panel(&editor, V2(GetSize(editor.bounds).x, GetSize(editor.bounds).y * 0.3) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   layout editor_bar = Panel(&editor, V2(GetSize(editor.bounds).x, GetSize(editor.bounds).y * 0.05), V2(0, 0), V2(0, 0)).lout;
   layout editor_panel = Panel(&editor, V2(GetSize(editor.bounds).x, GetSize(editor.bounds).y * 0.65), V2(0, 0), V2(0, 0)).lout;
   
   DrawGraphView(&graph_view, &dashstate->auto_editor);
   DrawEditorBar(&editor_bar, &dashstate->auto_editor);
   DrawEditorPanel(&editor_panel, &dashstate->auto_editor);
   
   if(dashstate->auto_editor.block_grabbed)
   {
      string grabbed_block_text = dashstate->auto_editor.grabbed_block.is_wait_block ? Literal("Wait") : dashstate->auto_editor.grabbed_block.hardware->name;
      
      if(!IsEmpty(grabbed_block_text))
      {
         context->tooltip = grabbed_block_text;
      }
   }
}