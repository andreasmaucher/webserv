#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>

// #define DEBUG ON

#ifdef DEBUG
#define DEBUG_MSG(msg, var) std::cerr << "[DEBUG] " << msg << ": " << var << std::endl
#else
#define DEBUG_MSG(msg, var) ((void)0)
#endif

#define DEBUG_1 ON

#ifdef DEBUG_1
#define DEBUG_MSG_1(msg, var) std::cerr << "[DEBUG] " << msg << ": " << var << std::endl
#else
#define DEBUG_MSG_1(msg, var) ((void)0)
#endif

#endif // DEBUG_HPP