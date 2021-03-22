#pragma once
#include "parameters.hpp"
#ifdef USING_RPI
int setup(const int devId);

bool hasResponded(int fd);
#endif