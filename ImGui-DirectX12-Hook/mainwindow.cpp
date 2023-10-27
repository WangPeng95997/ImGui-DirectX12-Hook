#include "mainwindow.h"
#include "utils.h"

GuiWindow::GuiWindow()
{
    // 申请内存
    this->lpBuffer = (LPBYTE)::VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    ::memset(this->lpBuffer, 0xCC, 0x1000);

    // 加载中文字体
    this->fontPath = new char[MAX_PATH] {};
    ::GetEnvironmentVariableA("WINDIR", this->fontPath, MAX_PATH);
    ::strcat_s(this->fontPath, MAX_PATH, "\\Fonts\\msyh.ttc");

    // 窗口名称
    std::string strText;
    strText = strText.append(WINDOWNAME).append(" v") + std::to_string(MAJORVERSION) + std::string().append(".") + std::to_string(MINORVERSION) + std::string().append(".") + std::to_string(REVISIONVERSION);
    size_t nLength = strText.length() + 1;
    this->windowName = new char[nLength] {};
    ::memcpy(this->windowName, strText.c_str(), strText.length());

    // 窗口设置
    this->startPostion = ImVec2(0.0f, 0.0f);
    this->windowStatus = WindowStatus::Repaint;

    // 功能菜单
    this->showMenu = true;
    this->crossHair = false;
}

GuiWindow::~GuiWindow()
{
    ::VirtualFree(this->lpBuffer, 0, MEM_RELEASE);

    delete[] this->fontPath;
    delete[] this->windowName;
}

void GuiWindow::Init()
{
    do
    {
        this->hwnd = FindWindow(TARGETCLASS, TARGETWINDOW);
        this->hProcess = ::GetCurrentProcess();
        this->hModule = GetModuleHandle(TARGETMODULE);
        Sleep(200);
    } while (this->hwnd == NULL || this->hProcess == NULL || this->hModule == NULL);

    this->baseAddress = (LPBYTE)this->hModule;
}

void GuiWindow::Repaint()
{
    ImGui::SetWindowPos(this->startPostion);
    ImGui::SetWindowSize(ImVec2(WIDTH, HEIGHT));
    this->windowStatus &= ~WindowStatus::Repaint;
}

void GuiWindow::Update()
{
    if (this->showMenu)
    {
        const ImGuiWindowFlags windowflags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin(this->windowName, nullptr, windowflags);
        ImVec2 windowPostion = ImGui::GetWindowPos();

        ImGui::Text(this->windowName);

        if (this->windowStatus & WindowStatus::Repaint)
            Repaint();

        if (ImGui::CloseButton(0x1000, ImVec2(windowPostion.x + WIDTH - 20.0f, windowPostion.y)))
            Button_Exit();

        if (ImGui::Checkbox(u8"绘制准星", &this->crossHair))
            Toggle_CrossHair(this->crossHair);

        const std::string authorInfo = std::string(AUTHORINFO);
        ImVec2 textSize = ImGui::CalcTextSize(authorInfo.c_str());
        ImGui::SetCursorPosY(HEIGHT - textSize.y);
        ImGui::Text(authorInfo.c_str());

        const std::string hotKey = std::string(u8"M键显示/隐藏界面");
        textSize = ImGui::CalcTextSize(hotKey.c_str());
        ImGui::SetCursorPos(ImVec2(WIDTH - textSize.x, HEIGHT - textSize.y));
        ImGui::Text(hotKey.c_str());
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (this->crossHair)
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        drawList->AddLine(
            ImVec2(displaySize.x / 2 - 30.0f, displaySize.y / 2),
            ImVec2(displaySize.x / 2 + 30.0f, displaySize.y / 2),
            ImColor(0, 255, 0, 255));
        drawList->AddLine(
            ImVec2(displaySize.x / 2, displaySize.y / 2 - 30.0f),
            ImVec2(displaySize.x / 2, displaySize.y / 2 + 30.0f),
            ImColor(0, 255, 0, 255));
    }

    ImGui::End();
}

void GuiWindow::Button_Exit()
{
    this->windowStatus |= WindowStatus::Exit;
}

void GuiWindow::Toggle_CrossHair(const bool& isEnable)
{
    this->crossHair = isEnable;
}