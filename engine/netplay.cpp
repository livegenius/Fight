#include <cmath>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include <string>
#include <enet/enet.h>

#include "netplay.h"
#include "main.h"
#include "raw_input.h"

namespace net {

Result NetplayArgs(int argc, char** argv, ENetHost *&local, std::string &address_)
{
		if (enet_initialize() != 0) {
			std::cerr << "An error occurred while initializing ENet.\n";
			return FailedToInit;
		}

		bool success = false;
		bool host = false;
		int inputDelay;
		local = nullptr;
		if(argc == 3)
		{
			std::string address(argv[1]);
			std::string port(argv[2]);
			std::cout << "Joining " << address << ":" << port << "...\n";
			
			success = Join(address, port, local);
			address_ = std::move(address);
		}
		else if(argc == 2)
		{
			std::string port(argv[1]);
			std::cout << "Hosting at " << port << "...\n";
			success = Host(port, local);
			if(!local)
				return NeedsQuit;
			host = true;
		}

		if(success)
		{
			std::cout << "Success\n";
			/* 
			std::cout << "Enter input delay (0-30): ";
            std::cin >> inputDelay;
            if(!std::cin.good())
			{
				std::cout << "Invalid input.\n";
				std::cin.get();
				return FailedNeedsCleanup;
			} */
		}
		else
		{
			std::cout << "Something failed\n";
			std::cout << "\nUsage: \tHosting: <port>\n\t\tJoining: <adress> <port>\n";
			return FailedNeedsCleanup;
		}

	if(host)
		return Hosting;
	else
		return Joining;
}

}

bool Host(std::string portStr, ENetHost *&oServer)
{
	unsigned short port = 0;
	try
	{
		port = std::stoi(portStr);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << " exception: Invalid port number: " << port << "\n";
		return false;
	}

	ENetAddress address;
	ENetHost * server;

	address.host = ENET_HOST_ANY;
	address.port = port;

	server = enet_host_create (&address, 1,2,0,0);
	if(!server)
		return false;
	oServer = server;
	std::cout << "Hosting at "<<server->address.host<<":"<<server->address.port<<"\n";

	ENetEvent event;
	int tick = 0;
	for(;;)
	{	
		if(PollShouldQuit())
		{
			std::cout << "Quitting";
			enet_host_destroy(server);
			oServer = nullptr;
			return false;
		}
		while (enet_host_service (server, & event, 120) > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
			{
				std::cout<<"A new client connected from "<<
						event.peer -> address.host<<":"<<
						event.peer -> address.port<<"\n";
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
			{
				if(event.packet->dataLength >= 3 && strncmp((const char*)event.packet->data, "OK1",3)==0)
				{
					std::cout << "Received OK1 from "<<event.peer -> address.host<<":"<<event.peer->address.port<<"\n";
					enet_packet_destroy (event.packet);
					ENetPacket * packet = enet_packet_create ("OK2", 3, ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(event.peer, 1, packet);
					enet_host_service(server,&event,500);
					return true;
				}
				enet_packet_destroy (event.packet);	
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT:
				std::cout<< "peer disconnected.\n";
				break;
			case ENET_EVENT_TYPE_NONE:
				break;
			}
		}
		if(event.type != ENET_EVENT_TYPE_NONE)
			std::cout << event.type <<"\n";
		else if(tick%16 == 0)
			std::cout << ".";
		++tick;
	}
}

bool Join(std::string addressStr, std::string portStr, ENetHost *&oClient)
{
	unsigned short port = 0;

	ENetAddress address;
	try
	{
		if(enet_address_set_host(&address, addressStr.c_str()) < 0)
			return false;
		address.port = port = std::stoi(portStr);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << " exception at Join()\n";
		return false;
	}

	ENetHost * client;
	client = enet_host_create(nullptr,1,2,0,0);
	if(!client)
	{
		std::cerr << "Can't create client host.\n";
		return false;
	}
	oClient = client;
	
	ENetPeer *peer;
	peer = enet_host_connect (client, & address, 2, 0);
	if(!peer)
	{
		std::cerr << "Can't initiate connection to foreign host.\n";
		return false;
	}

	ENetEvent event;
	/* Wait up to 10 seconds for a connection event. */
	for(int i = 0; i < 10; ++i)
	{
		if (enet_host_service (client, &event, 1000) > 0 && event.type ==  ENET_EVENT_TYPE_CONNECT)
		{
			std::cout<<"A new client connected from "<< 
					event.peer -> address.host<<":"<<
					event.peer -> address.port<<"\n";
			/* Store any relevant client information here. */
			//event.peer -> data = "Client information";
			goto success;
		}
	}
	std::cerr << "Couldn't connect to host\n";
	enet_peer_reset (peer);
	return false;
success:
	ENetPacket * packet = enet_packet_create ("OK1", 3, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send (peer, 0, packet);
	std::cout <<"I am "<<client->address.host<<"\n";
	while (enet_host_service (client, &event, 3000) > 0)
	{
		if(event.type == ENET_EVENT_TYPE_RECEIVE)
		{
			if(event.packet->dataLength >= 3 && strncmp((const char*)event.packet->data, "OK2",3)==0)
			{
				std::cout << "Received OK2\n";
				enet_packet_destroy (event.packet);
				return true;
			}
			enet_packet_destroy (event.packet);
			break;
		}
		else{
			auto e = event.type;
			std::cout << e; 
		}
	}

	enet_peer_reset (peer);
	std::cerr << "Forcing disconnect";
	return false;
}
