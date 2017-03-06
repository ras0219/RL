#include "pch.h"

#include "deviceresources.h"
#include "renderer.h"

using namespace Microsoft::WRL;

void ConsoleRenderer::InitializeDeviceDependentResources(DeviceResources & deviceResources)
{
    DeviceDependentResources resources = {};

    auto context2d = deviceResources.m_deviceDependentResources.d2d.context;

    ReturnIfFailed(context2d->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White, 1.0f),
        &resources.whiteBrush));

    ReturnIfFailed(context2d->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Gray, 1.0f),
        &resources.grayBrush));

    // Create device independent resources
    ComPtr<IDWriteTextFormat> textFormat;
    ReturnIfFailed(deviceResources.m_deviceIndependentResources.dwrite.factory->CreateTextFormat(
        L"Consolas",
        nullptr,
        DWRITE_FONT_WEIGHT_LIGHT,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        18.0f,
        L"en-US",
        &resources.textFormat));
    ReturnIfFailed(resources.textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
    ReturnIfFailed(resources.textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));

    m_deviceDependentResources = resources;
}

void ConsoleRenderer::Render(DeviceResources & deviceResources)
{
    auto context3d = deviceResources.m_deviceDependentResources.d3d.context;
    auto context2d = deviceResources.m_deviceDependentResources.d2d.context;

    // Reset the viewport to target the whole screen.
    auto viewport = deviceResources.m_windowDependentResources.d3d.screenViewport;
    context3d->RSSetViewports(1, &viewport);

    // Reset render targets to the screen.
    ID3D11RenderTargetView *const targets[1] = { deviceResources.m_windowDependentResources.d3d.renderTargetView.Get() };
    context3d->OMSetRenderTargets(1, targets, deviceResources.m_windowDependentResources.d3d.depthStencilView.Get());

    // Clear the back buffer and depth stencil view.
    context3d->ClearRenderTargetView(deviceResources.m_windowDependentResources.d3d.renderTargetView.Get(), DirectX::Colors::Black);
    context3d->ClearDepthStencilView(deviceResources.m_windowDependentResources.d3d.depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    context2d->BeginDraw();

    context2d->SetTransform(D2D1::Matrix3x2F::Identity());
    context2d->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    D2D1_SIZE_F size = context2d->GetSize();

    const std::string text = "ABCDEFGHIJKLMNOPQURSTUVWXYZ";
    const D2D1_SIZE_U tileSize = D2D1::SizeU(14.0f, 22.0f);

    for (auto x = 0; x < size.width / tileSize.width; x++) {
        for (auto y = 0; y < size.height / tileSize.height; y++) {
            D2D1_RECT_F tileRect = D2D1::RectF(
                x * tileSize.width,
                y * tileSize.height,
                (x + 1) * tileSize.width,
                (y + 1) * tileSize.height);

            if ((x + y) % 2)
            {
                context2d->FillRectangle(
                    tileRect,
                    m_deviceDependentResources.grayBrush.Get());
            }

            WCHAR ch = (WCHAR)text[(x * (int)size.height / tileSize.width + y) % text.length()];
            context2d->DrawText(
                &ch,
                1,
                m_deviceDependentResources.textFormat.Get(),
                tileRect,
                m_deviceDependentResources.whiteBrush.Get());
        }
    }

    context2d->EndDraw();
}
