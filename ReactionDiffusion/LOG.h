#pragma once
#include <iostream>
#include <limits>


#ifndef LOG_OUT
    #define LOG_OUT std::clog
#endif

#ifndef ERR_OUT
    #define ERR_OUT std::cerr
#endif

#define LOG(msg)				LOG_OUT << "LOG: [" << __FUNCTION__ << "] " << msg << "\n"
#define PAUSE()					LOG_OUT << "Press ENTER to continue..."; while(std::cin.peek() == -1); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); std::cin.clear()
#define ASSERT(cond, msg)		if(!(cond)) { ERR_OUT << "ASSERT: [" << __FUNCTION__ << "] " << #cond << ": " << msg << "\n"; PAUSE(); }

