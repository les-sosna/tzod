// functions.h

#pragma once

//-------------------------------------------------------

#include <cstdint>
#include <string>

uint32_t CalcCRC32(const void *data, size_t size);

bool PauseGame(bool pause);

std::string StrFromErr(unsigned int messageId);

///////////////////////////////////////////////////////////////////////////////
// end of file
