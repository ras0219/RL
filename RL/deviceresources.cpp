#include "pch.h"

#include "deviceresources.h"

using namespace Microsoft::WRL;

DeviceResources::DeviceResources()
{
    InitializeDeviceIndependentResources();
    InitializeDeviceResources();
}

void DeviceResources::InitializeDeviceIndependentResources()
{
    DeviceIndependentResources resources = {};

    // Initialize Direct2D resources.
    D2D1_FACTORY_OPTIONS options;
    ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
    // If the project is in a debug build, enable Direct2D debugging via SDK Layers.
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    // Initialize the Direct2D Factory.
    ReturnIfFailed(D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory3),
        &options,
        &resources.d2d.factory));

    // Initialize the DirectWrite Factory.
    ReturnIfFailed(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory3),
        &resources.dwrite.factory
    ));

    // Initialize the Windows Imaging Component (WIC) Factory.
    ReturnIfFailed(CoCreateInstance(
        CLSID_WICImagingFactory2,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&resources.dwrite.wicFactory)
    ));

    // Store resources if initialization succeeded.
    m_deviceIndependentResources = resources;
}

void DeviceResources::InitializeDeviceResources()
{
    DeviceDependentResources resources = {};

    HRESULT hr = S_OK;

    // This flag adds support for surfaces with a different color channel ordering
    // than the API default. It is required for compatibility with Direct2D.
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
        0,
        D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
        nullptr,                    // Any feature level will do.
        0,
        D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
        nullptr,                    // No need to keep the D3D device reference.
        nullptr,                    // No need to know the feature level.
        nullptr                     // No need to keep the D3D device context reference.
    );

    if (SUCCEEDED(hr))
    {
        // If the project is in a debug build, enable debugging via SDK Layers with this flag.
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
#endif

    // This array defines the set of DirectX hardware feature levels this app will support.
    // Note the ordering should be preserved.
    // Don't forget to declare your application's minimum required feature level in its
    // description.  All applications are assumed to support 9.1 unless otherwise stated.
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    // Create the Direct3D 11 API device object and a corresponding context.
    hr = D3D11CreateDevice(
        nullptr,                        // Specify nullptr to use the default adapter.
        D3D_DRIVER_TYPE_HARDWARE,       // Create a device using the hardware graphics driver.
        0,                              // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        creationFlags,                  // Set debug and Direct2D compatibility flags.
        featureLevels,                  // List of feature levels this app can support.
        ARRAYSIZE(featureLevels),       // Size of the list above.
        D3D11_SDK_VERSION,              // Always set this to D3D11_SDK_VERSION for Windows Store apps.
        &resources.d3d.device,          // Returns the Direct3D device created.
        &resources.d3d.featureLevel,    // Returns feature level of device created.
        &resources.d3d.context          // Returns the device immediate context.
    );

    if (FAILED(hr))
    {
        // If the initialization fails, fall back to the WARP device.
        ReturnIfFailed(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
            0,
            creationFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &resources.d3d.device,
            &resources.d3d.featureLevel,
            &resources.d3d.context
        ));
    }

    // Create the Direct2D device object and a corresponding context.
    ComPtr<IDXGIDevice3> dxgiDevice;
    ReturnIfFailed(resources.d3d.device.As(&dxgiDevice));

    ReturnIfFailed(m_deviceIndependentResources.d2d.factory->CreateDevice(
        dxgiDevice.Get(),
        &resources.d2d.device));

    ReturnIfFailed(resources.d2d.device->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        &resources.d2d.context));

    // Store resources if initialization succeeded.
    m_deviceDependentResources = resources;
}

void DeviceResources::InitializeWindowResources(winrt::Windows::UI::Core::CoreWindow const & window)
{
    WindowDependentResources resources = {};

    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    m_deviceDependentResources.d3d.context->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
    m_windowDependentResources.d3d.renderTargetView = nullptr;
    m_deviceDependentResources.d2d.context->SetTarget(nullptr);
    m_windowDependentResources.d2d.targetBitmap = nullptr;
    m_windowDependentResources.d3d.depthStencilView = nullptr;
    m_deviceDependentResources.d3d.context->Flush();

    // Calculate the size of the window.
    resources.d3d.renderTargetSize.Width = window.Bounds().Width;
    resources.d3d.renderTargetSize.Height = window.Bounds().Height;

    if (m_windowDependentResources.d3d.swapChain)
    {
        // If the swap chain already exists, resize it.
        resources.d3d.swapChain = m_windowDependentResources.d3d.swapChain;

        HRESULT hr = resources.d3d.swapChain->ResizeBuffers(
            2, // Double-buffered swap chain.
            lround(resources.d3d.renderTargetSize.Width),
            lround(resources.d3d.renderTargetSize.Height),
            DXGI_FORMAT_B8G8R8A8_UNORM,
            0
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            //HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            ReturnIfFailed(hr);
        }
    }
    else
    {
        // Otherwise, create a new one using the same adapter as the existing Direct3D device.
        DXGI_SCALING scaling = DXGI_SCALING_NONE;
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

        swapChainDesc.Width = lround(resources.d3d.renderTargetSize.Width);     // Match the size of the window.
        swapChainDesc.Height = lround(resources.d3d.renderTargetSize.Height);
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;                      // This is the most common swap chain format.
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;                                     // Don't use multi-sampling.
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;                                          // Use double-buffering to minimize latency.
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;            // All Windows Store apps must use this SwapEffect.
        swapChainDesc.Flags = 0;
        swapChainDesc.Scaling = scaling;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        // This sequence obtains the DXGI factory that was used to create the Direct3D device above.
        ComPtr<IDXGIDevice3> dxgiDevice;
        ReturnIfFailed(m_deviceDependentResources.d3d.device.As(&dxgiDevice));

        ComPtr<IDXGIAdapter> dxgiAdapter;
        ReturnIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));

        ComPtr<IDXGIFactory4> dxgiFactory;
        ReturnIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

        ComPtr<IUnknown> windowUnkown;
        window->QueryInterface(IID_PPV_ARGS(&windowUnkown));
        HRESULT hr = dxgiFactory->CreateSwapChainForCoreWindow(
            m_deviceDependentResources.d3d.device.Get(),
            windowUnkown.Get(),
            &swapChainDesc,
            nullptr,
            &resources.d3d.swapChain);

        // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
        // ensures that the application will only render after each VSync, minimizing power consumption.
        ReturnIfFailed(dxgiDevice->SetMaximumFrameLatency(1));
    }

    // Create a render target view of the swap chain back buffer.
    ComPtr<ID3D11Texture2D1> backBuffer;
    ReturnIfFailed(resources.d3d.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

    ReturnIfFailed(m_deviceDependentResources.d3d.device->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        &resources.d3d.renderTargetView));

    // Create a depth stencil view for use with 3D rendering if needed.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        lround(resources.d3d.renderTargetSize.Width),
        lround(resources.d3d.renderTargetSize.Height),
        1, // This depth stencil view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_DEPTH_STENCIL
    );

    ComPtr<ID3D11Texture2D> depthStencil;
    ReturnIfFailed(m_deviceDependentResources.d3d.device->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        &depthStencil));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    ReturnIfFailed(m_deviceDependentResources.d3d.device->CreateDepthStencilView(
        depthStencil.Get(),
        &depthStencilViewDesc,
        &resources.d3d.depthStencilView));

    // Set the 3D rendering viewport to target the entire window.
    resources.d3d.screenViewport = CD3D11_VIEWPORT(
        0.0f,
        0.0f,
        resources.d3d.renderTargetSize.Width,
        resources.d3d.renderTargetSize.Height
    );

    m_deviceDependentResources.d3d.context->RSSetViewports(1, &resources.d3d.screenViewport);

    // Create a Direct2D target bitmap associated with the
    // swap chain back buffer and set it as the current target.
    D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96,
            96
        );

    ComPtr<IDXGISurface2> dxgiBackBuffer;
    ReturnIfFailed(resources.d3d.swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)));

    ReturnIfFailed(m_deviceDependentResources.d2d.context->CreateBitmapFromDxgiSurface(
        dxgiBackBuffer.Get(),
        &bitmapProperties,
        &resources.d2d.targetBitmap));

    m_deviceDependentResources.d2d.context->SetTarget(resources.d2d.targetBitmap.Get());
    m_deviceDependentResources.d2d.context->SetDpi(96, 96);

    // Grayscale text anti-aliasing is recommended for all Windows Store apps.
    m_deviceDependentResources.d2d.context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    m_windowDependentResources = resources;
}

void DeviceResources::Present()
{
    Microsoft::WRL::ComPtr<ID3D11DeviceContext3> context;
    ReturnIfFailed(m_deviceDependentResources.d3d.context.As(&context));

    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    DXGI_PRESENT_PARAMETERS parameters = { 0 };
    HRESULT hr = m_windowDependentResources.d3d.swapChain->Present1(1, 0, &parameters);

    // Discard the contents of the render target.
    // This is a valid operation only when the existing contents will be entirely
    // overwritten. If dirty or scroll rects are used, this call should be removed.
    context->DiscardView1(m_windowDependentResources.d3d.renderTargetView.Get(), nullptr, 0);

    // Discard the contents of the depth stencil.
    context->DiscardView1(m_windowDependentResources.d3d.depthStencilView.Get(), nullptr, 0);

    // If the device was removed either by a disconnection or a driver upgrade, we 
    // must recreate all device resources.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        //HandleDeviceLost();
    }
    else
    {
        ReturnIfFailed(hr);
    }
}
