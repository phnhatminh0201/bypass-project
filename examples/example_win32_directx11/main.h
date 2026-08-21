// MADE BY FINEX BOYZZ.
// DC LINK: https://discord.gg/AHwg2YA6sE 

#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <algorithm>
#include <sstream>
#include <chrono>
#include "Fonts.h"
#include "images.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"
#include "imgui_settings.h"
#include "SoundPlayer.h"
#include <atlsecurity.h> 
#include <ctime>
#include <D3DX11.h>
#include <d3d11.h>
#include <D3DX11tex.h>
#include <dwmapi.h>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <shlobj.h>
#include <ShObjIdl_core.h>
#include <string>
#include <TlHelp32.h>
#include <tchar.h>
#include <thread>
#include <Windows.h>
#pragma comment (lib, "d3dx11.lib")

HWND hwnd;
RECT rc;
D3DX11_IMAGE_LOAD_INFO info;
ID3DX11ThreadPump* pump{ nullptr };


static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


void AnimatedCaption(const std::vector<std::string>& captions, ImVec2 center_pos)
{
    auto draw = ImGui::GetWindowDrawList();

    static int current_index = 0;
    static float timer = 0.0f;
    static float display_duration = 2.5f;
    static float randomize_duration = 1.5f;

    static std::string display_text;
    static std::vector<char> char_states;

    static bool resolving = false;
    static bool waiting = false;

    ImGuiIO& io = ImGui::GetIO();
    timer += io.DeltaTime;

    std::string& target = const_cast<std::string&>(captions[current_index]);

    if (display_text.empty() || target.length() != display_text.length())
    {
        display_text = std::string(target.length(), ' ');
        char_states = std::vector<char>(target.length(), 0);
    }

    float t = timer / (resolving ? display_duration : randomize_duration);
    t = ImClamp(t, 0.0f, 1.0f);
    float curve_speed = resolving ? (1.0f - t) : t;
    curve_speed = powf(curve_speed, 2.0f);
    float rand_chance = 1.0f - curve_speed * 0.9f;

    if (!waiting)
    {
        for (size_t i = 0; i < target.length(); ++i)
        {
            if (target[i] == ' ')
            {
                display_text[i] = ' ';
                continue;
            }

            if (!resolving)
            {
                if (ImGui::GetIO().Framerate > 0.0f && ((rand() % 100) / 100.0f < curve_speed))
                    display_text[i] = (char)('A' + (rand() % 26));
            }
            else if (char_states[i] == 0)
            {
                if ((rand() / (float)RAND_MAX) < rand_chance)
                    display_text[i] = (char)('A' + (rand() % 26));

                if ((rand() % 100) < 10)
                {
                    display_text[i] = target[i];
                    char_states[i] = 1;
                }
            }
        }
    }

    if (!resolving && timer >= randomize_duration)
    {
        resolving = true;
        timer = 0.0f;
    }
    else if (resolving && timer >= display_duration)
    {
        waiting = true;
        timer = 0.0f;
    }
    else if (waiting && timer >= 1.0f)
    {
        current_index = (current_index + 1) % captions.size();
        display_text.clear();
        char_states.clear();
        resolving = false;
        waiting = false;
        timer = 0.0f;
    }


    ImGui::PushFont(Montserrat);

    ImVec2 text_size = ImGui::CalcTextSize(display_text.c_str());
    ImVec2 pos = center_pos - ImVec2(text_size.x / 2.0f, 0.0f);
    ImGui::ShadowText(display_text.c_str(), main_color, main_color, 50.f, pos);
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0, text_size.y + 10));
}

time_t string_to_timet(const std::string& time_str) {
    return static_cast<time_t>(std::stoll(time_str));
}

tm timet_to_tm(time_t t) {
    tm timeinfo;
    localtime_s(&timeinfo, &t);
    return timeinfo;
}

std::string tm_to_readable_time(const tm& t) {
    std::ostringstream oss;
    oss << std::put_time(&t, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string FormatExpiryDate(const std::string& timestampStr) {

    if (timestampStr.empty() || !std::all_of(timestampStr.begin(), timestampStr.end(), ::isdigit)) {
        return "Invalid timestamp";
    }

    std::time_t expiryTime = std::stoll(timestampStr);
    std::tm* tm = std::localtime(&expiryTime);

    if (!tm) {
        return "Invalid time";
    }

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm);
    return std::string(buffer);
}

void RainAnim(ImDrawList* d, ImVec2 a, ImVec2 b, ImVec2 sz, ImVec2, float t)
{
    for (int i = 0; i < 3000; ++i) {
        unsigned h = ImGui::GetID((void*)(intptr_t)(d + i + int(t / 4)));
        auto f = fmodf(t + fmodf(h / 777.f, 99), 99);
        auto tx = h % (int)sz.x;
        auto ty = h % (int)sz.y;

        if (f < 1) {
            auto py = ty - 1000 * (1 - f);


            float line_length = (std::min)(py + 10, (float)ty) - py;
            ImVec2 points[4] = {
                { a.x + tx - 1.0f, a.y + py },         
                { a.x + tx + 1.0f, a.y + py },         
                { a.x + tx + 1.0f, a.y + py + line_length },
                { a.x + tx - 1.0f, a.y + py + line_length } 
            };


            d->AddShadowConvexPoly(
                points,
                4,
                main_color,
                45.f,
                ImVec2(0, 0),
                ImDrawFlags_None
            );


            d->AddConvexPolyFilled(points, 4, main_color);
        }
        else if (f < 1.2f) {

            d->AddShadowCircle(ImVec2(a.x + tx, a.y + ty),
                1.1f + (f - 1) * 10 + h % 5,
                ImColor(main_color.Value.x, main_color.Value.y, main_color.Value.z, 0.5f), 45.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 36);

            d->AddCircle(ImVec2(a.x + tx, a.y + ty),
                1.1f + (f - 1) * 10 + h % 5,
                ImColor(main_color.Value.x, main_color.Value.y, main_color.Value.z, 0.5f));


        }
    }
}

extern ImVec4 accent_col;

void DrawTriangleParticles()
{
    ImVec2 screen_size = { (float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN) };

    static ImVec2 partile_pos[100];
    static ImVec2 partile_target_pos[100];
    static float partile_speed[100];
    static float partile_size[100];
    static float partile_radius[100];
    static float partile_rotate[100];

    for (int i = 1; i < 60; i++)
    {
        if (partile_pos[i].x == 0 || partile_pos[i].y == 0)
        {
            partile_pos[i].x = rand() % (int)screen_size.x + 1;
            partile_pos[i].y = -15.f;
            partile_speed[i] = 1 + rand() % 25;
            partile_radius[i] = rand() % 4;
            partile_size[i] = rand() % 3;

            partile_target_pos[i].x = rand() % (int)screen_size.x;
            partile_target_pos[i].y = screen_size.y * 2;
        }

        partile_pos[i] = ImLerp(partile_pos[i], partile_target_pos[i], ImGui::GetIO().DeltaTime * (partile_speed[i] / 60));
        partile_rotate[i] += ImGui::GetIO().DeltaTime;

        if (partile_pos[i].y > screen_size.y)
        {
            partile_pos[i].x = 0;
            partile_pos[i].y = 0;
            partile_rotate[i] = 0;
        }

        float s = partile_size[i] + 2.0f;

        ImVec2 p1 = ImVec2(partile_pos[i].x, partile_pos[i].y - s);
        ImVec2 p2 = ImVec2(partile_pos[i].x - s, partile_pos[i].y + s);
        ImVec2 p3 = ImVec2(partile_pos[i].x + s, partile_pos[i].y + s);

        ImGui::GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, ImGui::GetColorU32(accent_col));

        ImGui::GetWindowDrawList()->AddShadowCircle(partile_pos[i], 6.f, ImGui::GetColorU32(accent_col), 40.f + partile_size[i], ImVec2(0, 0), 0, 20);
    }
}


void ToggleClickability(bool clickable) {
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (clickable) {
        exStyle &= ~WS_EX_TRANSPARENT;
    }
    else {
        exStyle |= WS_EX_TRANSPARENT;
    }
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
}

void ToggleClickability(bool clickable);

void move_window(float h = 0.f) {
    float mh = (h > 0.f) ? h : login_size.y;
    ImGui::SetCursorPos(ImVec2(0, 0));
    if (ImGui::InvisibleButton("Move_detector", ImVec2(login_size.x, mh)));
    if (ImGui::IsItemActive()) {

        GetWindowRect(hwnd, &rc);
        MoveWindow(hwnd, rc.left + ImGui::GetMouseDragDelta().x, rc.top + ImGui::GetMouseDragDelta().y, login_size.x, login_size.y, TRUE);
    }
}

void move_window2() {
    ImGui::SetCursorPos(ImVec2(0, 0));
    if (ImGui::InvisibleButton("Move_detector", ImVec2(menu_size.x, menu_size.y)));
    if (ImGui::IsItemActive()) {

        GetWindowRect(hwnd, &rc);
        MoveWindow(hwnd, rc.left + ImGui::GetMouseDragDelta().x, rc.top + ImGui::GetMouseDragDelta().y, menu_size.x, menu_size.y, TRUE);
    }
}


int rotation_start_index;
void ImRotateStart()
{
    rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
}

ImVec2 ImRotationCenter()
{
    ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);

    const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

    return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);
}


void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
{
    float s = sin(rad), c = cos(rad);
    center = ImRotate(center, s, c) - center;

    auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
}

int g_rotation_start_index;
void g_ImRotateStart()
{
    g_rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
}

ImVec2 g_ImRotationCenter()
{
    ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);
    const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = g_rotation_start_index; i < buf.Size; i++)
        l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);
    return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);
}

void g_ImRotateEnd(float rad, ImVec2 center = g_ImRotationCenter())
{
    float s = sin(rad), c = cos(rad);
    center = ImRotate(center, s, c) - center;
    auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = g_rotation_start_index; i < buf.Size; i++)
        buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
}

void ImGui::ImageRotation(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImU32 tint_col, float speed)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGui::SetCursorPos(ImVec2(menu_size.x / 2 - size.x / 2, menu_size.y / 2 - size.y / 2));

    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);

    ItemSize(bb);
    if (!ItemAdd(bb, 0))
        return;

    static float rotation = 0.f;
    rotation += speed / ImGui::GetIO().Framerate * 60.f;

    ImRotateStart();
    window->DrawList->AddImage(user_texture_id, bb.Min, bb.Max, uv0, uv1, tint_col);

    ImRotateEnd(rotation);
}


namespace ImGui
{
    int rotation_start_index;
    void ImRotateStart()
    {
        rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
    }

    ImVec2 ImRotationCenter()
    {
        ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);

        const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
        for (int i = rotation_start_index; i < buf.Size; i++)
            l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

        return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);
    }


    void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
    {
        float s = sin(rad), c = cos(rad);
        center = ImRotate(center, s, c) - center;

        auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
        for (int i = rotation_start_index; i < buf.Size; i++)
            buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
    }
}




bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        levels, 2, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice,
        nullptr, &g_pd3dDeviceContext) != S_OK)
        return false;
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBack;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBack));
    g_pd3dDevice->CreateRenderTargetView(pBack, nullptr, &g_mainRenderTargetView);
    pBack->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_ResizeWidth = (UINT)LOWORD(lParam);
            g_ResizeHeight = (UINT)HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

