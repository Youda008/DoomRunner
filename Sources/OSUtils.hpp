//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  17.1.2021
// Description: OS-specific utils
//======================================================================================================================

#ifndef OS_UTILS_INCLUDED
#define OS_UTILS_INCLUDED


#include "Common.hpp"

#include <string>
#include <vector>


//======================================================================================================================

struct MonitorInfo
{
	std::string name;
	int width;
	int height;
	bool isPrimary;
};
std::vector< MonitorInfo > listMonitors();


#endif // OS_UTILS_INCLUDED
