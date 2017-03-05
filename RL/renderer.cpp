#include "pch.h"

#include "deviceresources.h"
#include "renderer.h"

void Renderer::Render(DeviceResources & resources)
{
    auto context3d = resources.m_deviceDependentResources.d3d.context;
    auto context2d = resources.m_deviceDependentResources.d2d.context;

    // Reset the viewport to target the whole screen.
    auto viewport = resources.m_windowDependentResources.d3d.screenViewport;
    context3d->RSSetViewports(1, &viewport);

    // Reset render targets to the screen.
    ID3D11RenderTargetView *const targets[1] = { resources.m_windowDependentResources.d3d.renderTargetView.Get() };
    context3d->OMSetRenderTargets(1, targets, resources.m_windowDependentResources.d3d.depthStencilView.Get());

    // Clear the back buffer and depth stencil view.
    context3d->ClearRenderTargetView(resources.m_windowDependentResources.d3d.renderTargetView.Get(), DirectX::Colors::Black);
    context3d->ClearDepthStencilView(resources.m_windowDependentResources.d3d.depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    context2d->BeginDraw();

    context2d->SetTransform(D2D1::Matrix3x2F::Identity());
    context2d->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    D2D1_SIZE_F size = context2d->GetSize();

    if (!m_brush)
    {
        ReturnIfFailed(context2d->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::White, 1.0f),
            &m_brush));
    }

    const auto tileSize = 40.0f;
    for (auto x = 0; x < size.width / tileSize; x++) {
        for (auto y = (x % 2) ? 0 : 1; y < size.height / tileSize; y += 2) {
            context2d->FillRectangle(
                D2D1::RectF(x * tileSize, y * tileSize, (x + 1) * tileSize, (y + 1) * tileSize),
                m_brush.Get());
        }
    }

    context2d->EndDraw();
}
