void DrawEditorMenu(layout *editor_menu, AutonomousEditor *auto_editor, NetworkState *net_state)
{
   RenderContext *render_context = editor_menu->context->render_context;
   
   Rectangle(render_context, editor_menu->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(render_context, editor_menu->bounds, V4(0, 0, 0, 1));
   
   v2 button_size = V2(GetSize(editor_menu->bounds).x * 0.9, 40);
   v2 button_margin = V2(GetSize(editor_menu->bounds).x * 0.05, 5);
   
   //TODO: make these buttons work
   if(Button(editor_menu, NULL, Literal("Open"), button_size, V2(0, 0), button_margin).state)
   {
	   
   }
   
   if(Button(editor_menu, NULL, Literal("Save"), button_size, V2(0, 0), button_margin).state)
   {
	   
   }
   
   if(Button(editor_menu, NULL, Literal("Upload"), button_size, V2(0, 0), button_margin).state)
   {
	   UploadAutonomous(net_state, auto_editor);
   }
   
   Button(editor_menu, NULL, Literal("Record"), button_size, V2(0, 0), button_margin);
   
   if(Button(editor_menu, NULL, Literal("Clear"), button_size, V2(0, 0), button_margin).state)
   {
	   
   }
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
   
	layout timeline_view = Panel(auto_editor, V2(editor_size.x * 0.95 - 7.5, editor_size.y * 0.5), V2(0, 0), V2(5, 0)).lout;
	layout menu_bar = Panel(auto_editor, V2(editor_size.x * 0.05 - 7.5, editor_size.y * 0.5), V2(0, 0), V2(5, 0)).lout;
	NextLine(auto_editor);
 
	element title_box = TextBox(auto_editor, dashstate->auto_editor.name, V2(editor_size.x, editor_size.y * 0.1) - V2(10, 10), V2(0, 0), V2(5, 5));
	
	layout block_editor = Panel(auto_editor, V2(editor_size.x, editor_size.y * 0.4) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   
	DrawGraphView(&timeline_view, &dashstate->auto_editor);
	DrawEditorMenu(&menu_bar, &dashstate->auto_editor, dashstate->net_state);
	DrawBlockEditor(&block_editor, dashstate, dashstate->generic_arena);
}