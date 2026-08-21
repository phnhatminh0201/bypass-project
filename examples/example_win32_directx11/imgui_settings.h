// MADE BY FINEX BOYZZ.
// DC LINK: https://discord.gg/AHwg2YA6sE 

#pragma once

#include <algorithm>
#include <string>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <D3D11.h>
#include <vector>

inline ImFont* icon_font;
inline ImFont* icon_font2;
inline ImFont* icon_font3;
inline ImFont* second_font;
inline ImFont* poppinsreg;
inline ImFont* inter_default = nullptr;
inline ImFont* nevan = nullptr;
inline ImFont* Montserrat = nullptr;
inline ImFont* Montserrat_1 = nullptr;
inline ImFont* Montserrat_6 = nullptr;
inline ImFont* Montserrat_7 = nullptr;

extern ID3D11ShaderResourceView* widget_ico;
extern ID3D11ShaderResourceView* keybind_ico;
extern ID3D11ShaderResourceView* combo;

inline ImGuiWindow* main_window;

inline char search[64];

inline ImVec2 login_size = ImVec2(320, 380);

inline ImVec2 menu_size = ImVec2(475, 500);

inline ImVec2 frame_size = ImVec2(500, 50);

inline ImColor main_color(0, 0, 0, 255);

inline ImColor decent_green(0, 184, 12);

inline ImColor red(255, 0, 0);

inline ImVec4 rain_color = ImColor(0, 0, 0);

inline ImColor white(255, 255, 255, 255);

inline ImColor black(0, 0, 0, 255);

inline ImVec4 circle_rect = ImColor(215, 215, 215);

inline ImVec4 circle_inactive_1 = ImColor(150, 150, 150);

inline ImColor icon_inactive(255, 255, 255, 50);

inline ImColor toggle_bgcol(245, 255, 0, 10);

inline ImColor second_color(55, 55, 55, 250);

inline float anim_speed = 15.f;

inline ImVec4 text_hov = ImColor(245, 255, 0, 255);

inline ImVec4 text = ImColor(255, 255, 255);

inline ImVec4 text2 = ImColor(255, 255, 255, 70);

inline ImColor line_color(255, 255, 255, 0);

inline ImColor button(30, 30, 30, 60);


inline ImVec2 center_text(ImVec2 min, ImVec2 max, const char* text)
{
    return min + (max - min) / 2 - ImGui::CalcTextSize(text) / 2;
}


struct Notification
{
    std::string message, information;
    ImColor color;
    ImRect bb;
    float timer = 0;
    float alpha = 0.0f;
    float y_position = 0;
    float target_y_position = 0;
    float fill_progress = 0.0f;  
    bool fading_out = false;
};

class NotificationSystem
{
private:
    std::vector<Notification> notifications;

public:
    void AddNotification(const std::string& message, const std::string& information, ImColor color)
    {
        Notification notif;
        notif.message = message;
        notif.information = information;
        notif.color = color;
        notif.timer = 0;
        notif.alpha = 0.0f;
        notif.y_position = 0;
        notif.target_y_position = 0;
        notif.fading_out = false;
        notifications.push_back(notif);
    }

    void DrawNotifications()
    {
        float spacing = 78.0f;
        const ImVec2 screenSize = ImGui::GetIO().DisplaySize;

        float baseY = screenSize.y - 80.0f;
        for (int i = notifications.size() - 1; i >= 0; --i)
        {
            Notification& notif = notifications[i];
            notif.target_y_position = baseY;
            baseY -= spacing;
        }

        for (int i = 0; i < notifications.size(); )
        {
            Notification& notif = notifications[i];
            notif.timer += ImGui::GetIO().DeltaTime;

            float fadeInSpeed = 3.0f;
            float fadeOutDelay = 3.0f;
            float fadeOutSpeed = 1.0f;
            float fillSpeed = 3.5f;

            if (!notif.fading_out && notif.alpha < 1.0f)
                notif.alpha = ImMin(notif.alpha + ImGui::GetIO().DeltaTime * fadeInSpeed, 1.0f);

            if (!notif.fading_out && notif.timer >= fadeOutDelay)
                notif.fading_out = true;

            if (notif.fading_out)
                notif.alpha = ImMax(notif.alpha - ImGui::GetIO().DeltaTime * fadeOutSpeed, 0.0f);

            if (!notif.fading_out)
                notif.fill_progress = ImMin(notif.fill_progress + ImGui::GetIO().DeltaTime * fillSpeed, 1.0f);
            else
                notif.fill_progress = ImMax(notif.fill_progress - ImGui::GetIO().DeltaTime * fillSpeed * 0.8f, 0.0f);

            if (notif.y_position == 0.0f && notif.target_y_position != 0.0f)
                notif.y_position = notif.target_y_position;

            notif.y_position = ImLerp(notif.y_position, notif.target_y_position, ImGui::GetIO().DeltaTime * 10.0f);

            ImGui::PushFont(inter_default);
            ImVec2 msgSize = ImGui::CalcTextSize(notif.message.c_str());
            ImVec2 infSize = ImGui::CalcTextSize(notif.information.c_str());
            ImGui::PopFont();

            float padding = 25.0f;
            float notifWidth = ImMax(msgSize.x, infSize.x) + padding;
            float notifHeight = 68.0f;

            ImVec2 notifPos = ImVec2(screenSize.x - notifWidth - 20, notif.y_position);
            notif.bb = ImRect(notifPos, notifPos + ImVec2(notifWidth, notifHeight));

            ImDrawList* draw = ImGui::GetForegroundDrawList();
            ImU32 bgColor = ImColor(10, 10, 10, int(220 * notif.alpha));
            draw->AddRectFilled(notif.bb.Min, notif.bb.Max, bgColor, 3.0f, ImDrawFlags_RoundCornersAll);

            draw->AddText(notif.bb.Min + ImVec2(12.5f, 8.f), ImGui::GetColorU32(ImVec4(1, 1, 1, notif.alpha)), notif.message.c_str());

            ImGui::PushFont(inter_default);
            draw->AddText(notif.bb.Min + ImVec2(12.5f, 28.f), ImGui::GetColorU32(ImVec4(0.8f, 0.8f, 0.8f, notif.alpha)), notif.information.c_str());
            ImGui::PopFont();

            float glowHeight = 10.0f;
            float fullGlowWidth = notif.bb.GetWidth() - 20;
            float animatedGlowWidth = fullGlowWidth * notif.fill_progress;

            ImVec2 glowStart = notif.bb.Min + ImVec2(10, notif.bb.GetHeight() - glowHeight - 6);
            ImVec2 glowEnd = glowStart + ImVec2(animatedGlowWidth, glowHeight - 5);

            ImU32 glowColor = ImGui::GetColorU32(ImVec4(notif.color.Value.x, notif.color.Value.y, notif.color.Value.z, notif.alpha));
            draw->AddRectFilled(glowStart, glowEnd, glowColor);
            draw->AddShadowRect(glowStart, glowEnd, glowColor, 25.f, ImVec2(0, 0), 0.0f);

            if (notif.alpha <= 0.01f && notif.fill_progress <= 0.01f)
            {
                notifications.erase(notifications.begin() + i);
                continue;
            }

            ++i;
        }
    }
};


inline NotificationSystem notificationSystem;
