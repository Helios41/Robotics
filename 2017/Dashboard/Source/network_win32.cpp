#include "packet_definitions.h"

struct NetworkState
{
   SOCKET socket;
   struct sockaddr_in server_info;
   b32 bound;
   r64 last_packet;
};

void NetworkReconnect(NetworkState *net_state, network_settings *net_settings)
{  
   //TODO: make the notification system cache its messages so we dont have
   //      to worry about the lifetime of the strings we pass
   /*
   AddMessage((Console *) &params->dashstate->console,
            "Attempting to connect to: " Literal((char *) params->connect_to),
            network_notification);
   */
   
   char *host_ip = net_settings->is_mdns ? NULL : net_settings->connect_to.text;
   //TODO: this is super unsafe, our strings DO NOT need to end in a \0 so we need a proper convert to CString function
   
   if(net_settings->is_mdns)
   {
      hostent *robot_server = gethostbyname((char *) net_settings->connect_to.text);
   
      if(robot_server)
      {
         host_ip = inet_ntoa(*((in_addr *)robot_server->h_addr));
         
		 /*
         AddMessage((Console *) &params->dashstate->console,
                  "MDNS IP: " Literal(host_ip),
                  network_notification);
		 */
      }
      else
      {
		  /*
         AddMessage((Console *) &params->dashstate->console, Literal("MDNS IP address lookup FAILED"),
                  network_notification);
				  */
      }
   }
   
   /*
   AddMessage((Console *) &params->dashstate->console,
            host_ip ? concat "Host IP: " Literal(host_ip) : Literal("Address: NULL"),
            network_notification);
   */
   
   if(host_ip)
   {
      net_state->server_info.sin_family = AF_INET;
      net_state->server_info.sin_addr.s_addr = inet_addr(host_ip);
      net_state->server_info.sin_port = htons(5800);
      
      generic_packet_header packet = {};
      packet.size = sizeof(packet);
      packet.type = PACKET_TYPE_JOIN;
      
      sendto(net_state->socket, (const char *) &packet, sizeof(packet), 0,
             (struct sockaddr *) &net_state->server_info, sizeof(net_state->server_info));
   }
}

RobotHardwareType ConvertType(u8 type)
{
	Assert(type != HARDWARE_TYPE_INVALID);
	
	switch(type)
	{
		case HARDWARE_TYPE_MOTOR:
			return Hardware_Motor;
		
		case HARDWARE_TYPE_SOLENOID:
			return Hardware_Solenoid;
		
		case HARDWARE_TYPE_DRIVE:
			return Hardware_Drive;
		
		case HARDWARE_TYPE_SWITCH:
			return Hardware_Switch;
		
		case HARDWARE_TYPE_CAMERA:
			return Hardware_Camera;
			
		case HARDWARE_TYPE_DISTANCE_SENSOR:
			return Hardware_DistanceSensor;
			
		case HARDWARE_TYPE_LIGHT:
			return Hardware_Light;
		
		default:
			InvalidCodePath;
	}
	
	return (RobotHardwareType)0;
}

void HandlePacket(u8 *buffer, Robot *robot)
{
   generic_packet_header *header = (generic_packet_header *) buffer;
   Assert(header->type != PACKET_TYPE_INVALID);
   
   if(header->type == PACKET_TYPE_WELCOME)
   {
      welcome_packet_header *welcome_header = (welcome_packet_header *) header;
	  
	  robot->name = String((char *) malloc(sizeof(char) * 16), 16);
	  CopyTo(String(welcome_header->name, 16), robot->name);
      
	  robot->hardware = (RobotHardware *) malloc(sizeof(RobotHardware) * welcome_header->hardware_count);
	  robot->hardware_count = welcome_header->hardware_count;
	  Clear(robot->hardware, sizeof(RobotHardware) * welcome_header->hardware_count);
	  
	  robot->functions = (RobotBuiltinFunction *) malloc(sizeof(RobotBuiltinFunction) * welcome_header->function_count);
	  robot->function_count = welcome_header->function_count;
	  Clear(robot->functions, sizeof(RobotBuiltinFunction) * welcome_header->function_count);
	  
	  robot_hardware *hardware = (robot_hardware *)(welcome_header + 1);
	  robot_function *functions = (robot_function *)(hardware + welcome_header->hardware_count);
	  
	  for(u32 i = 0;
		  i < welcome_header->hardware_count;
		  i++)
	  {
         robot_hardware *curr_hardware_in = hardware + i;
		 RobotHardware *curr_hardware = robot->hardware + i;
		 
		 curr_hardware->type = ConvertType(curr_hardware_in->type);
		 curr_hardware->id = curr_hardware_in->id;
		 curr_hardware->name = String((char *) malloc(sizeof(char) * 16), 16);
		 CopyTo(String(curr_hardware_in->name, 16), curr_hardware->name);
	  }
	  
	  for(u32 i = 0;
		  i < welcome_header->function_count;
		  i++)
	  {
         robot_function *curr_function_in = functions + i;
		 RobotBuiltinFunction *curr_function = robot->functions + i;
		 
		 curr_function->id = curr_function_in->id;
		 curr_function->name = String((char *) malloc(sizeof(char) * 16), 16);
		 CopyTo(String(curr_function_in->name, 16), curr_function->name);
	  }
	  
	  robot->connected = true;
   }
   else if(header->type == PACKET_TYPE_HARDWARE_SAMPLE)
   {
	   hardware_sample_packet_header *hardware_sample_header = (hardware_sample_packet_header *) header;
	   RobotHardware *hardware = NULL;
	   
	   for(u32 i = 0;
		   i < robot->hardware_count;
		   i++)
	   {
		   RobotHardware *curr_hardware = robot->hardware + i;
		   if(curr_hardware->id == hardware_sample_header->id)
		   {
			   hardware = curr_hardware ;
			   break;
		   }
	   }
	   
	   if(hardware)
	   {
		   RobotHardwareSample sample = {};
		   sample.timestamp = hardware_sample_header->timestamp;
		   
		   Assert((hardware_sample_header->type != HARDWARE_TYPE_INVALID) &&
				  (hardware_sample_header->type != HARDWARE_TYPE_CAMERA));
		   
		   switch(hardware->type)
		   {
			     case Hardware_Motor:
					sample.state.motor = hardware_sample_header->motor;
					break;
				 
				 case Hardware_Solenoid:
					sample.state.solenoid = hardware_sample_header->solenoid;
					break;
				 
				 case Hardware_Drive:
					sample.state.forward = hardware_sample_header->forward;
					sample.state.rotate = hardware_sample_header->rotate;
					break;
				 
				 case Hardware_Switch:
					sample.state._switch = hardware_sample_header->_switch;
					break;
				 
				 case Hardware_DistanceSensor:
					sample.state.distance_sensor = hardware_sample_header->distance_sensor;
					break;
				 
				 case Hardware_Light:
					sample.state.light = hardware_sample_header->light;
					break;
		   }
		   
		   if(sample.timestamp > hardware->samples[(hardware->at_sample - 1) % 64].timestamp)
		   {
		      hardware->samples[hardware->at_sample] = sample;
			  hardware->at_sample = (hardware->at_sample + 1) % 64;
		   }
	   }
   }
   else if(header->type == PACKET_TYPE_UPLOADED_STATE)
   {
	   uploaded_state_packet_header *uploaded_state_header = (uploaded_state_packet_header *) header;
	   {
		   
	   }
   }
   else if(header->type == PACKET_TYPE_CAMERA_FEED)
   {
	   {
		   
	   }
   }
   else if(header->type == PACKET_TYPE_DEBUG_MESSAGE)
   {
	   {
		   
	   }
   }
}

void HandlePackets(NetworkState *net_state, Robot *robot, r64 curr_time)
{
	b32 has_packets = true;
    while(has_packets)
    {
		char buffer[512] = {};
         
        struct sockaddr_in sender_info = {};
        int sender_info_size = sizeof(sender_info);
        
        int recv_return = recvfrom(net_state->socket, buffer, sizeof(buffer), 0,
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
			net_state->last_packet = curr_time;
			HandlePacket((u8 *)buffer, robot);
        }
	}	
}