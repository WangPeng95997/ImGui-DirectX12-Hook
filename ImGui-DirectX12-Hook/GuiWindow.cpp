#include "GuiWindow.h"

GuiWindow::GuiWindow()
{
    // Initialize settings
    this->hWnd = nullptr;
    this->hModule = nullptr;
    this->hProcess = nullptr;
    this->lpModuleAddress = nullptr;
    this->initialPosition = ImVec2(0.0f, 0.0f);
    this->uiStatus = static_cast<DWORD>(GuiState::GuiState_Reset);
    this->showMenu = true;

    // Set font path
    LPSTR lpBuffer = new char[MAX_PATH] {};
    ::GetEnvironmentVariable("WINDIR", lpBuffer, MAX_PATH);
    ::strcat_s(lpBuffer, MAX_PATH, "\\Fonts\\segoeui.ttf");
    this->fontPath = lpBuffer;
    delete[] lpBuffer;

    // Set window title
    this->windowTitle = this->windowTitle.append(WINDOW_NAME).append("v") +
        std::to_string(MAJOR_VERSION) + "." +
        std::to_string(MINOR_VERSION) + "." +
        std::to_string(REVISION_VERSION);

    // Allocate memory
    this->lpBuffer = (LPBYTE)::VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (this->lpBuffer)
        ::memset(this->lpBuffer, 0xCC, 0x1000);
}

GuiWindow::~GuiWindow()
{
    ::VirtualFree(this->lpBuffer, 0, MEM_RELEASE);
}

static bool CALLBACK EnumHwndCallback(HWND hWnd, LPARAM lParam)
{
    const auto isMainWindow = [hWnd]() {
        return GetWindow(hWnd, GW_OWNER) == nullptr && IsWindowVisible(hWnd);
        };

    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);

    if (GetCurrentProcessId() != dwProcessId || !isMainWindow() || hWnd == GetConsoleWindow())
        return true;

    *(HWND*)lParam = hWnd;

    return false;
}

void GuiWindow::Initialize()
{
    do
    {
        ::EnumWindows((WNDENUMPROC)EnumHwndCallback, (LPARAM)&this->hWnd);
        Sleep(200);
    } while (!this->hWnd);

    this->hProcess = ::GetCurrentProcess();
    this->hModule = ::GetModuleHandle(MODULE_NAME);
    this->lpModuleAddress = (LPBYTE)this->hModule;
}

void GuiWindow::Update()
{
    // Set window flags to disable title bar, resizing, scrollbars, mouse wheel scrolling, and saved settings
    const ImGuiWindowFlags windowflags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin(WINDOW_NAME, nullptr, windowflags);

    // Check if the window state needs to be reset
    if (this->uiStatus & static_cast<DWORD>(GuiState::GuiState_Reset))
    {
        ImGui::SetWindowPos(this->initialPosition);
        ImGui::SetWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));
        this->uiStatus &= ~static_cast<DWORD>(GuiState::GuiState_Reset);
    }

    // Get window padding and position
    ImVec2& windowPadding = ImGui::GetStyle().WindowPadding;
    ImVec2 windowPosition = ImGui::GetWindowPos();

    // Display CloseButton in the top-right corner
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::CloseButton(CLOSE_BUTTON_ID, ImVec2(windowPosition.x + WINDOW_WIDTH - FONT_SIZE, windowPosition.y)))
        this->uiStatus |= static_cast<DWORD>(GuiState::GuiState_Exiting);
    ImGui::PopStyleVar();

    // Display shortcut hint in the bottom-right corner
    const std::string hotKey = std::string("Press INSERT to Toggle Menu");
    ImVec2 textSize = ImGui::CalcTextSize(hotKey.c_str());
    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH - windowPadding.x - textSize.x, WINDOW_HEIGHT - windowPadding.y - textSize.y));
    ImGui::Text(hotKey.c_str());

    // Display author information and build date in the bottom-left corner
    const std::string authorInfo = std::string(AUTHOR_INFO) + __DATE__;
    ImGui::SetCursorPosY(WINDOW_HEIGHT - windowPadding.y - textSize.y);
    ImGui::Text(authorInfo.c_str());

    // Check if an exit operation is required
    if (this->uiStatus & static_cast<DWORD>(GuiState::GuiState_Exiting))
        this->ExitButton();

    // Display the window title at the top of the ImGui window
    ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y));
    ImGui::Text(windowTitle.c_str());

    // Insert your custom ImGui code here
    ImGui::Text("Hello World");
    ImGui::Checkbox("Hello World##0", (bool*)this->lpBuffer);
    ImGui::Button("Hello World##1", ImVec2(100.0f, 20.0f));

    ImGui::End();
}

void GuiWindow::ExitButton()
{
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChildFrame(CHILD_FRAME_ID, ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChild("Exiting", ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));

    std::string strText = std::string("Do you want to exit this program?");
    ImVec2 textSize = ImGui::CalcTextSize(strText.c_str());
    ImGui::SetCursorPos(ImVec2((WINDOW_WIDTH - textSize.x) * 0.5f, WINDOW_HEIGHT * 0.382f - textSize.y * 0.5f));
    ImGui::Text(strText.c_str());

    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH * 0.5f - 120, WINDOW_HEIGHT * 0.618f));
    if (ImGui::Button("Confirm", ImVec2(100.0f, 50.0f)))
        this->uiStatus |= static_cast<DWORD>(GuiState::GuiState_Detach);

    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH * 0.5f + 20, WINDOW_HEIGHT * 0.618f));
    if (ImGui::Button("Cancel", ImVec2(100.0f, 50.0f)))
        this->uiStatus &= ~static_cast<DWORD>(GuiState::GuiState_Exiting);

    ImGui::EndChild();
    ImGui::EndChildFrame();
}