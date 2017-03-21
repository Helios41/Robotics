struct NetworkState
{
   SOCKET socket;
   struct sockaddr_in server_info;
   b32 bound;
   r64 last_packet_recieved;
   r64 last_packet_sent;
};

void SendGenericPacket(NetworkState *net_state, u8 type)
{
	generic_packet_header packet = {};
    packet.size = sizeof(packet);
    packet.type = type;
      
    sendto(net_state->socket, (const char *) &packet, sizeof(packet), 0,
           (struct sockaddr *) &net_state->server_info, sizeof(net_state->server_info));
}

void NetworkReconnect(NetworkState *net_state, network_settings *net_settings)
{  
   //TODO: this is super unsafe, our strings DO NOT need to end in a \0 so we need a proper convert to CString function
   char *host_ip = net_settings->is_mdns ? NULL : net_settings->connect_to.text;
   
   if(net_settings->is_mdns)
   {
      hostent *robot_server = gethostbyname((char *) net_settings->connect_to.text);
   
      if(robot_server)
      {
         host_ip = inet_ntoa(*((in_addr *)robot_server->h_addr));   
	  }
   }
   
   if(host_ip)
   {
	  net_state->server_info.sin_family = AF_INET;
      net_state->server_info.sin_addr.s_addr = inet_addr(host_ip);
      net_state->server_info.sin_port = htons(5800);
    
	  SendGenericPacket(net_state, PACKET_TYPE_JOIN);
   }
}

void SendPing(NetworkState *net_state)
{
	SendGenericPacket(net_state, PACKET_TYPE_PING);
}

void UploadAutonomous(NetworkState *net_state, AutonomousEditor *auto_builder, u8 slot)
{
	u32 block_count = 0;
	FunctionBlockLink *last_link = auto_builder->coroutine.first_block;
	while(last_link){ last_link = last_link->next; block_count++; };
	
	u32 packet_size = sizeof(upload_autonomous_packet_header) +
					  sizeof(FunctionBlock) * block_count;
	u8 *packet = (u8 *) malloc(packet_size);
	
	upload_autonomous_packet_header *upload_auto_header = (upload_autonomous_packet_header *) packet;
    upload_auto_header->header.size = packet_size;
    upload_auto_header->header.type = PACKET_TYPE_UPLOAD_AUTONOMOUS;
    upload_auto_header->block_count = block_count;
	CopyTo(auto_builder->name, String(upload_auto_header->name, 32));
	upload_auto_header->slot = slot;
	
	FunctionBlock *blocks = (FunctionBlock *)(upload_auto_header + 1);
	
	u32 i = 0;
	for(FunctionBlockLink *curr_block_link = auto_builder->coroutine.first_block;
		curr_block_link;
		curr_block_link = curr_block_link->next)
	{
		blocks[i++] = curr_block_link->block;
	}
	  
    sendto(net_state->socket, (const char *) packet, packet_size, 0,
           (struct sockaddr *) &net_state->server_info, sizeof(net_state->server_info));
		   
	free(packet);
}

void UploadControls(NetworkState *net_state)
{
	
}

void RequestUploadedState(NetworkState *net_state)
{
	SendGenericPacket(net_state, PACKET_TYPE_REQUEST_UPLOADED_STATE);
}

void SendSetFloat(NetworkState *net_state, u32 hardware_index, r32 value)
{
	set_float_packet_header packet = {};
	packet.header.size = sizeof(packet);
	packet.header.type = PACKET_TYPE_SET_FLOAT;
	packet.index = hardware_index;
	packet.value = value;
	
	sendto(net_state->socket, (const char *) &packet, sizeof(packet), 0,
           (struct sockaddr *) &net_state->server_info, sizeof(net_state->server_info));
}

void SendSetMultiplier(NetworkState *net_state, u32 hardware_index, r32 multiplier)
{
	set_multiplier_packet_header packet = {};
	packet.header.size = sizeof(packet);
	packet.header.type = PACKET_TYPE_SET_MULTIPLIER;
	packet.index = hardware_index;
	packet.multiplier = multiplier;
	
	sendto(net_state->socket, (const char *) &packet, sizeof(packet), 0,
           (struct sockaddr *) &net_state->server_info, sizeof(net_state->server_info));
}

void SendArcadeDrive(NetworkState *net_state, r32 power, r32 rotate)
{
	arcade_drive_packet_header packet = {};
	packet.header.size = sizeof(packet);
	packet.header.type = PACKET_TYPE_ARCADE_DRIVE;
	packet.power = power;
	packet.rotate = rotate;
	
	sendto(net_state->socket, (const char *) &packet, sizeof(packet), 0,
           (struct sockaddr *) &net_state->server_info, sizeof(net_state->server_info));
}

void SendSetDriveMultiplier(NetworkState *net_state, r32 multiplier)
{
	set_drive_multiplier_packet_header packet = {};
	packet.header.size = sizeof(packet);
	packet.header.type = PACKET_TYPE_SET_DRIVE_MULTIPLIER;
	packet.multiplier = multiplier;
	
	sendto(net_state->socket, (const char *) &packet, sizeof(packet), 0,
           (struct sockaddr *) &net_state->server_info, sizeof(net_state->server_info));
}

void HandlePacket(MemoryArena *arena, u8 *buffer, DashboardState *dashstate)
{
	Console *console = &dashstate->console;
	Robot *robot = &dashstate->robot;
	
   generic_packet_header *header = (generic_packet_header *) buffer;
   Assert(header->type != PACKET_TYPE_INVALID);
   
   if(header->type == PACKET_TYPE_WELCOME)
   {
	  welcome_packet_header *welcome_header = (welcome_packet_header *) header;
	  
	  robot->name = String((char *) malloc(sizeof(char) * strlen(welcome_header->name)), strlen(welcome_header->name));
	  CopyTo(String(welcome_header->name, strlen(welcome_header->name)), robot->name);
      
	  robot->drive_encoder = welcome_header->drive_encoder;
	  
	  //TODO: way to deallocate these on resend?
	  robot->hardware_count = welcome_header->hardware_count;
	  robot->hardware = PushArray(arena, robot->hardware_count, RobotHardware, Arena_Clear);
	  robot_hardware *hardware = (robot_hardware *)(welcome_header + 1);
	  
	  for(u32 i = 0;
		  i < welcome_header->hardware_count;
		  i++)
	  {
         robot_hardware *curr_hardware_in = hardware + i;
		 RobotHardware *curr_hardware = robot->hardware + i;
		 
		 curr_hardware->type = (RobotHardwareType) curr_hardware_in->type;
		 curr_hardware->name = String((char *) malloc(sizeof(char) * strlen(curr_hardware_in->name)), strlen(curr_hardware_in->name));
		 CopyTo(String(curr_hardware_in->name, strlen(curr_hardware_in->name)), curr_hardware->name);
	  }
	  
	  robot->connected = true;
   }
   else if(header->type == PACKET_TYPE_HARDWARE_SAMPLE)
   {
	   hardware_sample_packet_header *hardware_sample_header = (hardware_sample_packet_header *) header;
	   
	   if(hardware_sample_header->index < robot->hardware_count)
	   {   
		   RobotHardware *hardware = robot->hardware + hardware_sample_header->index;
		   RobotHardwareSample sample = hardware_sample_header->sample;
		   
		   if(sample.timestamp > hardware->samples[(hardware->at_sample - 1) % ArrayCount(hardware->samples)].timestamp)
		   {
		      hardware->samples[hardware->at_sample] = sample;
			  hardware->at_sample = (hardware->at_sample + 1) % ArrayCount(hardware->samples);
			  dashstate->latest_sample_timestamp = sample.timestamp;
		   }
	   }
   }
   else if(header->type == PACKET_TYPE_DRIVE_SAMPLE)
   {
		drive_sample_packet_header *drive_sample_header = (drive_sample_packet_header *) header;
		RobotDriveSample sample = drive_sample_header->sample;
		   
		if(sample.timestamp > robot->samples[(robot->at_sample - 1) % ArrayCount(robot->samples)].timestamp)
		{
			robot->samples[robot->at_sample] = sample;
			robot->at_sample = (robot->at_sample + 1) % ArrayCount(robot->samples);
			dashstate->latest_sample_timestamp = sample.timestamp;
		}
   }
	else if(header->type == PACKET_TYPE_UPLOADED_STATE)
	{
		uploaded_state_packet_header *uploaded_state_header = (uploaded_state_packet_header *) header;
		
		string auto_name = String((char *) malloc(sizeof(char) * strlen(uploaded_state_header->autonomous_name)), strlen(uploaded_state_header->autonomous_name));
		CopyTo(String(uploaded_state_header->autonomous_name, strlen(uploaded_state_header->autonomous_name)), auto_name);
		
		dashstate->robot.uploaded_state.has_autonomous = uploaded_state_header->has_autonomous;
		dashstate->robot.uploaded_state.autonomous_name = auto_name;
	}
	else if(header->type == PACKET_TYPE_DEBUG_MESSAGE)
	{
		debug_message_packet_header *debug_message_packet  = (debug_message_packet_header *) header;
		string message = String((char *) malloc(sizeof(char) * strlen(debug_message_packet->text)), strlen(debug_message_packet->text));
		CopyTo(String(debug_message_packet->text, strlen(debug_message_packet->text)), message);
		
		AddMessage(console, message, debug_message_packet->is_autonomous_debug ? Category_RobotAutonomousDebug : Category_RobotDebug);
	}
}

void HandlePackets(MemoryArena *arena, NetworkState *net_state,
				   DashboardState *dashstate, r64 curr_time)
{
	b32 has_packets = true;
    while(has_packets)
    {
		char buffer[1024] = {};
         
        struct sockaddr_in sender_info = {};
        int sender_info_size = sizeof(sender_info);
        
        int recv_return = recvfrom(net_state->socket, buffer, 1024, 0,
                                   (struct sockaddr *) &sender_info, &sender_info_size);
        
        //TODO: check that sender_info equals net_state.server_info
        
        if(recv_return == SOCKET_ERROR)
        {
			int wsa_error = WSAGetLastError();
           
			if(wsa_error == WSAEWOULDBLOCK)
			{
				has_packets = false;
			}
			else if(wsa_error != 0)
			{
              
			}
        }
        else
        {
			net_state->last_packet_recieved = curr_time;
			HandlePacket(arena, (u8 *)buffer, dashstate);
        }
	}	
}