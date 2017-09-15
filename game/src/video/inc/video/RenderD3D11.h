#pragma once
#include <memory>

struct IRender;
struct ID3D11DeviceContext;

std::unique_ptr<IRender> RenderCreateD3D11(ID3D11DeviceContext *context);
