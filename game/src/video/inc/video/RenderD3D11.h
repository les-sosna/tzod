#pragma once
#include <memory>

struct IRender;

std::unique_ptr<IRender> RenderCreateD3D11();
