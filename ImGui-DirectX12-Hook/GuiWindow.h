#pragma once
#include <Windows.h>
#include <string>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Imgui/imgui_impl_dx12.h"
#include "Imgui/imgui_impl_win32.h"

#define AUTHORINFO          "Build.20xx.xx.xx\nby l4kkS41"	

#define WINDOWNAME          "ImGui Window"
#define MAJORVERSION        1
#define MINORVERSION        0
#define REVISIONVERSION     0

#define TARGETCLASS         "gfx_test"
#define TARGETWINDOW        "Renderer: [DirectX12], Input: [Window Messages], 64 bits"
#define TARGETMODULE        "GFXTest64.exe"

#define WIDTH               600
#define HEIGHT              400

typedef unsigned __int64 QWORD;

class GuiWindow
{
public:
    enum GuiStatus : DWORD
    {
        Reset = 1 << 0,
        Exit = 1 << 1,
        Detach = 1 << 2
    };

    HWND        hWnd;
    HMODULE     hModule;
    HANDLE      hProcess;
    PCHAR       FontPath;
    PCHAR       WindowName;
    LPBYTE      ModuleAddress;
    LPBYTE      lpBuffer;
    ImVec2      StartPostion;
    DWORD       UIStatus;
    bool        bCrosshair;
    bool        bShowMenu;

    GuiWindow();
    ~GuiWindow();

    void Init();
    void Update();

    void Button_Exit();

    void Toggle_Crosshair(const bool& isEnable);
};