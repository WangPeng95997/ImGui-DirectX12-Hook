#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include "GuiWindow.h"
#include "MinHook/include/MinHook.h"

typedef struct TagFrameContext
{
    ID3D12CommandAllocator* d3d12CommandAllocator;
    ID3D12Resource* mainRenderTargetResource;
    D3D12_CPU_DESCRIPTOR_HANDLE d3d12DescriptorHandle;
}FrameContext;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
typedef HRESULT(WINAPI* ExecuteCommandLists)(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
typedef HRESULT(WINAPI* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(WINAPI* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
HRESULT WINAPI HK_ExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
HRESULT WINAPI HK_Present(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT WINAPI HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
LRESULT WINAPI HK_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

ExecuteCommandLists Original_ExecuteCommandLists;
Present Original_Present;
ResizeBuffers Original_ResizeBuffers;
WNDPROC Original_WndProc;
HMODULE g_hInstance;
HANDLE g_hEndEvent;
LPVOID g_lpVirtualTable;
GuiWindow* g_GuiWindow;
FrameContext* g_pFrameContext;
ID3D12Device* g_pDX12Device;
ID3D12DescriptorHeap* g_pDX12DescHeapBackBuffers;
ID3D12DescriptorHeap* g_pDX12DescHeapImGuiRender;
ID3D12GraphicsCommandList* g_pDX12CommandList;
ID3D12CommandQueue* g_pDX12CommandQueue;
size_t g_nBufferCounts = 0;

void InitHook()
{
    ULONG_PTR* lpVTable = (ULONG_PTR*)g_lpVirtualTable;
    MH_Initialize();

    // ExecuteCommandLists
    LPVOID lpTarget = (LPVOID)lpVTable[54];
    MH_CreateHook(lpTarget, HK_ExecuteCommandLists, (void**)&Original_ExecuteCommandLists);
    MH_EnableHook(lpTarget);

    // Present
    lpTarget = (LPVOID)lpVTable[140];
    MH_CreateHook(lpTarget, HK_Present, (void**)&Original_Present);
    MH_EnableHook(lpTarget);

    // ResizeBuffers
    //lpTarget = (LPVOID)lpVTable[145];
    //MH_CreateHook(lpTarget, HK_ResizeBuffers, (void**)&Original_ResizeBuffers);
    //MH_EnableHook(lpTarget);
}

void ReleaseHook()
{
    ::SetWindowLongPtr(g_GuiWindow->hWnd, GWLP_WNDPROC, (LONG_PTR)Original_WndProc);
    MH_DisableHook(MH_ALL_HOOKS);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_pDX12Device->Release();
    g_pDX12DescHeapBackBuffers->Release();
    g_pDX12DescHeapImGuiRender->Release();
    g_pDX12CommandList->Release();
    g_pDX12CommandQueue->Release();

    delete[] g_pFrameContext;
    ::free(g_lpVirtualTable);
    g_lpVirtualTable = nullptr;

    ::SetEvent(g_hEndEvent);
}

LRESULT WINAPI HK_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_INSERT)
            g_GuiWindow->bShowMenu = !g_GuiWindow->bShowMenu;
        break;

    case WM_DESTROY:
        ReleaseHook();
        break;
    }

    if (g_GuiWindow->bShowMenu && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return ::CallWindowProc(Original_WndProc, hWnd, uMsg, wParam, lParam);
}

inline void InitImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    io.Fonts->AddFontFromFileTTF(g_GuiWindow->FontPath, 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGuiStyle& Style = ImGui::GetStyle();
    Style.ButtonTextAlign.y = 0.46f;
    Style.WindowBorderSize = 0.0f;
    Style.WindowRounding = 0.0f;
    Style.WindowPadding.x = 0.0f;
    Style.WindowPadding.y = 0.0f;
    Style.FrameRounding = 0.0f;
    Style.FrameBorderSize = 0.0f;
    Style.FramePadding.x = 0.0f;
    Style.FramePadding.y = 0.0f;
    Style.ChildRounding = 0.0f;
    Style.ChildBorderSize = 0.0f;
    Style.GrabRounding = 0.0f;
    Style.GrabMinSize = 8.0f;
    Style.PopupBorderSize = 0.0f;
    Style.PopupRounding = 0.0f;
    Style.ScrollbarRounding = 0.0f;
    Style.TabBorderSize = 0.0f;
    Style.TabRounding = 0.0f;
    Style.DisplaySafeAreaPadding.x = 0.0f;
    Style.DisplaySafeAreaPadding.y = 0.0f;
    Style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    Style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    Style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    Style.Colors[ImGuiCol_FrameBg] = ImColor(0, 74, 122, 100).Value;
    Style.Colors[ImGuiCol_FrameBgHovered] = ImColor(0, 74, 122, 175).Value;
    Style.Colors[ImGuiCol_FrameBgActive] = ImColor(0, 74, 122, 255).Value;
    Style.Colors[ImGuiCol_TitleBg] = ImColor(0, 74, 122, 255).Value;
    Style.Colors[ImGuiCol_TitleBgActive] = ImColor(0, 74, 122, 255).Value;

    ImGui_ImplWin32_Init(g_GuiWindow->hWnd);
    ImGui_ImplDX12_Init(g_pDX12Device, (int)g_nBufferCounts, DXGI_FORMAT_R8G8B8A8_UNORM,
        g_pDX12DescHeapImGuiRender, g_pDX12DescHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(), g_pDX12DescHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());
    ImGui_ImplDX12_CreateDeviceObjects();
    Original_WndProc = (WNDPROC)::SetWindowLongPtr(g_GuiWindow->hWnd, GWLP_WNDPROC, (LONG_PTR)HK_WndProc);
}

HRESULT WINAPI HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    // TODO
    HRESULT hResult = Original_ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    return hResult;
}

HRESULT WINAPI HK_Present(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags)
{
    static bool bImGuiInit = false;

    if (!bImGuiInit)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&g_pDX12Device)))
        {
            DXGI_SWAP_CHAIN_DESC swapChainDesc{};
            pSwapChain->GetDesc(&swapChainDesc);
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            swapChainDesc.OutputWindow = g_GuiWindow->hWnd;
            swapChainDesc.Windowed = !(GetWindowLongPtr(g_GuiWindow->hWnd, GWL_STYLE) & WS_POPUP) ? true : false;

            g_nBufferCounts = swapChainDesc.BufferCount;
            g_pFrameContext = new FrameContext[g_nBufferCounts];

            D3D12_DESCRIPTOR_HEAP_DESC descriptorImGuiRender{};
            descriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            descriptorImGuiRender.NumDescriptors = (UINT)g_nBufferCounts;
            descriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            ID3D12CommandAllocator* d3d12CommandAllocator;
            if (g_pDX12Device->CreateDescriptorHeap(&descriptorImGuiRender, IID_PPV_ARGS(&g_pDX12DescHeapImGuiRender)) != S_OK)
                return Original_Present(pSwapChain, SyncInterval, Flags);
            if (g_pDX12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d12CommandAllocator)) != S_OK)
                return Original_Present(pSwapChain, SyncInterval, Flags);
            for (size_t i = 0; i < g_nBufferCounts; i++)
                g_pFrameContext[i].d3d12CommandAllocator = d3d12CommandAllocator;

            if (g_pDX12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12CommandAllocator, NULL, IID_PPV_ARGS(&g_pDX12CommandList)) != S_OK || g_pDX12CommandList->Close() != S_OK)
                return Original_Present(pSwapChain, SyncInterval, Flags);

            D3D12_DESCRIPTOR_HEAP_DESC descriptorBackBuffers{};
            descriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            descriptorBackBuffers.NumDescriptors = (UINT)g_nBufferCounts;
            descriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            descriptorBackBuffers.NodeMask = 1;

            if (g_pDX12Device->CreateDescriptorHeap(&descriptorBackBuffers, IID_PPV_ARGS(&g_pDX12DescHeapBackBuffers)) != S_OK)
                return Original_Present(pSwapChain, SyncInterval, Flags);

            size_t rtvDescriptorSize = g_pDX12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pDX12DescHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();
            ID3D12Resource* pBackBuffer;
            for (size_t i = 0; i < g_nBufferCounts; i++)
            {
                g_pFrameContext[i].d3d12DescriptorHandle = rtvHandle;
                pSwapChain->GetBuffer((UINT)i, IID_PPV_ARGS(&pBackBuffer));
                g_pDX12Device->CreateRenderTargetView(pBackBuffer, nullptr, rtvHandle);
                g_pFrameContext[i].mainRenderTargetResource = pBackBuffer;
                rtvHandle.ptr += rtvDescriptorSize;
            }

            InitImGui();
            bImGuiInit = true;
        }
    }
    if (g_GuiWindow->UIStatus & GuiWindow::Detach)
    {
        ReleaseHook();
        return Original_Present(pSwapChain, SyncInterval, Flags);
    }
    if (g_pDX12CommandQueue == nullptr)
        return Original_Present(pSwapChain, SyncInterval, Flags);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();
    //g_GuiWindow->Update();

    ImGui::EndFrame();

    FrameContext& frameContext = g_pFrameContext[pSwapChain->GetCurrentBackBufferIndex()];
    frameContext.d3d12CommandAllocator->Reset();

    D3D12_RESOURCE_BARRIER d3d12Barrier;
    d3d12Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    d3d12Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    d3d12Barrier.Transition.pResource = frameContext.mainRenderTargetResource;
    d3d12Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    d3d12Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    d3d12Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    g_pDX12CommandList->Reset(frameContext.d3d12CommandAllocator, nullptr);
    g_pDX12CommandList->ResourceBarrier(1, &d3d12Barrier);
    g_pDX12CommandList->OMSetRenderTargets(1, &frameContext.d3d12DescriptorHandle, false, nullptr);
    g_pDX12CommandList->SetDescriptorHeaps(1, &g_pDX12DescHeapImGuiRender);

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pDX12CommandList);
    d3d12Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    d3d12Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    g_pDX12CommandList->ResourceBarrier(1, &d3d12Barrier);
    g_pDX12CommandList->Close();
    g_pDX12CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&g_pDX12CommandList));

    return Original_Present(pSwapChain, SyncInterval, Flags);
}

HRESULT WINAPI HK_ExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
{
    if (!g_pDX12CommandQueue)
        g_pDX12CommandQueue = pCommandQueue;

    return Original_ExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
}

DWORD WINAPI Start(LPVOID lpParameter)
{
    g_hInstance = (HMODULE)lpParameter;
    g_hEndEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    g_GuiWindow = new GuiWindow();
    g_GuiWindow->Init();

    WNDCLASSEX windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = ::DefWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = ::GetModuleHandle(NULL);
    windowClass.hIcon = NULL;
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = "DirectX12";
    windowClass.hIconSm = NULL;

    ::RegisterClassEx(&windowClass);
    HWND hWnd = ::CreateWindow(
        windowClass.lpszClassName,
        "DirectX12Window",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        100,
        100,
        NULL,
        NULL,
        windowClass.hInstance,
        NULL);

    IDXGIFactory* pDXGIFactory;
    IDXGIAdapter* pDXGIAdapter;
    ID3D12Device* pD3D12Device;

    LPVOID CreateDXGIFactory = ::GetProcAddress(::GetModuleHandle("dxgi.dll"), "CreateDXGIFactory");
    ((DWORD(WINAPI*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&pDXGIFactory);
    pDXGIFactory->EnumAdapters(0, &pDXGIAdapter);
    LPVOID D3D12CreateDevice = ::GetProcAddress(::GetModuleHandle("d3d12.dll"), "D3D12CreateDevice");
    ((DWORD(WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(D3D12CreateDevice))(pDXGIAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&pD3D12Device);

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = 0;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    ID3D12CommandQueue* pD3D12CommandQueue;
    ID3D12CommandAllocator* pD3D12CommandAllocator;
    ID3D12GraphicsCommandList* pD3D12CommandList;
    IDXGISwapChain* pSwapChain;

    pD3D12Device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&pD3D12CommandQueue);
    pD3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&pD3D12CommandAllocator);
    pD3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pD3D12CommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&pD3D12CommandList);

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Width = 100;
    swapChainDesc.BufferDesc.Height = 100;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Windowed = 1;

    pDXGIFactory->CreateSwapChain(pD3D12CommandQueue, &swapChainDesc, &pSwapChain);

    g_lpVirtualTable = ::calloc(150, sizeof(ULONG_PTR));
    if (g_lpVirtualTable)
    {
        ::memcpy(g_lpVirtualTable, *(ULONG_PTR**)pD3D12Device, 44 * sizeof(ULONG_PTR));
        ::memcpy((ULONG_PTR*)g_lpVirtualTable + 44, *(ULONG_PTR**)pD3D12CommandQueue, 19 * sizeof(ULONG_PTR));
        ::memcpy((ULONG_PTR*)g_lpVirtualTable + 44 + 19, *(ULONG_PTR**)pD3D12CommandAllocator, 9 * sizeof(ULONG_PTR));
        ::memcpy((ULONG_PTR*)g_lpVirtualTable + 44 + 19 + 9, *(ULONG_PTR**)pD3D12CommandList, 60 * sizeof(ULONG_PTR));
        ::memcpy((ULONG_PTR*)g_lpVirtualTable + 44 + 19 + 9 + 60, *(ULONG_PTR**)pSwapChain, 18 * sizeof(ULONG_PTR));

        pD3D12Device->Release();
        pD3D12CommandQueue->Release();
        pD3D12CommandAllocator->Release();
        pD3D12CommandList->Release();
        pSwapChain->Release();

        InitHook();
    }
    ::DestroyWindow(hWnd);
    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

    if (g_hEndEvent)
        ::WaitForSingleObject(g_hEndEvent, INFINITE);
    ::FreeLibraryAndExitThread(g_hInstance, EXIT_SUCCESS);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (::GetModuleHandleA("d3d12.dll") == NULL)
            return false;

        ::DisableThreadLibraryCalls(hModule);
        ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Start, hModule, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        ::Sleep(100);
        MH_Uninitialize();
        break;
    }

    return true;
}