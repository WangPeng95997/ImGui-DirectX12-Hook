#pragma once
#include "ImGui/imgui.h"
#include "Imgui/imgui_impl_dx12.h"
#include "Imgui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"
#include <Windows.h>
#include <string>

constexpr const char* AUTHOR_INFO = "Created by l4kkS41 On ";
constexpr const char* WINDOW_NAME = "Dear ImGui ";
constexpr const char* MODULE_NAME = nullptr;
constexpr DWORD MAJOR_VERSION = 1;
constexpr DWORD MINOR_VERSION = 0;
constexpr DWORD REVISION_VERSION = 0;
constexpr float FONT_SIZE = 20.0f;
constexpr float WINDOW_WIDTH = 600.0f;
constexpr float WINDOW_HEIGHT = 400.0f;
constexpr ImGuiID CLOSE_BUTTON_ID = 0x1000;
constexpr ImGuiID CHILD_FRAME_ID = 0x2000;

class GuiWindow
{
public:
    enum class GuiState : DWORD
    {
        GuiState_None = 0,
        GuiState_Reset = 1,
        GuiState_Exiting = 2,
        GuiState_Detach = 4
    };

    HWND hWnd;
    HMODULE hModule;
    HANDLE hProcess;
    std::string fontPath;
    std::string windowTitle;
    LPBYTE lpBuffer;
    LPBYTE lpModuleAddress;
    ImVec2 initialPosition;
    DWORD uiStatus;
    bool showMenu;

    GuiWindow();
    ~GuiWindow();

    void Initialize();
    void Update();

    void ExitButton();
};