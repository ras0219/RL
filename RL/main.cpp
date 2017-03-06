#include "pch.h"

#include "deviceresources.h"
#include "renderer.h"

using namespace winrt;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Core;

struct AppView : implements<AppView, IFrameworkView>
{
    void Initialize(CoreApplicationView const & view)
    {
        m_renderer.InitializeDeviceDependentResources(m_deviceResources);
    }

    void Load(hstring)
    {
        //
    }

    void Uninitialize()
    {
        //
    }

    void Run()
    {
        CoreWindow window = CoreWindow::GetForCurrentThread();
        window.Activate();

        CoreDispatcher dispatcher = window.Dispatcher();
        while (!m_state.closed)
        {
            if (m_state.visible)
            {
                dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

                if (m_state.activated)
                {
                    m_renderer.Render(m_deviceResources);
                    m_deviceResources.Present();
                }
            }
            else
            {
                dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }
    }

    void SetWindow(CoreWindow const & window)
    {
        m_activated = window.Activated(auto_revoke, { this, &AppView::OnActivated });
    }

    fire_and_forget OnActivated(CoreWindow window, WindowActivatedEventArgs)
    {
        m_activated.revoke();

        window.SizeChanged([=](auto &&, WindowSizeChangedEventArgs const & args)
        {
            m_deviceResources.InitializeWindowResources(window);
        });

        window.VisibilityChanged([=](auto &&, VisibilityChangedEventArgs const & args)
        {
            m_state.visible = args.Visible();
        });

        window.Closed([=](auto &&, CoreWindowEventArgs const & args)
        {
            m_state.closed = true;
        });

        m_deviceResources.InitializeWindowResources(window);
        m_state.activated = true;

        return{};
    }

    CoreWindow::Activated_revoker m_activated;

    DeviceResources m_deviceResources;
    Renderer m_renderer;

    struct {
        bool activated = false;
        bool closed = false;
        bool visible = true;
    } m_state;
};

struct AppViewSource : implements<AppViewSource, IFrameworkViewSource>
{
    IFrameworkView CreateView()
    {
        return m_mainView;
    }

    AppView m_mainView;
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    CoreApplication::Run(AppViewSource());
}
