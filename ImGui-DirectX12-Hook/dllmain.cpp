#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include "mainwindow.h"
#include "MinHook.h"
#pragma comment(lib, "d3d12.lib")

typedef struct TagFrameContext
{
    ID3D12CommandAllocator* d3d12CommandAllocator;
    ID3D12Resource* mainRenderTargetResource;
    D3D12_CPU_DESCRIPTOR_HANDLE d3d12DescriptorHandle;
}FrameContext, * LPFrameContext;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef HRESULT(WINAPI* ExecuteCommandLists)(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
typedef HRESULT(WINAPI* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(WINAPI* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
HRESULT WINAPI HK_ExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
HRESULT WINAPI HK_Present(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT WINAPI HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
LRESULT WINAPI HK_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

ExecuteCommandLists Original_ExecuteCommandLists;
Present Original_Present;
ResizeBuffers Original_ResizeBuffers;
WNDPROC Original_WndProc;
HWND g_mainWindow;
HMODULE g_hHinstance;
HANDLE g_hEndEvent;
DWORD64* g_methodsTable;
GuiWindow* g_GuiWindow;
FrameContext* g_frameContext;
ID3D12Device* g_dx12Device;
ID3D12DescriptorHeap* g_dx12DescHeapBackBuffers;
ID3D12DescriptorHeap* g_dx12DescHeapImGuiRender;
ID3D12GraphicsCommandList* g_dx12CommandList;
ID3D12CommandQueue* g_dx12CommandQueue;
int g_bufferCounts = 0;
bool g_ImGuiInit = false;

void InitHook()
{
    MH_Initialize();

    // ExecuteCommandLists
    int index = 54;
    void* pTarget = (void*)g_methodsTable[index];
    MH_CreateHook(pTarget, HK_ExecuteCommandLists, (void**)&Original_ExecuteCommandLists);
    MH_EnableHook(pTarget);

    // Present
    index = 140;
    pTarget = (void*)g_methodsTable[index];
    MH_CreateHook(pTarget, HK_Present, (void**)&Original_Present);
    MH_EnableHook(pTarget);

    // ResizeBuffers
    //index = 145;
    //pTarget = (void*)g_methodsTable[index];
    //MH_CreateHook(pTarget, HK_ResizeBuffers, (void**)&Original_ResizeBuffers);
    //MH_EnableHook(pTarget);
}

void ReleaseHook()
{
    MH_DisableHook(MH_ALL_HOOKS);
    SetWindowLongPtr(g_mainWindow, GWLP_WNDPROC, (LONG_PTR)Original_WndProc);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_dx12Device->Release();
    g_dx12DescHeapBackBuffers->Release();
    g_dx12DescHeapImGuiRender->Release();
    g_dx12CommandList->Release();
    g_dx12CommandQueue->Release();
    delete g_frameContext;
    ::free(g_methodsTable);
    ::SetEvent(g_hEndEvent);
}

LRESULT WINAPI HK_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_M)
            g_GuiWindow->showMenu = !g_GuiWindow->showMenu;
        break;

    case WM_DESTROY:
        ReleaseHook();
        break;
    }

    if (g_GuiWindow->showMenu && ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(Original_WndProc, hwnd, uMsg, wParam, lParam);
}


inline void InitImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    io.Fonts->AddFontFromFileTTF(g_GuiWindow->fontPath, 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
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

    ImGui_ImplWin32_Init(g_mainWindow);
    ImGui_ImplDX12_Init(g_dx12Device, g_bufferCounts, DXGI_FORMAT_R8G8B8A8_UNORM,
        g_dx12DescHeapImGuiRender, g_dx12DescHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(), g_dx12DescHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());
    ImGui_ImplDX12_CreateDeviceObjects();
    Original_WndProc = (WNDPROC)SetWindowLongPtr(g_mainWindow, GWLP_WNDPROC, (LONG_PTR)HK_WndProc);

    g_ImGuiInit = true;
}

// TODO
HRESULT WINAPI HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    HRESULT hResult = Original_ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    D3D12_VIEWPORT viewPort{};
    viewPort.Width = (float)Width;
    viewPort.Height = (float)Height;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0.0f;
    viewPort.TopLeftY = 0.0f;
    g_dx12CommandList->RSSetViewports(1, &viewPort);

    return hResult;
}

HRESULT WINAPI HK_Present(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!g_ImGuiInit)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&g_dx12Device)))
        {
            DXGI_SWAP_CHAIN_DESC swapChainDesc{};
            pSwapChain->GetDesc(&swapChainDesc);
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            swapChainDesc.OutputWindow = g_mainWindow;
            swapChainDesc.Windowed = !(GetWindowLongPtr(g_mainWindow, GWL_STYLE) & WS_POPUP) ? true : false;

            g_bufferCounts = swapChainDesc.BufferCount;
            g_frameContext = new FrameContext[g_bufferCounts];

            D3D12_DESCRIPTOR_HEAP_DESC descriptorImGuiRender{};
            descriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            descriptorImGuiRender.NumDescriptors = g_bufferCounts;
            descriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            ID3D12CommandAllocator* d3d12CommandAllocator;
            if (g_dx12Device->CreateDescriptorHeap(&descriptorImGuiRender, IID_PPV_ARGS(&g_dx12DescHeapImGuiRender)) != S_OK)
                return Original_Present(pSwapChain, SyncInterval, Flags);
            if (g_dx12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d12CommandAllocator)) != S_OK)
                return Original_Present(pSwapChain, SyncInterval, Flags);
            for (int i = 0; i < g_bufferCounts; i++)
                g_frameContext[i].d3d12CommandAllocator = d3d12CommandAllocator;

            if (g_dx12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12CommandAllocator, NULL, IID_PPV_ARGS(&g_dx12CommandList)) != S_OK || g_dx12CommandList->Close() != S_OK)
                return Original_Present(pSwapChain, SyncInterval, Flags);

            D3D12_DESCRIPTOR_HEAP_DESC descriptorBackBuffers{};
            descriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            descriptorBackBuffers.NumDescriptors = g_bufferCounts;
            descriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            descriptorBackBuffers.NodeMask = 1;

            if (g_dx12Device->CreateDescriptorHeap(&descriptorBackBuffers, IID_PPV_ARGS(&g_dx12DescHeapBackBuffers)) != S_OK)
                return Original_Present(pSwapChain, SyncInterval, Flags);

            size_t rtvDescriptorSize = g_dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_dx12DescHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();
            ID3D12Resource* pBackBuffer;
            for (int i = 0; i < g_bufferCounts; i++)
            {
                g_frameContext[i].d3d12DescriptorHandle = rtvHandle;
                pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
                g_dx12Device->CreateRenderTargetView(pBackBuffer, nullptr, rtvHandle);
                g_frameContext[i].mainRenderTargetResource = pBackBuffer;
                rtvHandle.ptr += rtvDescriptorSize;
            }

            InitImGui();
        }
    }
    if (g_GuiWindow->windowStatus & WindowStatus::Exit)
    {
        ReleaseHook();
        return Original_Present(pSwapChain, SyncInterval, Flags);
    }
    if (g_dx12CommandQueue == nullptr)
        return Original_Present(pSwapChain, SyncInterval, Flags);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();
    //g_GuiWindow->Update();

    ImGui::EndFrame();

    FrameContext& frameContext = g_frameContext[pSwapChain->GetCurrentBackBufferIndex()];
    frameContext.d3d12CommandAllocator->Reset();

    D3D12_RESOURCE_BARRIER d3d12barrier;
    d3d12barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    d3d12barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    d3d12barrier.Transition.pResource = frameContext.mainRenderTargetResource;
    d3d12barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    d3d12barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    d3d12barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    g_dx12CommandList->Reset(frameContext.d3d12CommandAllocator, nullptr);
    g_dx12CommandList->ResourceBarrier(1, &d3d12barrier);
    g_dx12CommandList->OMSetRenderTargets(1, &frameContext.d3d12DescriptorHandle, false, nullptr);
    g_dx12CommandList->SetDescriptorHeaps(1, &g_dx12DescHeapImGuiRender);

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_dx12CommandList);
    d3d12barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    d3d12barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    g_dx12CommandList->ResourceBarrier(1, &d3d12barrier);
    g_dx12CommandList->Close();
    g_dx12CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&g_dx12CommandList));

    return Original_Present(pSwapChain, SyncInterval, Flags);
}

HRESULT WINAPI HK_ExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
{
    if (!g_dx12CommandQueue)
        g_dx12CommandQueue = pCommandQueue;

    return Original_ExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
}

DWORD WINAPI Start(LPVOID lpParameter)
{
    g_hHinstance = HMODULE(lpParameter);
    g_GuiWindow = new GuiWindow();
    g_GuiWindow->Init();
    g_mainWindow = g_GuiWindow->hwnd;
    g_hEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    WNDCLASSEX windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hIcon = NULL;
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = "DirectX12";
    windowClass.hIconSm = NULL;

    RegisterClassEx(&windowClass);
    HWND hwnd = CreateWindow(windowClass.lpszClassName, "DirectX12Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

    HMODULE d3d12Module = ::GetModuleHandleA("d3d12.dll");
    HMODULE dxgiModule = ::GetModuleHandleA("dxgi.dll");
    if (d3d12Module == NULL || dxgiModule == NULL)
        return 0xF;

    IDXGIFactory* dxgiFactory;
    IDXGIAdapter* dxgiAdapter;
    ID3D12Device* d3d12Device;

    LPVOID CreateDXGIFactory = ::GetProcAddress(dxgiModule, "CreateDXGIFactory");
    ((long(__fastcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
    dxgiFactory->EnumAdapters(0, &dxgiAdapter);
    LPVOID D3D12CreateDevice = ::GetProcAddress(d3d12Module, "D3D12CreateDevice");
    ((long(__fastcall*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(D3D12CreateDevice))(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&d3d12Device);

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = 0;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    ID3D12CommandQueue* d3d12CommandQueue;
    ID3D12CommandAllocator* d3d12CommandAllocator;
    ID3D12GraphicsCommandList* d3d12CommandList;
    IDXGISwapChain* swapChain;

    d3d12Device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&d3d12CommandQueue);
    d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&d3d12CommandAllocator);
    d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12CommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&d3d12CommandList);

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
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Windowed = 1;

    dxgiFactory->CreateSwapChain(d3d12CommandQueue, &swapChainDesc, &swapChain);

    g_methodsTable = (DWORD64*)::calloc(150, sizeof(DWORD64));
    ::memcpy(g_methodsTable, *(DWORD64**)d3d12Device, 44 * sizeof(DWORD64));
    ::memcpy(g_methodsTable + 44, *(DWORD64**)d3d12CommandQueue, 19 * sizeof(DWORD64));
    ::memcpy(g_methodsTable + 44 + 19, *(DWORD64**)d3d12CommandAllocator, 9 * sizeof(DWORD64));
    ::memcpy(g_methodsTable + 44 + 19 + 9, *(DWORD64**)d3d12CommandList, 60 * sizeof(DWORD64));
    ::memcpy(g_methodsTable + 44 + 19 + 9 + 60, *(DWORD64**)swapChain, 18 * sizeof(DWORD64));

    InitHook();

    d3d12Device->Release();
    d3d12CommandQueue->Release();
    d3d12CommandAllocator->Release();
    d3d12CommandList->Release();
    swapChain->Release();

    ::DestroyWindow(hwnd);
    UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

    ::WaitForSingleObject(g_hEndEvent, INFINITE);
    ::FreeLibraryAndExitThread(g_hHinstance, 0);

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
        ::Sleep(1000);
        MH_Uninitialize();
        break;
    }

    return true;
}