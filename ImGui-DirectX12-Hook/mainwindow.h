#pragma once
#include <Windows.h>
#include <string>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Imgui/imgui_impl_dx12.h"
#include "Imgui/imgui_impl_win32.h"

#define AUTHORINFO          "Build.20xx.xx.xx\nby l4kkS41"	

#define WINDOWNAME          "ImGui Window"
#define MAJORVERSION        0
#define MINORVERSION        1
#define REVISIONVERSION     0

#define TARGETCLASS         "gfx_test"
#define TARGETWINDOW        "Renderer: [DirectX12], Input: [Window Messages], 64 bits"
#define TARGETMODULE        "GFXTest64.exe"

#define WIDTH               600
#define HEIGHT              400

#define VK_M                0x4D

enum WindowStatus : DWORD
{
    Normal      = 0,
    Repaint     = 1 << 0,
    Exit        = 1 << 1
};

class GuiWindow
{
public:
    char*       fontPath;
    char*       windowName;
    HWND        hwnd;
    HANDLE      hProcess;
    HMODULE     hModule;
    LPBYTE      baseAddress;
    LPBYTE      lpBuffer;
    DWORD       windowStatus;
    ImVec2      startPostion;
    bool        showMenu;
    bool        crossHair;

    GuiWindow();
    ~GuiWindow();

    void Init();
    void Repaint();
    void Update();

    void Button_Exit();
    void Toggle_CrossHair(const bool& isEnable);
};