#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>
#include <ostream>

//#define DEBUG ON

#ifdef DEBUG
#define DEBUG_MSG(msg, var) (std::cerr << "[DEBUG] " << msg << ": " << var << std::endl);
#else
#define DEBUG_MSG(msg, var) ((void)0)
#endif

//#define DEBUG_1 ON

#ifdef DEBUG_1
#define DEBUG_MSG_1(msg, var) (std::cerr << "[DEBUG_1] " << msg << ": " << var << std::endl);
#else
#define DEBUG_MSG_1(msg, var) ((void)0)
#endif

//#define DEBUG_2 ON

#ifdef DEBUG_2
#define DEBUG_MSG_2(msg, var) (std::cerr << "[DEBUG_3] " << msg << ": " << var << std::endl);
#else
#define DEBUG_MSG_2(msg, var) ((void)0)
#endif

//#define DEBUG_3 ON

#ifdef DEBUG_3
#define DEBUG_MSG_3(msg, var) (std::cerr << "[DEBUG_3] " << msg << ": " << var << std::endl);
#else
#define DEBUG_MSG_3(msg, var) ((void)0)
#endif

#endif // DEBUG_HPP