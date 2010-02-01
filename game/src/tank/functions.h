// functions.h

#pragma once

//-------------------------------------------------------

bool PtInFRect(const FRECT &rect, const vec2d &pt);
void RectToFRect(LPFRECT lpfrt, const LPRECT lprt);
void FRectToRect(LPRECT  lprt,  const LPFRECT lpfrt);
void OffsetFRect(LPFRECT lpfrt, float x, float y);
void OffsetFRect(LPFRECT lpfrt, const vec2d &x);

// generates a pseudo random number in range [0, max)
float frand(float max);
// generates a pseudo random vector of the specified length
vec2d vrand(float len);

DWORD CalcCRC32(const void *data, size_t size);

bool PauseGame(bool pause);

string_t StrFromErr(DWORD dwMessageId);

///////////////////////////////////////////////////////////////////////////////
// end of file
