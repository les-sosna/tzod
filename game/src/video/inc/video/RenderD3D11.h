#pragma once
#include <memory>

struct IRender;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;

std::unique_ptr<IRender> RenderCreateD3D11(ID3D11DeviceContext *context, ID3D11RenderTargetView *rtv);
