void DrawHardwareList(layout *hardware_list, UIContext *context, Robot *robot)
{
   Rectangle(context->render_context, hardware_list->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, hardware_list->bounds, V4(0, 0, 0, 1));
   
   Text(hardware_list, Literal("Hardware"), 40,
        V2((GetSize(hardware_list->bounds).x - GetTextWidth(context->render_context, Literal("Hardware"), 40)) / 2.0f, 0), V2(0, 5)); 
   
   ui_id list_id = GEN_UI_ID;
   interaction_state hardware_list_interact =
      ClickInteraction(context, Interaction(list_id, hardware_list), context->input_state.left_up,
                       context->input_state.left_down, Contains(hardware_list->bounds, context->input_state.pos));
   
   if(hardware_list_interact.became_selected)
   {
      robot->selected_hardware = NULL;
   }
   
   v2 button_size = V2(GetSize(hardware_list->bounds).x * 0.8, 40);
   v2 button_margin = V2(GetSize(hardware_list->bounds).x * 0.1, 10);
    
   for(u32 i = 0;
       i < robot->hardware_count;
       i++)
   {
      RobotHardware *hardware = robot->hardware + i;
      button hardware_button = _Button(POINTER_UI_ID(hardware), hardware_list, NULL,
                                       hardware->name, robot->selected_hardware == hardware,
                                       button_size, V2(0, 0), button_margin);
      
      if(hardware_button.state)
      {
         robot->selected_hardware = hardware;
      }
   }
}

RobotHardwareSample *GetLatestSample(RobotHardware *hardware)
{
	u64 latest_timestamp = MIN_U64;
	RobotHardwareSample *latest_sample = NULL;
   
	for(u32 i = 0;
		i < ArrayCount(hardware->samples);
		i++)
	{
		RobotHardwareSample *curr = hardware->samples + i;
		latest_timestamp = Max(latest_timestamp, curr->timestamp);
	
		if(curr->timestamp == latest_timestamp)
		{
			latest_sample = curr;
		}
	}
	
	return latest_sample;
}

struct robot_sample
{
	b32 is_drive_sample;
	union
	{
		struct
		{
			RobotHardwareSample sample;
			u32 index;
		} hardware;
		
		RobotDriveSample drive_sample;
	};
};

u64 GetTimestamp(robot_sample in)
{
	if(in.is_drive_sample)
	{
		return in.drive_sample.timestamp;
	}
	else
	{
		return in.hardware.sample.timestamp;
	}
}

struct earliest_sample_result
{
	b32 success;
	robot_sample sample;
};

u64 GetTimestamp(earliest_sample_result in)
{
	return in.success ? GetTimestamp(in.sample) : 0;
}

earliest_sample_result GetEarliestSampleFromAll(Robot *robot, u64 after_timestamp, 
												u32 *do_not_include = NULL, u32 do_not_include_length = 0)
{
	earliest_sample_result result = {};
	
	for(u32 hw_index = 0;
		hw_index < robot->hardware_count;
		hw_index++)
	{
		RobotHardware *curr_hardware = robot->hardware + hw_index;
		
		for(u32 i = 0;
			i < ArrayCount(curr_hardware->samples);
			i++)
		{
			RobotHardwareSample *curr = curr_hardware->samples + i;
			b32 include = true;
		
			for(u32 i = 0;
				i < do_not_include_length;
				i++)
			{
				if(do_not_include[i] == hw_index)
				{
					include = false;
					break;
				}
			}
		
			if(include && (curr->timestamp > after_timestamp))
			{
				if(GetTimestamp(result) == 0)
				{
					result.success = true;
					result.sample.hardware.index = hw_index;
					result.sample.hardware.sample = *curr;
				}
				else if(curr->timestamp < GetTimestamp(result))
				{
					result.sample.hardware.index = hw_index;
					result.sample.hardware.sample = *curr;
				}
			}
		}
	}

	for(u32 i = 0;
		i < ArrayCount(robot->samples);
		i++)
	{
		RobotDriveSample *curr = robot->samples + i;
		
		if(curr->timestamp > after_timestamp)
		{
			if(GetTimestamp(result) == 0)
			{
				result.success = true;
				result.sample.is_drive_sample = true;
				result.sample.drive_sample = *curr;
			}
			else if(curr->timestamp < GetTimestamp(result))
			{
				result.sample.is_drive_sample = true;
				result.sample.drive_sample = *curr;
			}
		}
	}
	
	return result;
}

bool sort_by_timestamp(robot_sample a, robot_sample b)
{
	return (GetTimestamp(a) < GetTimestamp(b));
}

std::vector<robot_sample> *ListSamplesFromTo(Robot *robot, u64 from, u64 to, u32 *do_not_include = NULL, u32 do_not_include_length = 0)
{
	std::vector<robot_sample> *list = new std::vector<robot_sample>();
	
	for(u32 hw_index = 0;
		hw_index < robot->hardware_count;
		hw_index++)
	{
		RobotHardware *curr_hardware = robot->hardware + hw_index;
		b32 include = true;
		
		for(u32 i = 0;
			i < do_not_include_length;
			i++)
		{
			if(do_not_include[i] == hw_index)
			{
				include = false;
				break;
			}
		}
		
		if(include)
		{			
			for(u32 i = 0;
				i < ArrayCount(curr_hardware->samples);
				i++)
			{
				RobotHardwareSample *curr = curr_hardware->samples + i;
				
				if((from <= curr->timestamp) && (curr->timestamp <= to) && (curr->timestamp != 0))
				{
					robot_sample sample = {};
					sample.hardware.index = hw_index;
					sample.hardware.sample = *curr;
					
					list->push_back(sample);
				}
			}
		}
	}

	for(u32 i = 0;
		i < ArrayCount(robot->samples);
		i++)
	{
		RobotDriveSample *curr = robot->samples + i;
		
		if((from <= curr->timestamp) && (curr->timestamp <= to) && (curr->timestamp != 0))
		{
			robot_sample sample = {};
			sample.is_drive_sample = true;
			sample.drive_sample = *curr;
					
			list->push_back(sample);
		}
	}
	
	std::sort(list->begin(), list->end(), sort_by_timestamp);
	
	return list;
}

void DrawSelectedHardwarePage(layout *selected_hardware_page, UIContext *context,
                              Robot *robot, MemoryArena *generic_arena)
{
	Rectangle(context->render_context, selected_hardware_page->bounds, V4(0.3, 0.3, 0.3, 0.6));
	RectangleOutline(context->render_context, selected_hardware_page->bounds, V4(0, 0, 0, 1));
   
	RobotHardware *selected_hardware = robot->selected_hardware;
	Text(selected_hardware_page, selected_hardware->name, 40,
         V2((GetSize(selected_hardware_page->bounds).x - GetTextWidth(context->render_context, selected_hardware->name, 40)) / 2.0f, 0), V2(0, 5)); 
   
	layout time_graph = Panel(selected_hardware_page, V2(GetSize(selected_hardware_page->bounds).x - 10, GetSize(selected_hardware_page->bounds).y * 0.45), V2(0, 0), V2(5, 5)).lout;
	NextLine(selected_hardware_page);
   
	u64 earliest_timestamp = MAX_U64;
	u64 latest_timestamp = MIN_U64;

	r32 lowest_value = FLTMAX;
	r32 highest_value = -FLTMAX;
	
	RobotHardwareSample *latest_sample = NULL;
   
	for(u32 i = 0;
		i < ArrayCount(selected_hardware->samples);
		i++)
	{
		RobotHardwareSample *curr = selected_hardware->samples + i;
	
		if(curr->timestamp != 0)
		{
			r32 curr_value = 0;
			
			if((selected_hardware->type == Hardware_Motor) ||
			   (selected_hardware->type == Hardware_EncoderMotor))
			{
				curr_value = curr->motor;
			}
			else if(selected_hardware->type == Hardware_Potentiometer)
			{ 
				curr_value = curr->potentiometer;
			}
			
			lowest_value = Min(lowest_value, curr_value);
			highest_value = Max(highest_value, curr_value);		
	
			earliest_timestamp = Min(earliest_timestamp, curr->timestamp);
			latest_timestamp = Max(latest_timestamp, curr->timestamp);
		
			if(curr->timestamp == latest_timestamp)
			{
				latest_sample = curr;
			}
		}
	}
   
	Rectangle(context->render_context, time_graph.bounds, V4(0.3, 0.3, 0.3, 0.6));
	RectangleOutline(context->render_context, time_graph.bounds, V4(0, 0, 0, 1));
   
	if(latest_sample)
	{
		r32 value_difference = highest_value - lowest_value;
	
		r32 graph_height = GetSize(time_graph.bounds).y;
		r32 x_axis_interval = GetSize(time_graph.bounds).x / (latest_timestamp - earliest_timestamp);
		r32 y_axis_scale = ((selected_hardware->type == Hardware_Motor) || (selected_hardware->type == Hardware_EncoderMotor) ||(selected_hardware->type == Hardware_Potentiometer)) ? (GetSize(time_graph.bounds).y / value_difference) : 0;
	  
		u32 sample_count = 0;
	  
		for(u32 i = 0;
			i < ArrayCount(selected_hardware->samples);
			i++)
		{
			if(selected_hardware->samples[i].timestamp != 0)
			{
				r32 x = x_axis_interval * (selected_hardware->samples[i].timestamp - earliest_timestamp);
				r32 y = 0;
				
				if(selected_hardware->type == Hardware_Motor)
				{
					y = graph_height - (selected_hardware->samples[i].motor - lowest_value) * y_axis_scale;
				}
				else if(selected_hardware->type == Hardware_EncoderMotor)
				{
					y = graph_height - (selected_hardware->samples[i].motor - lowest_value) * y_axis_scale;
				}
				else if(selected_hardware->type == Hardware_Solenoid)
				{
					y = GetSize(time_graph.bounds).y * (selected_hardware->samples[i]._switch ? 0.2 : 0.8);
				}
				else if(selected_hardware->type == Hardware_Switch)
				{
					y = GetSize(time_graph.bounds).y * (selected_hardware->samples[i].solenoid ? 0.2 : 0.8);
				}
				else if(selected_hardware->type == Hardware_Potentiometer)
				{
					y = graph_height - (selected_hardware->samples[i].potentiometer - lowest_value) * y_axis_scale;
				}
				
				Rectangle(context->render_context,
						  RectPosSize(V2(x, y) + time_graph.bounds.min, V2(5, 5)),
						  V4(0, 0, 0, 1));
				
				sample_count++;
			}
		}
	  
		TemporaryMemoryArena temp_memory = BeginTemporaryMemory(generic_arena);
	  
		if(selected_hardware->type == Hardware_Motor)
		{
			Text(selected_hardware_page, 
				 Concat(Literal("State: "), ToString(latest_sample->motor, &temp_memory), &temp_memory),
				 20, V2(0, 0), V2(0, 5));
		}
		else if(selected_hardware->type == Hardware_EncoderMotor)
		{	
			Text(selected_hardware_page, 
				 Concat(Literal("State: "), ToString(latest_sample->motor, &temp_memory), Literal("RPM"), &temp_memory),
				 20, V2(0, 0), V2(0, 5));
		}
		else if(selected_hardware->type == Hardware_Solenoid)
		{
			string solenoid_state_text = latest_sample->solenoid ? Literal("State: Extended") : Literal("State: Retracted");
			Text(selected_hardware_page, solenoid_state_text, 20, V2(0, 0), V2(0, 5)); 
		}
		else if(selected_hardware->type == Hardware_Switch)
		{
			string solenoid_state_text = latest_sample->_switch ? Literal("State: Pressed") : Literal("State: Released");
			Text(selected_hardware_page, solenoid_state_text, 20, V2(0, 0), V2(0, 5)); 
		}
		else if(selected_hardware->type == Hardware_Potentiometer)
		{
			Text(selected_hardware_page, 
				 Concat(Literal("State: "), ToString(latest_sample->potentiometer, &temp_memory), &temp_memory),
				 20, V2(0, 0), V2(0, 5));
		}
		
		NextLine(selected_hardware_page);
			
		Text(selected_hardware_page,
			 Concat(Literal("Sample Count: "), ToString(sample_count, &temp_memory), Literal(" of "), ToString((u32)ArrayCount(selected_hardware->samples), &temp_memory), &temp_memory),
			 20, V2(0, 0), V2(0, 5)); 			
		NextLine(selected_hardware_page);
			
		if(Button(selected_hardware_page, NULL, Literal("Clear"), V2(120, 40), V2(0, 0), V2(5, 5)).state)
		{
			for(u32 i = 0;
				i < ArrayCount(selected_hardware->samples);
				i++)
			{
				selected_hardware->samples[i].timestamp = 0;
			}
		}
		
		EndTemporaryMemory(temp_memory);
	}
	else
	{
		Text(&time_graph, Literal("No Data"), 40,
			 V2((GetSize(time_graph.bounds).x - GetTextWidth(context->render_context, Literal("No Data"), 40)) / 2.0f,
				(GetSize(time_graph.bounds).y - 40) / 2.0f), V2(0, 0)); 
	}
}

void DrawRobot(layout *robot_ui, UIContext *context, DashboardState *dashstate)
{
	Robot *robot = &dashstate->robot;
	v2 robot_ui_size = GetSize(robot_ui->bounds);
	layout hardware_list = Panel(robot_ui, V2(robot_ui_size.x * 0.2, robot_ui_size.y) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
	layout selected_hardware_page = Panel(robot_ui, V2(robot_ui_size.x * 0.8, robot_ui_size.y) - V2(5, 10), V2(0, 0), V2(5, 5)).lout;
   
	DrawHardwareList(&hardware_list, context, robot);
   
	if(robot->selected_hardware)
	{
		DrawSelectedHardwarePage(&selected_hardware_page, context, robot, dashstate->generic_arena);
	}
	else
	{
		Rectangle(context->render_context, selected_hardware_page.bounds, V4(0.3, 0.3, 0.3, 0.6));
		RectangleOutline(context->render_context, selected_hardware_page.bounds, V4(0, 0, 0, 1));
   
		RobotHardware *selected_hardware = robot->selected_hardware;
		Text(&selected_hardware_page, Literal("Drive Train"), 40,
			 V2((GetSize(selected_hardware_page.bounds).x - GetTextWidth(context->render_context, Literal("Drive Train"), 40)) / 2.0f, 0), V2(0, 5)); 
   
		layout time_graph = Panel(&selected_hardware_page, V2(GetSize(selected_hardware_page.bounds).x - 10, GetSize(selected_hardware_page.bounds).y * 0.45), V2(0, 0), V2(5, 5)).lout;
		NextLine(&selected_hardware_page);
   
		Rectangle(context->render_context, time_graph.bounds, V4(0.3, 0.3, 0.3, 0.6));
		RectangleOutline(context->render_context, time_graph.bounds, V4(0, 0, 0, 1));
   
		u64 earliest_timestamp = MAX_U64;
		u64 latest_timestamp = MIN_U64;
		
		r32 lowest_left_value = FLTMAX;
		r32 highest_left_value = -FLTMAX;
		
		r32 lowest_right_value = FLTMAX;
		r32 highest_right_value = -FLTMAX;
		
		RobotDriveSample *latest_sample = NULL;
		
		for(u32 i = 0;
			i < ArrayCount(robot->samples);
			i++)
		{
			RobotDriveSample *curr = robot->samples + i;
	
			if(curr->timestamp != 0)
			{
				lowest_left_value = Min(lowest_left_value, curr->left);
				highest_left_value = Max(highest_left_value, curr->left);		
		
				lowest_right_value = Min(lowest_right_value, curr->right);
				highest_right_value = Max(highest_right_value, curr->right);
				
				earliest_timestamp = Min(earliest_timestamp, curr->timestamp);
				latest_timestamp = Max(latest_timestamp, curr->timestamp);
			
				if(curr->timestamp == latest_timestamp)
				{
					latest_sample = curr;
				}
			}
		}
		
		if(latest_sample)
		{
			r32 lowest_value = Min(lowest_left_value, lowest_right_value);
			r32 highest_value = Max(highest_left_value, highest_right_value);
		
			r32 value_difference = highest_value - lowest_value;
	
			r32 graph_height = GetSize(time_graph.bounds).y;
			r32 x_axis_interval = GetSize(time_graph.bounds).x / (latest_timestamp - earliest_timestamp);
			r32 y_axis_scale = GetSize(time_graph.bounds).y / value_difference;
	  
			u32 sample_count = 0;
			b32 in_turn = false;
			
			for(u32 i = 0;
				i < ArrayCount(robot->samples);
				i++)
			{
				if(robot->samples[i].timestamp != 0)
				{
					r32 x = x_axis_interval * (robot->samples[i].timestamp - earliest_timestamp);
					r32 left_y = graph_height - (robot->samples[i].left - lowest_value) * y_axis_scale;
					r32 right_y = graph_height - (robot->samples[i].right - lowest_value) * y_axis_scale;		
				
					Rectangle(context->render_context,
							  RectPosSize(V2(x, left_y) + time_graph.bounds.min, V2(5, 5)),
							  V4(0, 1, 0, 1));
				
					RectangleOutline(context->render_context,
									 RectPosSize(V2(x, left_y) + time_graph.bounds.min, V2(5, 5)),
									 V4(0, 0, 0, 1));

					Rectangle(context->render_context,
							  RectPosSize(V2(x, right_y) + time_graph.bounds.min, V2(5, 5)),
							  V4(0, 0, 1, 1));
												 
					RectangleOutline(context->render_context,
									 RectPosSize(V2(x, right_y) + time_graph.bounds.min, V2(5, 5)),
									 V4(0, 0, 0, 1));
					
					b32 was_in_turn = in_turn;
					in_turn = Abs(robot->samples[i].left - robot->samples[i].right) > 1000;
					
					if((!was_in_turn && in_turn) ||
					   (was_in_turn && !in_turn))
					{
						Line(context->render_context,
							 V2(x + time_graph.bounds.min.x, time_graph.bounds.min.y),
							 V2(x + time_graph.bounds.min.x, time_graph.bounds.max.y),
							 V4(1, 0, 1, 1), 4);
					}
					
					sample_count++;
				}
			}
			
			TemporaryMemoryArena temp_memory = BeginTemporaryMemory(dashstate->generic_arena);
       
			Text(&selected_hardware_page,
				 Concat(Literal("Left: "), ToString(latest_sample->left, &temp_memory), robot->drive_encoder ? Literal("RPM") : Literal(""), &temp_memory),
				 20, V2(0, 0), V2(0, 5)); 
			NextLine(&selected_hardware_page);
			Text(&selected_hardware_page,
				 Concat(Literal("Right: "), ToString(latest_sample->right, &temp_memory), robot->drive_encoder ? Literal("RPM") : Literal(""), &temp_memory),
				 20, V2(0, 0), V2(0, 5)); 
			NextLine(&selected_hardware_page);
			
			Text(&selected_hardware_page,
				 Concat(Literal("Sample Count: "), ToString(sample_count, &temp_memory), Literal(" of "), ToString((u32)ArrayCount(robot->samples), &temp_memory), &temp_memory),
				 20, V2(0, 0), V2(0, 5)); 			
			NextLine(&selected_hardware_page);
			
			if(Button(&selected_hardware_page, NULL, Literal("Clear"), V2(120, 40), V2(0, 0), V2(5, 5)).state)
			{
				for(u32 i = 0;
					i < ArrayCount(robot->samples);
					i++)
				{
					robot->samples[i].timestamp = 0;
				}
			}
			
			EndTemporaryMemory(temp_memory);
		}
		else
		{
			Text(&selected_hardware_page, Literal("No Data"), 20, V2(0, 0), V2(0, 5)); 
		}
	}
}