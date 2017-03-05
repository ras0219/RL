#pragma once

struct Renderer
{
    void Render(DeviceResources & resources);

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
};
