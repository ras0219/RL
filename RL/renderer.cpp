#include "pch.h"

#include "deviceresources.h"
#include "renderer.h"

void Renderer::Render(DeviceResources & resources)
{
    auto context = resources.m_deviceDependentResources.d3d.context;

    // Reset the viewport to target the whole screen.
    auto viewport = resources.m_windowDependentResources.d3d.screenViewport;
    context->RSSetViewports(1, &viewport);

    // Reset render targets to the screen.
    ID3D11RenderTargetView *const targets[1] = { resources.m_windowDependentResources.d3d.renderTargetView.Get() };
    context->OMSetRenderTargets(1, targets, resources.m_windowDependentResources.d3d.depthStencilView.Get());

    // Clear the back buffer and depth stencil view.
    context->ClearRenderTargetView(resources.m_windowDependentResources.d3d.renderTargetView.Get(), DirectX::Colors::Black);
    context->ClearDepthStencilView(resources.m_windowDependentResources.d3d.depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
