#pragma once

#define ReturnIfFailed(hr) { if (FAILED(hr)) { __debugbreak(); return; } }

struct DeviceResources
{
    DeviceResources();

    void Present();

    struct DeviceIndependentResources
    {
        struct {
            Microsoft::WRL::ComPtr<ID2D1Factory3> factory;
        } d2d;
    } m_deviceIndependentResources;

    void InitializeDeviceIndependentResources();

    struct DeviceDependentResources
    {
        struct {
            Microsoft::WRL::ComPtr<ID3D11Device> device;
            D3D_FEATURE_LEVEL featureLevel;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
        } d3d;
        struct {
            Microsoft::WRL::ComPtr<ID2D1Device2> device;
            Microsoft::WRL::ComPtr<ID2D1DeviceContext2> context;
        } d2d;
    } m_deviceDependentResources;

    void InitializeDeviceResources();

    struct WindowDependentResources
    {
        struct {
            winrt::Windows::Foundation::Size renderTargetSize;
            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
            D3D11_VIEWPORT screenViewport;
        } d3d;
        struct {
            Microsoft::WRL::ComPtr<ID2D1Bitmap1> targetBitmap;
        } d2d;
    } m_windowDependentResources;

    void InitializeWindowResources(winrt::Windows::UI::Core::CoreWindow const & window);
};
