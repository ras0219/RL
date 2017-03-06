#pragma once

struct Renderer
{
    void Render(DeviceResources & resources);

    struct DeviceDependentResources
    {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> whiteBrush;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
    } m_deviceDependentResources;

    void InitializeDeviceDependentResources(DeviceResources & deviceResources);
};
