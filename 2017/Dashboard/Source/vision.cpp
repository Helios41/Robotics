cv::Mat process(cv::Mat frame)
{	
	cv::Scalar low(120, 100, 16);
	cv::Scalar high(255, 255, 64);
	
	cv::Mat masked;
	cv::inRange(frame, low, high, masked);
	
	cv::Mat eroded;
	cv::erode(masked, eroded, cv::Mat());
	
	cv::Mat dilated;
	cv::dilate(eroded, dilated, cv::Mat());
	
	return dilated;
}

bool sort_by_size(cv::Rect a, cv::Rect b)
{
	return (a.area() > b.area());
}

struct Target
{
	b32 hit;
	cv::Rect top_box;
	cv::Rect bottom_box;
};

Target track(cv::Mat masked)
{
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(masked.clone(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	std::vector<cv::Rect> hits;
	
	for(u32 i = 0; i < contours.size(); i++)
	{
		cv::Rect contour = cv::boundingRect(contours[i]);
		if(contour.area() > 40)
		{
			hits.push_back(contour);
		}
	}
	
	Target result = {};
	
	if(hits.size() >= 2)
	{
		std::sort(hits.begin(), hits.end(), sort_by_size);
		
		r32 box0_y = hits[0].y + hits[0].height / 2;
		r32 box1_y = hits[1].y + hits[1].height / 2;
		
		cv::Rect top_box = (box0_y > box1_y) ? hits[0] : hits[1];
		cv::Rect bottom_box = (box0_y > box1_y) ? hits[1] : hits[0];
		
		r32 difference = (top_box.x + top_box.width / 2) - (bottom_box.x + bottom_box.width / 2);
		
		if(Abs(difference) < 10)
		{	
			result.hit = true;
			result.top_box = top_box;
			result.bottom_box = bottom_box;
		}
	}
	
	return result;
}

r32 VisionTest(cv::VideoCapture *cap, s32 brightness,
			   rect2 top_reference, rect2 bottom_reference,
			   DashboardState *dashstate)
{
	cv::Mat frame;
	bool frame_success = cap->read(frame);
		
	dashstate->vision.frame_grab_success = frame_success;
		
	if(frame_success)
	{
		frame = frame + cv::Scalar(brightness, brightness, brightness);
		frame.copyTo(*dashstate->vision.grabbed_frame);
		
		cv::Mat masked = process(frame);
		Target target = track(masked);
		
		dashstate->vision.target_hit = target.hit;
		dashstate->vision.top_target = RectMinSize(V2(target.top_box.x, target.top_box.y), V2(target.top_box.width, target.top_box.height));
		dashstate->vision.bottom_target = RectMinSize(V2(target.bottom_box.x, target.bottom_box.y), V2(target.bottom_box.width, target.bottom_box.height));
		
		if(target.hit)
		{
			r32 top_movement = GetCenter(top_reference).x - (target.top_box.x + target.top_box.width / 2);
			r32 bottom_movement = GetCenter(bottom_reference).x - (target.bottom_box.x + target.bottom_box.width / 2);
				
			r32 movement = (top_movement + bottom_movement) / 2;
			return movement;
		}
	}
	
	return 0;
}

void RunVision(UIContext *context, DashboardState *dashstate)
{
	if(dashstate->robot.connected && dashstate->vision.enabled)
	{
		r32 potentiometer_reading = GetLatestSample(dashstate->robot.hardware + 9)->potentiometer;
			
		if((context->curr_time - dashstate->vision.last_track_time) > (1.0 / (r32)dashstate->vision.tracks_per_second))
		{
			dashstate->vision.movement = VisionTest(dashstate->vision.camera, dashstate->vision.brightness,
													dashstate->vision.top_reference, dashstate->vision.bottom_reference,
													dashstate);
			
			if(Abs(dashstate->vision.movement) > 170)
			{
				dashstate->vision.turret_speed = (dashstate->vision.movement / Abs(dashstate->vision.movement)) * 0.25;
			}
			else if((170 > Abs(dashstate->vision.movement)) && (Abs(dashstate->vision.movement) > 20))
			{
				dashstate->vision.turret_speed = (dashstate->vision.movement / Abs(dashstate->vision.movement)) * 0.165;
			}
			else if(20 > Abs(dashstate->vision.movement))
			{
				dashstate->vision.turret_speed = 0.0f;
			}
			
			b32 moving_right = (dashstate->vision.turret_speed > 0) && (dashstate->vision.right_limit > potentiometer_reading);
			b32 moving_left = (dashstate->vision.turret_speed < 0) && (dashstate->vision.left_limit < potentiometer_reading);
			b32 stopping = (dashstate->vision.turret_speed == 0);
			
			SendSetFloat(dashstate->net_state, 5, dashstate->vision.turret_speed);
			
			dashstate->vision.last_track_time = context->curr_time;
		}
		
		if(dashstate->vision.target_hit)
		{
			if(dashstate->vision.turret_speed == 0)
			{
				r32 motor_reading = GetLatestSample(dashstate->robot.hardware + 1)->motor;
				b32 speed_hit = (Abs(motor_reading - dashstate->vision.shooter_target) < dashstate->vision.shooter_threshold);
			
				SendSetFloat(dashstate->net_state, 3, speed_hit ? -0.65 : 0);
				SendSetFloat(dashstate->net_state, 0, speed_hit ? 1 : 0);
			}
		}
		else
		{
			if(dashstate->vision.left_limit > potentiometer_reading)
			{
				dashstate->vision.sweep_speed = -0.2;
			}
			else if(potentiometer_reading > dashstate->vision.right_limit)
			{
				dashstate->vision.sweep_speed = 0.2;
			}
			
			SendSetFloat(dashstate->net_state, 5, dashstate->vision.sweep_speed);
		}
	}
}

struct vision_config_file_format
{
	s32 brightness;
	u32 tracks_per_second;
	r32 left_limit;
	r32 right_limit;
	rect2 top_reference;
	rect2 bottom_reference;
};

void SaveVisionConfig(DashboardState *dashstate)
{
	FILE *vision_config_file = fopen("donotdelete.vconfig", "wb");
	if(vision_config_file)
	{
		vision_config_file_format vision_config_file_data = {};
		
		vision_config_file_data.brightness = dashstate->vision.brightness;
		vision_config_file_data.tracks_per_second = dashstate->vision.tracks_per_second;
		vision_config_file_data.left_limit = dashstate->vision.left_limit;
		vision_config_file_data.right_limit = dashstate->vision.right_limit;
		vision_config_file_data.top_reference = dashstate->vision.top_reference;
		vision_config_file_data.bottom_reference = dashstate->vision.bottom_reference;
		
		fwrite(&vision_config_file_data, sizeof(vision_config_file_data), 1, vision_config_file);
		fclose(vision_config_file);
	}	
}

void LoadVisionConfig(DashboardState *dashstate)
{
	FILE *vision_config_file = fopen("donotdelete.vconfig", "rb");
	if(vision_config_file)
	{
		vision_config_file_format vision_config_file_data = {};
		fread(&vision_config_file_data, sizeof(vision_config_file_data), 1, vision_config_file);
		fclose(vision_config_file);
		
		dashstate->vision.brightness = vision_config_file_data.brightness;
		dashstate->vision.tracks_per_second = vision_config_file_data.tracks_per_second;
		dashstate->vision.left_limit = vision_config_file_data.left_limit;
		dashstate->vision.right_limit = vision_config_file_data.right_limit;
		dashstate->vision.top_reference = vision_config_file_data.top_reference;
		dashstate->vision.bottom_reference = vision_config_file_data.bottom_reference;	
	}
}

void DrawVision(layout *vision_ui, UIContext *context, DashboardState *dashstate)
{
	RenderContext *render_context = context->render_context;
	v2 vision_ui_size = GetSize(vision_ui->bounds);
	layout vision_config_list = Panel(vision_ui, V2(vision_ui_size.x * 0.2, vision_ui_size.y) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
	layout vision_info_page = Panel(vision_ui, V2(vision_ui_size.x * 0.8, vision_ui_size.y) - V2(5, 10), V2(0, 0), V2(5, 5)).lout;
	
	Rectangle(render_context, vision_config_list.bounds, V4(0.3, 0.3, 0.3, 0.6));
	RectangleOutline(render_context, vision_config_list.bounds, V4(0, 0, 0, 1));

	TemporaryMemoryArena temp_memory = BeginTemporaryMemory(dashstate->generic_arena);
	
	b32 previous_enable_state = dashstate->vision.enabled;
	
	ToggleSlider(&vision_config_list, &dashstate->vision.enabled, Literal("Enabled"), Literal("Disabled"), V2(120, 20), V2(0, 0), V2(0, 5));
	NextLine(&vision_config_list);
	
	if(previous_enable_state && !dashstate->vision.enabled && dashstate->robot.connected)
	{
		SendSetFloat(dashstate->net_state, 5, 0);
	}
	
	SliderBar(&vision_config_list, -80, 80, &dashstate->vision.brightness, V2(GetSize(vision_config_list.bounds).x, 20), V2(0, 0), V2(0, 0));
	NextLine(&vision_config_list);
	Text(&vision_config_list, Concat(Literal("Brightness: "), ToString((r32)dashstate->vision.brightness, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_config_list);
	
	SliderBar(&vision_config_list, 1, 30, &dashstate->vision.tracks_per_second, V2(GetSize(vision_config_list.bounds).x, 20), V2(0, 0), V2(0, 0));
	NextLine(&vision_config_list);
	Text(&vision_config_list, Concat(Literal("Tracks Per Second: "), ToString(dashstate->vision.tracks_per_second, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_config_list);
	
	Text(&vision_config_list, Concat(Literal("Left Limit: "), ToString(dashstate->vision.left_limit, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_config_list);
	TextBox(&vision_config_list, &dashstate->vision.left_limit, V2(GetSize(vision_config_list.bounds).x, 20), V2(0, 0), V2(0, 0));
	NextLine(&vision_config_list);
	
	Text(&vision_config_list, Concat(Literal("Right Limit: "), ToString(dashstate->vision.right_limit, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_config_list);
	TextBox(&vision_config_list, &dashstate->vision.right_limit, V2(GetSize(vision_config_list.bounds).x, 20), V2(0, 0), V2(0, 0));
	NextLine(&vision_config_list);
	
	Text(&vision_config_list, Concat(Literal("Shooter Target: "), ToString(dashstate->vision.shooter_target, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_config_list);
	TextBox(&vision_config_list, &dashstate->vision.shooter_target, V2(GetSize(vision_config_list.bounds).x, 20), V2(0, 0), V2(0, 0));
	NextLine(&vision_config_list);
	
	Text(&vision_config_list, Concat(Literal("Shooter Threshold: "), ToString(dashstate->vision.shooter_threshold, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_config_list);
	TextBox(&vision_config_list, &dashstate->vision.shooter_threshold, V2(GetSize(vision_config_list.bounds).x, 20), V2(0, 0), V2(0, 0));
	NextLine(&vision_config_list);
	
	if(Button(&vision_config_list, NULL, Literal("Camera Reconnect"), V2(120, 40), V2(0, 0), V2(5, 5)).state)
	{
		delete dashstate->vision.camera;
		dashstate->vision.camera = new cv::VideoCapture(CAMERA_CONNECTION);
	}
	
	if(Button(&vision_config_list, NULL, Literal("Set References"), V2(120, 40), V2(0, 0), V2(5, 5)).state)
	{
		if(dashstate->vision.target_hit)
		{
			dashstate->vision.top_reference = dashstate->vision.top_target;
			dashstate->vision.bottom_reference = dashstate->vision.bottom_target;
		}
	}
	
	Rectangle(render_context, vision_info_page.bounds, V4(0.3, 0.3, 0.3, 0.6));
	RectangleOutline(render_context, vision_info_page.bounds, V4(0, 0, 0, 1));
	
	Text(&vision_info_page, dashstate->vision.camera->isOpened() ? Literal("Camera Active") : Literal("Camera Not Active"), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_info_page);
	
	Text(&vision_info_page, Concat(Literal("Movement: "), ToString((r32)dashstate->vision.movement, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_info_page);

	Text(&vision_info_page, dashstate->vision.target_hit ? Literal("Target Hit") : Literal("Target Not Hit"), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_info_page);
	
	Text(&vision_info_page, Concat(Literal("Turret Speed: "), ToString((r32)dashstate->vision.turret_speed, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_info_page);
	
	Text(&vision_info_page, Concat(Literal("Last Track Timestamp: "), ToString((r32)dashstate->vision.last_track_time, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
	NextLine(&vision_info_page);
	
	if(dashstate->robot.connected)
	{
		r32 potentiometer_reading = GetLatestSample(dashstate->robot.hardware + 9)->potentiometer;
	
		Text(&vision_info_page, Concat(Literal("Potentiometer: "), ToString(potentiometer_reading, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
		NextLine(&vision_info_page);
			
		Text(&vision_info_page, Concat(Literal("Swivel Speed: "), ToString(dashstate->vision.sweep_speed, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
		NextLine(&vision_info_page);
		
		r32 motor_reading = GetLatestSample(dashstate->robot.hardware + 1)->motor;
		
		Text(&vision_info_page, Concat(Literal("Shooter Motor: "), ToString(motor_reading, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
		NextLine(&vision_info_page);
		
		r32 speed_diff = Abs(motor_reading - dashstate->vision.shooter_target);
		b32 speed_hit = (speed_diff < dashstate->vision.shooter_threshold);
		
		Text(&vision_info_page, Concat(speed_hit ? Literal("Speed Hit") : Literal("Speed Not Hit: "), ToString(speed_diff, &temp_memory), &temp_memory), 20, V2(0, 0), V2(0, 5));
		NextLine(&vision_info_page);
	}
	
	if(dashstate->vision.camera->isOpened())
	{
		v2 camera_resolution = V2(dashstate->vision.camera->get(CV_CAP_PROP_FRAME_WIDTH), dashstate->vision.camera->get(CV_CAP_PROP_FRAME_HEIGHT));
		element camera_view = Rectangle(&vision_info_page, V4(0, 0, 0, 1), camera_resolution, V2(0, 0), V2(5, 5));
		
		if(dashstate->vision.frame_grab_success)
		{
			DrawMat(render_context, camera_view.bounds.min, dashstate->vision.grabbed_frame);
		}
		else
		{
			Text(render_context, camera_view.bounds.min, Literal("No Frame"), 20);
		}
		
		RectangleOutline(render_context,
						 RectMinSize(dashstate->vision.top_reference.min + camera_view.bounds.min, GetSize(dashstate->vision.top_reference)),
						 V4(1, 0, 0, 1));
		RectangleOutline(render_context,
						 RectMinSize(dashstate->vision.bottom_reference.min + camera_view.bounds.min, GetSize(dashstate->vision.bottom_reference)),
						 V4(1, 0, 0, 1));
		
		if(dashstate->vision.target_hit)
		{
			RectangleOutline(render_context,
							 RectMinSize(dashstate->vision.top_target.min + camera_view.bounds.min, GetSize(dashstate->vision.top_target)),
							 V4(0, 0, 1, 1));
			RectangleOutline(render_context,
							 RectMinSize(dashstate->vision.bottom_target.min + camera_view.bounds.min, GetSize(dashstate->vision.bottom_target)),
							 V4(0, 0, 1, 1));
		}
	}
	
	EndTemporaryMemory(temp_memory);
}