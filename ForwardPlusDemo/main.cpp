#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

#include <Application.h>
#include <ForwardPlusDemo.h>

#include <dxgidebug.h>

void ReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    int retCode = 0;

    // Set the working directory to the path of the executable.
    WCHAR path[MAX_PATH];
    HMODULE hModule = GetModuleHandleW(NULL);
    if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
    {
        PathRemoveFileSpecW(path);
        SetCurrentDirectoryW(path);
    }

    auto& app = dx12demo::core::Application::Create(hInstance);
    {
        std::shared_ptr<dx12demo::ForwardPlusDemo> demo = std::make_shared<dx12demo::ForwardPlusDemo>(L"DirectX 12 - ForwardPlusDemo", 1600, 1200);
        retCode = app.Run(demo);
    }
    dx12demo::core::Application::Destroy();

    return retCode;
}