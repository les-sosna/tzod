// functions.h

#pragma once

//-------------------------------------------------------

// Касание не считается за пересечение.
// Прямоугольники должны быть правильными
//     (т. е. left < right, top < bottom)
BOOL IsIntersect(const LPFRECT lprtRect1, const LPFRECT lprtRect2);
bool PtInFRect(const FRECT &rect, const vec2d &pt);
void RectToFRect(LPFRECT lpfrt, const LPRECT lprt);
void FRectToRect(LPRECT  lprt,  const LPFRECT lpfrt);
void OffsetFRect(LPFRECT lpfrt, float x, float y);
void OffsetFRect(LPFRECT lpfrt, const vec2d &x);

// генерация случайного числа от 0 до max
float frand(float max);
// генерация случайно направленого вектора длиной len
vec2d vrand(float len);

DWORD CalcCRC32(const void *data, size_t size);

bool PauseGame(bool pause);

string_t StrFromErr(DWORD dwMessageId);

///////////////////////////////////////////////////////////////////////////////
// end of file
