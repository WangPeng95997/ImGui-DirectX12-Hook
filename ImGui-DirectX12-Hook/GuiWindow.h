#pragma once
#include <Windows.h>
#include <sstream>
#include <string>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Imgui/imgui_impl_dx12.h"
#include "Imgui/imgui_impl_win32.h"

#define AUTHOR              "by l4kkS41"	

#define BUILD_DATE []{ \
    const char* date = __DATE__; \
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec"; \
    char month[4] = {date[0], date[1], date[2], '\0'}; \
    size_t monthIndex = (strstr(months, month) - months) / 3 + 1; \
    std::ostringstream oss; \
    oss << date + 7 << "." << (monthIndex < 10 ? "0" : "") << monthIndex << "." << (date[4] == ' ' ? '0' : date[4]) << date[5]; \
    static std::string buildDate = oss.str(); \
    return buildDate.c_str(); \
}()

#define WINDOWNAME          "ImGui Window"
#define MAJORVERSION        1
#define MINORVERSION        0
#define REVISIONVERSION     0

#define WIDTH               600
#define HEIGHT              400

#define MODULENAME          NULL

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