#include <cmath>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include <string>
#include <enet.h>

#include "netplay.h"
#include "main.h"

namespace net {

Result NetplayArgs(int argc, char** argv)
{
		if (enet_initialize() != 0) {
			std::cerr << "An error occurred while initializing ENet.\n";
			return FailedToInit;
		}

		bool success = false;
		bool host = false;
		int inputDelay;
		ENetHost *oHost = nullptr;
		if(argc == 3)
		{
			std::string address(argv[1]);
			std::string port(argv[2]);
			std::cout << "Joining " << address << ":" << port << "...\n";
			
			success = Join(address, port, oHost);
		}
		else if(argc == 2)
		{
			std::string port(argv[1]);
			std::cout << "Hosting at " << port << "...\nCTRL+C to close.\n\n";
			success = Host(port, oHost);
			host = true;
		}

		if(oHost)
			enet_host_destroy(oHost);

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

bool Host(std::string portStr, ENetHost *oServer)
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
	server = oServer;

	ENetEvent event;
	while (enet_host_service (server, & event, 10000) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			printf ("A new client connected from %x:%u.\n", 
					event.peer -> address.host,
					event.peer -> address.port);
			/* Store any relevant client information here. */
			event.peer -> data = (void*)"Client information";
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			printf ("A packet of length %zu containing %s was received from %s on channel %u.\n",
					event.packet -> dataLength,
					event.packet -> data,
					event.peer -> data,
					event.channelID);
			/* Clean up the packet now that we're done using it. */
			enet_packet_destroy (event.packet);
			
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			printf ("%s disconnected.\n", (char *)event.peer->data);
			/* Reset the peer's client information. */
			event.peer->data = NULL;
			break;

		case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			printf ("%s timeout.\n", (char *)event.peer->data);
			event.peer->data = NULL;
			break;

		case ENET_EVENT_TYPE_NONE: break;
		}
	}

	return true;
}

bool Join(std::string addressStr, std::string portStr, ENetHost *oClient)
{
	unsigned short port = 0;

	ENetAddress address;
	try
	{
		
		enet_address_set_host(&address, addressStr.c_str());
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
			printf ("A new client connected from %x:%u.\n", 
					event.peer -> address.host,
					event.peer -> address.port);
			/* Store any relevant client information here. */
			//event.peer -> data = "Client information";
			goto success;
		}
	}
	std::cerr << "Couldn't connect to host\n";
	enet_peer_reset (peer);
	return false;
success:
	const char* test = "Hello there.";
	ENetPacket * packet = enet_packet_create (test, strlen(test) + 1, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send (peer, 0, packet);
	enet_peer_disconnect(peer,0);

	while (enet_host_service (client, &event, 3000) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_RECEIVE:
			enet_packet_destroy (event.packet);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			puts ("Disconnection succeeded.");
			return true;
		}
	}

	enet_peer_reset (peer);
	std::cerr << "Forcing disconnect";
	return false;
}
