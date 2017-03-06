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

    const std::string map[] = {
        "XXXXXXXXXXXXXXXXXXXXX",
        "X                   X",
        "X                   X",
        "X                   X",
        "X       HELLO       X",
        "X       WORLD       X",
        "X                   X",
        "X                   X",
        "X                   X",
        "XXXXXXXXXXXXXXXXXXXXX",
    };
    const D2D1_SIZE_U tileSize = D2D1::SizeU(14, 22);

    D2D1_SIZE_U tileSpan = D2D1::SizeU(
        (int)ceil(size.width / tileSize.width),
        (int)ceil(size.height / tileSize.height));

    D2D1_SIZE_F consolePadding = D2D1::SizeF(
        floor((size.width - tileSpan.width * tileSize.width) / 2.0f),
        floor((size.height - tileSpan.height * tileSize.height) / 2.0f));

    D2D1_SIZE_U mapSize = D2D1::SizeU(
        map[0].length(),
        ARRAYSIZE(map));

    D2D1_SIZE_U mapOffset = D2D1::SizeU(
        (tileSpan.width - mapSize.width) / 2,
        (tileSpan.height - mapSize.height) / 2);

    for (auto x = 0; x < tileSpan.width; x++) {
        for (auto y = 0; y < tileSpan.height; y++) {

            D2D1_POINT_2U mapTile = D2D1::Point2U(
                x - mapOffset.width,
                y - mapOffset.height);

            if (mapTile.x < 0 ||
                mapTile.y < 0 ||
                mapTile.x >= mapSize.width ||
                mapTile.y >= mapSize.height)
            {
                continue;
            }

            D2D1_RECT_F tileRect = D2D1::RectF(
                x * tileSize.width + consolePadding.width,
                y * tileSize.height + consolePadding.height,
                (x + 1) * tileSize.width + consolePadding.width,
                (y + 1) * tileSize.height + consolePadding.height);

            /*
            context2d->FillRectangle(
                tileRect,
                m_deviceDependentResources.grayBrush.Get());
             */

            WCHAR ch = map[mapTile.y][mapTile.x];
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
