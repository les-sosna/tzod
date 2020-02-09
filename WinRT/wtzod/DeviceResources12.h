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

class DeviceResources12
{
public:
	explicit DeviceResources12(Windows::UI::Core::CoreWindow^ coreWindow);
	~DeviceResources12();
	bool ValidateDevice() const;
	bool IsDeviceRemoved() const;
	void Present();

	IRender& GetRender(int rotationAngle, int width, int height);
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

	int m_pxWidth = 0;
	int m_pxHeight = 0;
	int m_rotationAngle = -1;

	std::unique_ptr<IRender> _render;
	std::unique_ptr<RenderBinding> _renderBinding;

	HRESULT ResizeSwapChainInternal(int width, int height, int rotationAngle);
};
