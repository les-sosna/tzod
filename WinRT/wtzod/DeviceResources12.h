#pragma once
#include <wrl/client.h>
#include <memory>

struct ID3D12Device;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;

struct IRender;
class RenderBinding;
class RenderD3D12;
enum DisplayOrientation;

class DeviceResources12
{
public:
	explicit DeviceResources12(Windows::UI::Core::CoreWindow^ coreWindow);
	~DeviceResources12();
	bool ValidateDevice() const;
	bool IsDeviceRemoved() const;
	void Present();

	IRender& GetRender(int width, int height, DisplayOrientation displayOrientation);
	RenderBinding& GetRenderBinding() { return *_renderBinding; }

private:
	static constexpr int c_frameCount = 2;
	Microsoft::WRL::ComPtr<ID3D12Device> _d3dDevice;
	Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> _swapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> _renderTargets[c_frameCount];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocators[c_frameCount];
	UINT _rtvDescriptorSize;

	Microsoft::WRL::ComPtr<ID3D12Fence> _fence;
	UINT64 _fenceValues[c_frameCount];
	UINT _currentFrame;
	struct HandleDeleter
	{
		typedef HANDLE pointer;
		void operator()(HANDLE h) { ::CloseHandle(h); }
	};
	std::unique_ptr<HANDLE, HandleDeleter> _fenceEvent;

	int _pxWidth = 0;
	int _pxHeight = 0;
	DisplayOrientation _displayOrientation;

	std::unique_ptr<RenderD3D12> _render;
	std::unique_ptr<RenderBinding> _renderBinding;

	HRESULT ResizeSwapChainInternal(int width, int height, DisplayOrientation displayOrientation);
};
