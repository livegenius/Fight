#ifndef NETPLAY_H_INCLUDED
#define NETPLAY_H_INCLUDED

#include <chrono>
#include <deque>
#include <mutex>
#include <enet.h>

bool Host(std::string portStr, ENetHost *oHost);
bool Join(std::string addressStr, std::string portStr, ENetHost *oClient);

namespace net {

enum Result{
	FailedToInit,
	FailedNeedsCleanup,
	Success,
	Hosting,
	Joining,
};

Result NetplayArgs(int argc, char** argv);

};


#endif // NETPLAY_H_INCLUDED
