
#define IMGUI_DEFINE_MATH_OPERATORS
#include <cmath>
#include <map>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <Urlmon.h>
#include "main.h"
#include "BypassManager.h"
#include "resource.h"

#include "auth/auth.hpp"
#include "auth/skStr.h"


using namespace KeyAuth;

// Forward declare AddNotification for BypassManager compatibility
void AddNotification(const std::string& message, const std::string& type = "error", float displayTime = 3.0f);

// Global CertManager instance
static CertManager certMgr;





std::string name = skCrypt("CHEAT HUB V6").decrypt();
std::string ownerid = skCrypt("23uqRrzQnd").decrypt();
std::string version = skCrypt("1.0").decrypt();
std::string url = skCrypt("https://keyauth.win/api/1.3/").decrypt();
std::string path = skCrypt("").decrypt();


api KeyAuthApp(name, ownerid, version, url, path);



ID3D11ShaderResourceView* widget_ico = nullptr;
ID3D11ShaderResourceView* keybind_ico = nullptr;
ID3D11ShaderResourceView* combo = nullptr;



static ImVec4 accent_col = ImVec4(0.35f, 0.75f, 1.0f, 1.0f);
static ImVec4 scheme_text = ImVec4(200/255.f, 202/255.f, 212/255.f, 1.f);
static ImVec4 section_tint = ImVec4(0.5f, 0.5f, 0.5f, 1.f);

// ── Login / Auth State ─────────────────────────────────────────
enum LoginPhase { LOGIN, SUCCESS, RESIZING, MAIN, LOGGING_OUT };
static LoginPhase login_phase = LOGIN;
static float login_alpha = 1.f;
static float resize_progress = 1.f;
static ImVec2 current_win_size = login_size;

static char username_buf[124] = "";
static char password_buf[124] = "";
static char key_buf[128] = "";
static bool show_password = false;
static bool remember_me = false;
static bool show_register = false;
static bool authenticating = false;
static bool auth_done = false;
static bool auth_result = false;
static std::string auth_error_msg;
static bool streamermode_enabled = false;
bool streamer = false;
static void SaveRememberMe()
{
    char path[MAX_PATH]; GetTempPathA(MAX_PATH, path);
    strcat_s(path, "FinexLogin.txt");
    FILE* f;
    if (fopen_s(&f, path, "w") == 0) {
        fprintf(f, "%s\n", key_buf);
        fclose(f);
    }
}
static void LoadRememberMe()
{
    char path[MAX_PATH]; GetTempPathA(MAX_PATH, path);
    strcat_s(path, "FinexLogin.txt");
    FILE* f;
    if (fopen_s(&f, path, "r") == 0) {
        if (fgets(key_buf, sizeof(key_buf), f)) {
            size_t len = strlen(key_buf);
            if (len > 0 && key_buf[len-1] == '\n') key_buf[len-1] = 0;
        }
        fclose(f);
        remember_me = true;
    }
}

struct ToggleAnim { float t = 0.f; };
static std::map<ImGuiID, ToggleAnim> g_toggle_anim;

static bool ThemedToggle(const char* label, bool* value)
{
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (!w) return false;
    ImGuiID id = ImGui::GetID(label);
    ImVec2 sz = ImVec2(36.f, 20.f);
    ImVec2 pos = w->DC.CursorPos;
    const ImRect bb(pos, pos + sz);
    ImGui::ItemSize(sz);
    if (!ImGui::ItemAdd(bb, id)) return *value;
    bool hovered, held, pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *value = !*value;
    ToggleAnim& anim = g_toggle_anim[id];
    anim.t = ImLerp(anim.t, *value ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 12.f);
    float r = sz.y * 0.5f;
    ImVec4 ac4 = ImVec4(accent_col.x, accent_col.y, accent_col.z, 1.f);
    ImU32 track_col = ImGui::GetColorU32(ImVec4(ac4.x, ac4.y, ac4.z, 0.22f + 0.30f * anim.t));
    ImU32 thumb_col = ImGui::GetColorU32(ImVec4(ac4.x, ac4.y, ac4.z, 0.55f + 0.45f * anim.t));
    float thumb_ofs = anim.t * (sz.x - sz.y);
    w->DrawList->AddRectFilled(bb.Min, bb.Max, track_col, r);
    w->DrawList->AddCircleFilled(ImVec2(bb.Min.x + r + thumb_ofs, bb.Min.y + r), r - 2.f, thumb_col);
    w->DrawList->AddShadowCircle(ImVec2(bb.Min.x + r + thumb_ofs, bb.Min.y + r), r - 2.f, IM_COL32(0, 0, 0, (int)(20 * anim.t)), 10.f, ImVec2(0, 1), 0);
    float lh = ImGui::GetTextLineHeight();
    ImU32 lbl_col = ImGui::GetColorU32(ImVec4(0.6f, 0.6f, 0.65f, 1.f));
    w->DrawList->AddText(ImVec2(bb.Max.x + 8.f, bb.Min.y + (sz.y - lh) * 0.5f), lbl_col, label);
    return pressed;
}

static ImU32 AccentCol(float alpha = 1.f)
{
    return IM_COL32((int)(accent_col.x * 255.f), (int)(accent_col.y * 255.f), (int)(accent_col.z * 255.f), (int)(alpha * 255.f));
}

namespace edited {

struct check_state { float alpha; float hover_t; float rot_x; float rot_y; float elevation; float row_scale; float press_t; ImVec2 part_pos[30]; ImVec2 part_vel[30]; float part_rad[30]; float part_alpha[30]; };

bool Checkbox(const char* label, const char* description, bool* v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const float ch = 56.f;
    const float cw = ImGui::GetContentRegionAvail().x - 6.f;
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(cw, ch));
    static std::map<ImGuiID, check_state> anim;
    auto& state = anim[id];

    ImGui::ItemSize(ImVec2(cw, ch), -1.f);
    if (!ImGui::ItemAdd(rect, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
    if (pressed) { *v = !(*v); ImGui::MarkItemEdited(id); }

    state.alpha = ImLerp(state.alpha, *v ? 1.f : 0.f, g.IO.DeltaTime * 8.f);
    state.hover_t = ImLerp(state.hover_t, hovered ? 1.f : 0.f, g.IO.DeltaTime * 10.f);

    {
        ImVec2 mouse = ImGui::GetIO().MousePos;
        ImVec2 rc = rect.GetCenter();
        float rx = ImClamp((mouse.x - rc.x) / (cw * 0.5f), -1.f, 1.f);
        float ry = ImClamp((mouse.y - rc.y) / (ch * 0.5f), -1.f, 1.f);
        float target_rx = hovered ? ry * 0.3f : 0.f;
        float target_ry = hovered ? rx * 0.3f : 0.f;
        state.rot_x = ImLerp(state.rot_x, target_rx, g.IO.DeltaTime * 12.f);
        state.rot_y = ImLerp(state.rot_y, target_ry, g.IO.DeltaTime * 12.f);
    }

    state.elevation = ImLerp(state.elevation, hovered ? 10.f : 0.f, g.IO.DeltaTime * 12.f);
    state.row_scale = ImLerp(state.row_scale, hovered ? 1.06f : 1.f, g.IO.DeltaTime * 10.f);
    state.press_t = ImLerp(state.press_t, held ? 1.f : 0.f, g.IO.DeltaTime * 15.f);

    float elevation = state.elevation + state.press_t * 4.f;
    float row_s = state.row_scale * (1.f - state.press_t * 0.06f);
    float rdx = state.rot_x;
    float rdy = state.rot_y - elevation;

    ImVec2 rc = rect.GetCenter();
    ImVec2 rh = ImVec2(cw * 0.5f, ch * 0.5f) * row_s;
    ImVec2 rmin = rc - rh + ImVec2(rdx, rdy);
    ImVec2 rmax = rc + rh + ImVec2(rdx, rdy);

    bool hover_pop = state.hover_t > 0.01f;
    ImDrawList* dl = hover_pop ? ImGui::GetForegroundDrawList() : window->DrawList;

    if (state.hover_t > 0.01f) {
        float shad_a = ImLerp(0.f, 40.f, state.hover_t);
        float shad_thk = ImLerp(0.f, 22.f, state.hover_t);
        float shad_off = ImLerp(0.f, 8.f, state.hover_t);
        dl->AddShadowRect(rmin, rmax, IM_COL32(0,0,0,(int)shad_a), shad_thk, ImVec2(0, shad_off), 0, 6.f);
    }
    if (state.hover_t > 0.01f) {
        float aura_a = ImLerp(0.f, 10.f, state.hover_t);
        dl->AddShadowRect(rmin, rmax, AccentCol(aura_a / 255.f), 28.f, ImVec2(0, 0), 6.f);
    }

    ImVec4 row_bg = ImLerp(ImVec4(0.08f,0.08f,0.10f,1), ImVec4(0.12f,0.12f,0.16f,1), state.hover_t);
    ImVec4 row_border = ImLerp(ImVec4(0.16f,0.16f,0.20f,1), ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.78f), state.hover_t);
    dl->AddRectFilled(rmin, rmax, ImGui::GetColorU32(row_bg), 6.f);

    if (state.hover_t > 0.01f) {
        float hl_a = ImLerp(0.f, 35.f, state.hover_t);
        dl->AddRectFilled(rmin, ImVec2(rmax.x, rmin.y + 20.f * row_s), IM_COL32(255,255,255,(int)hl_a), 6.f, ImDrawFlags_RoundCornersTop);
    }

    dl->AddRect(rmin, rmax, ImGui::GetColorU32(row_border), 6.f, 0, ImLerp(0.5f, 0.8f, state.hover_t));

    ImU32 accent_col_c = AccentCol(state.alpha);
    dl->AddRectFilled(rmin, ImVec2(rmin.x + 3, rmax.y), accent_col_c, 6.f, ImDrawFlags_RoundCornersLeft);

    // Particles
    {
        ImVec2 cp = pos + ImVec2(14 + 11.f, ch * 0.5f) + ImVec2(rdx, rdy);
        if (*v || state.alpha > 0.01f) {
            for (int p = 0; p < 30; p++) {
                if (state.part_alpha[p] <= 0.f) {
                    state.part_pos[p] = cp + ImVec2((rand() % 200 - 100) * 0.1f, (rand() % 200 - 100) * 0.1f);
                    state.part_vel[p] = ImVec2((rand() % 200 - 100) * 0.3f, (rand() % 200 - 100) * 0.3f);
                    state.part_rad[p] = 1.f + (rand() % 20) * 0.1f;
                    state.part_alpha[p] = 0.3f + (rand() % 70) * 0.01f;
                }
                state.part_pos[p] += state.part_vel[p] * g.IO.DeltaTime;
                state.part_alpha[p] -= g.IO.DeltaTime * 0.5f;
                float pa = ImMin(state.part_alpha[p], state.alpha);
                if (pa > 0.01f)
                    dl->AddCircleFilled(state.part_pos[p], state.part_rad[p], AccentCol(pa * 0.3f), 20);
            }
        } else {
            for (int p = 0; p < 30; p++) state.part_alpha[p] = 0.f;
        }
    }

    float cb_scale = ImLerp(1.f, 1.08f, state.hover_t);
    const float sq = 22.f;
    float cb_sq = sq * cb_scale;
    ImVec2 cb_center = pos + ImVec2(14 + sq * 0.5f, ch * 0.5f) + ImVec2(rdx, rdy);
    ImRect cb(cb_center - ImVec2(cb_sq * 0.5f, cb_sq * 0.5f), cb_center + ImVec2(cb_sq * 0.5f, cb_sq * 0.5f));

    dl->AddRectFilled(cb.Min, cb.Max, IM_COL32(40,41,46,255), 6.f);
    if (state.alpha > 0.01f) {
        dl->AddRectFilled(cb.Min, cb.Max, AccentCol(state.alpha), 6.f);
        dl->AddRectFilled(cb.Min, cb.Max, IM_COL32(ImMin(255, (int)(accent_col.x * 255.f + 60)), ImMin(255, (int)(accent_col.y * 255.f + 60)), ImMin(255, (int)(accent_col.z * 255.f + 60)), (int)(60 * state.alpha)), 6.f);
    }
    ImVec4 cb_border = ImLerp(ImVec4(100/255.f,103/255.f,116/255.f,1), ImVec4(ImMax(0.f, accent_col.x - 0.05f), ImMax(0.f, accent_col.y - 0.05f), ImMax(0.f, accent_col.z - 0.05f), 1.f), state.alpha);
    cb_border = ImLerp(cb_border, ImVec4(ImMin(1.f, accent_col.x + 0.1f), ImMin(1.f, accent_col.y + 0.1f), ImMin(1.f, accent_col.z + 0.1f), 1.f), state.hover_t * 0.4f);
    dl->AddRect(cb.Min, cb.Max, ImGui::GetColorU32(cb_border), 6.f, 0, ImLerp(1.f, 1.6f, state.hover_t));

    if (state.alpha > 0.01f) {
        ImVec2 c = cb.GetCenter();
        ImVec2 pts[3] = { c + ImVec2(-5,  0), c + ImVec2(-1, +4), c + ImVec2(+6, -5) };
        dl->AddPolyline(pts, 3, IM_COL32(255,255,255,(int)(255 * state.alpha)), 0, ImLerp(2.2f, 2.6f, state.hover_t));
    }

    ImVec4 label_base = ImLerp(ImVec4(180/255.f,182/255.f,195/255.f,1), ImVec4(240/255.f,240/255.f,245/255.f,1), state.alpha);
    ImU32 label_col = ImGui::GetColorU32(label_base);
    ImVec2 label_pos = pos + ImVec2(48, 10) + ImVec2(rdx, rdy);

    if (state.hover_t > 0.01f) {
        float ts_a = ImLerp(0.f, 60.f, state.hover_t);
        ImGui::PushFont(Montserrat_1);
        dl->AddText(label_pos + ImVec2(0, 0.6f), IM_COL32(0,0,0,(int)ts_a), label);
        ImGui::PopFont();
    }
    ImGui::PushFont(Montserrat_1);
    dl->AddText(label_pos, label_col, label);
    ImGui::PopFont();

    ImVec4 desc_base = ImVec4(130/255.f,132/255.f,145/255.f,1);
    ImU32 desc_col = ImGui::GetColorU32(desc_base);
    ImVec2 desc_pos = pos + ImVec2(48, 30) + ImVec2(rdx, rdy);
    if (state.hover_t > 0.01f) {
        float ts_a = ImLerp(0.f, 40.f, state.hover_t);
        ImGui::PushFont(Montserrat_7);
        dl->AddText(desc_pos + ImVec2(0, 0.5f), IM_COL32(0,0,0,(int)ts_a), description);
        ImGui::PopFont();
    }
    ImGui::PushFont(Montserrat_7);
    dl->AddText(desc_pos, desc_col, description);
    ImGui::PopFont();
    return pressed;
}

bool CheckboxColor(const char* label, const char* description, bool* v, float color[4], const char* color_target, const char* pipe_cmd)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const float ch = 56.f;
    const float cw = ImGui::GetContentRegionAvail().x - 6.f;
    const float colorRegionWidth = 40.f;
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(cw, ch));

    static std::map<ImGuiID, check_state> anim;
    auto& state = anim[id];

    ImGui::ItemSize(ImVec2(cw, ch), 0.f);
    if (!ImGui::ItemAdd(rect, id)) return false;

    const ImVec2 cursor_after_row = window->DC.CursorPos;

    float cbtn_sz = 22.f;
    float cbtn_x = pos.x + cw - colorRegionWidth + (colorRegionWidth - cbtn_sz) * 0.5f;
    float cbtn_y = pos.y + (ch - cbtn_sz) * 0.5f;
    ImGui::PushID(id);
    ImGui::SetCursorScreenPos(ImVec2(cbtn_x, cbtn_y));
    ImGui::InvisibleButton("##cpick", ImVec2(cbtn_sz, cbtn_sz));
    window->DC.CursorPos = cursor_after_row;
    bool cbtn_hov = ImGui::IsItemHovered();
    if (ImGui::IsItemActivated()) ImGui::OpenPopup("color_popup");

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
    bool in_cbtn = ImGui::IsMouseHoveringRect(ImVec2(cbtn_x, cbtn_y), ImVec2(cbtn_x + cbtn_sz, cbtn_y + cbtn_sz));
    if (pressed && !in_cbtn)
    {
        *v = !(*v);
        ImGui::MarkItemEdited(id);
        std::thread([v, pipe_cmd]()
        {
           
        }).detach();
        notificationSystem.AddNotification(label, *v ? "Enabled" : "Disabled", ImColor(0, 255, 0));
    }

    state.alpha = ImLerp(state.alpha, *v ? 1.f : 0.f, g.IO.DeltaTime * 8.f);
    state.hover_t = ImLerp(state.hover_t, (hovered || cbtn_hov) ? 1.f : 0.f, g.IO.DeltaTime * 10.f);

    {
        ImVec2 mouse = ImGui::GetIO().MousePos;
        ImVec2 rc = rect.GetCenter();
        float rx = ImClamp((mouse.x - rc.x) / (cw * 0.5f), -1.f, 1.f);
        float ry = ImClamp((mouse.y - rc.y) / (ch * 0.5f), -1.f, 1.f);
        float target_rx = (hovered || cbtn_hov) ? ry * 0.3f : 0.f;
        float target_ry = (hovered || cbtn_hov) ? rx * 0.3f : 0.f;
        state.rot_x = ImLerp(state.rot_x, target_rx, g.IO.DeltaTime * 12.f);
        state.rot_y = ImLerp(state.rot_y, target_ry, g.IO.DeltaTime * 12.f);
    }

    state.elevation = ImLerp(state.elevation, (hovered || cbtn_hov) ? 10.f : 0.f, g.IO.DeltaTime * 12.f);
    state.row_scale = ImLerp(state.row_scale, (hovered || cbtn_hov) ? 1.06f : 1.f, g.IO.DeltaTime * 10.f);
    state.press_t = ImLerp(state.press_t, held ? 1.f : 0.f, g.IO.DeltaTime * 15.f);

    float elevation = state.elevation + state.press_t * 4.f;
    float row_s = state.row_scale * (1.f - state.press_t * 0.06f);
    float rdx = state.rot_x;
    float rdy = state.rot_y - elevation;

    ImVec2 rc = rect.GetCenter();
    ImVec2 rh = ImVec2(cw * 0.5f, ch * 0.5f) * row_s;
    ImVec2 rmin = rc - rh + ImVec2(rdx, rdy);
    ImVec2 rmax = rc + rh + ImVec2(rdx, rdy);

    bool hover_pop = state.hover_t > 0.01f;
    ImDrawList* dl = hover_pop ? ImGui::GetForegroundDrawList() : window->DrawList;

    if (state.hover_t > 0.01f) {
        float shad_a = ImLerp(0.f, 40.f, state.hover_t);
        float shad_thk = ImLerp(0.f, 22.f, state.hover_t);
        float shad_off = ImLerp(0.f, 8.f, state.hover_t);
        dl->AddShadowRect(rmin, rmax, IM_COL32(0,0,0,(int)shad_a), shad_thk, ImVec2(0, shad_off), 0, 6.f);
    }
    if (state.hover_t > 0.01f) {
        float aura_a = ImLerp(0.f, 10.f, state.hover_t);
        dl->AddShadowRect(rmin, rmax, AccentCol(aura_a / 255.f), 28.f, ImVec2(0, 0), 6.f);
    }

    ImVec4 row_bg = ImLerp(ImVec4(0.08f,0.08f,0.10f,1), ImVec4(0.12f,0.12f,0.16f,1), state.hover_t);
    ImVec4 row_border = ImLerp(ImVec4(0.16f,0.16f,0.20f,1), ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.78f), state.hover_t);
    dl->AddRectFilled(rmin, rmax, ImGui::GetColorU32(row_bg), 6.f);

    if (state.hover_t > 0.01f) {
        float hl_a = ImLerp(0.f, 35.f, state.hover_t);
        dl->AddRectFilled(rmin, ImVec2(rmax.x, rmin.y + 20.f * row_s), IM_COL32(255,255,255,(int)hl_a), 6.f, ImDrawFlags_RoundCornersTop);
    }

    dl->AddRect(rmin, rmax, ImGui::GetColorU32(row_border), 6.f, 0, ImLerp(0.5f, 0.8f, state.hover_t));

    ImU32 accent_col_c = AccentCol(state.alpha);
    dl->AddRectFilled(rmin, ImVec2(rmin.x + 3, rmax.y), accent_col_c, 6.f, ImDrawFlags_RoundCornersLeft);

    // Particles
    {
        ImVec2 cp = pos + ImVec2(14 + 11.f, ch * 0.5f) + ImVec2(rdx, rdy);
        if (*v || state.alpha > 0.01f) {
            for (int p = 0; p < 30; p++) {
                if (state.part_alpha[p] <= 0.f) {
                    state.part_pos[p] = cp + ImVec2((rand() % 200 - 100) * 0.1f, (rand() % 200 - 100) * 0.1f);
                    state.part_vel[p] = ImVec2((rand() % 200 - 100) * 0.3f, (rand() % 200 - 100) * 0.3f);
                    state.part_rad[p] = 1.f + (rand() % 20) * 0.1f;
                    state.part_alpha[p] = 0.3f + (rand() % 70) * 0.01f;
                }
                state.part_pos[p] += state.part_vel[p] * g.IO.DeltaTime;
                state.part_alpha[p] -= g.IO.DeltaTime * 0.5f;
                float pa = ImMin(state.part_alpha[p], state.alpha);
                if (pa > 0.01f)
                    dl->AddCircleFilled(state.part_pos[p], state.part_rad[p], AccentCol(pa * 0.3f), 20);
            }
        } else {
            for (int p = 0; p < 30; p++) state.part_alpha[p] = 0.f;
        }
    }

    float cb_scale = ImLerp(1.f, 1.08f, state.hover_t);
    const float sq = 22.f;
    float cb_sq = sq * cb_scale;
    ImVec2 cb_center = pos + ImVec2(14 + sq * 0.5f, ch * 0.5f) + ImVec2(rdx, rdy);
    ImRect cb(cb_center - ImVec2(cb_sq * 0.5f, cb_sq * 0.5f), cb_center + ImVec2(cb_sq * 0.5f, cb_sq * 0.5f));

    dl->AddRectFilled(cb.Min, cb.Max, IM_COL32(40,41,46,255), 6.f);
    if (state.alpha > 0.01f) {
        dl->AddRectFilled(cb.Min, cb.Max, AccentCol(state.alpha), 6.f);
        dl->AddRectFilled(cb.Min, cb.Max, IM_COL32(ImMin(255, (int)(accent_col.x * 255.f + 60)), ImMin(255, (int)(accent_col.y * 255.f + 60)), ImMin(255, (int)(accent_col.z * 255.f + 60)), (int)(60 * state.alpha)), 6.f);
    }
    ImVec4 cb_border = ImLerp(ImVec4(100/255.f,103/255.f,116/255.f,1), ImVec4(ImMax(0.f, accent_col.x - 0.05f), ImMax(0.f, accent_col.y - 0.05f), ImMax(0.f, accent_col.z - 0.05f), 1.f), state.alpha);
    cb_border = ImLerp(cb_border, ImVec4(ImMin(1.f, accent_col.x + 0.1f), ImMin(1.f, accent_col.y + 0.1f), ImMin(1.f, accent_col.z + 0.1f), 1.f), state.hover_t * 0.4f);
    dl->AddRect(cb.Min, cb.Max, ImGui::GetColorU32(cb_border), 6.f, 0, ImLerp(1.f, 1.6f, state.hover_t));

    if (state.alpha > 0.01f) {
        ImVec2 c = cb.GetCenter();
        ImVec2 pts[3] = { c + ImVec2(-5,  0), c + ImVec2(-1, +4), c + ImVec2(+6, -5) };
        dl->AddPolyline(pts, 3, IM_COL32(255,255,255,(int)(255 * state.alpha)), 0, ImLerp(2.2f, 2.6f, state.hover_t));
    }

    ImVec4 label_base = ImLerp(ImVec4(180/255.f,182/255.f,195/255.f,1), ImVec4(240/255.f,240/255.f,245/255.f,1), state.alpha);
    ImU32 label_col = ImGui::GetColorU32(label_base);
    ImVec2 label_pos = pos + ImVec2(48, 10) + ImVec2(rdx, rdy);

    if (state.hover_t > 0.01f) {
        float ts_a = ImLerp(0.f, 60.f, state.hover_t);
        ImGui::PushFont(Montserrat_1);
        dl->AddText(label_pos + ImVec2(0, 0.6f), IM_COL32(0,0,0,(int)ts_a), label);
        ImGui::PopFont();
    }
    ImGui::PushFont(Montserrat_1);
    dl->AddText(label_pos, label_col, label);
    ImGui::PopFont();

    ImVec4 desc_base = ImVec4(130/255.f,132/255.f,145/255.f,1);
    ImU32 desc_col = ImGui::GetColorU32(desc_base);
    ImVec2 desc_pos = pos + ImVec2(48, 30) + ImVec2(rdx, rdy);
    if (state.hover_t > 0.01f) {
        float ts_a = ImLerp(0.f, 40.f, state.hover_t);
        ImGui::PushFont(Montserrat_7);
        dl->AddText(desc_pos + ImVec2(0, 0.5f), IM_COL32(0,0,0,(int)ts_a), description);
        ImGui::PopFont();
    }
    ImGui::PushFont(Montserrat_7);
    dl->AddText(desc_pos, desc_col, description);
    ImGui::PopFont();

    // â”€â”€ Draw color picker square â”€â”€
    {
        ImVec2 cbtn_off = ImVec2(cbtn_x, cbtn_y) + ImVec2(rdx, rdy);
        ImU32 c_col = IM_COL32((int)(color[0]*255), (int)(color[1]*255), (int)(color[2]*255), (int)(color[3]*255));
        dl->AddRectFilled(cbtn_off, cbtn_off + ImVec2(cbtn_sz, cbtn_sz), c_col, 4.f);
        dl->AddRect(cbtn_off, cbtn_off + ImVec2(cbtn_sz, cbtn_sz), IM_COL32(100,103,116,255), 4.f, 0, 1.f);
        if (*v)
            dl->AddRectFilled(cbtn_off, cbtn_off + ImVec2(cbtn_sz, cbtn_sz), AccentCol(0.15f), 4.f);
    }

    // â”€â”€ Color picker popup â”€â”€
    ImGui::SetNextWindowSize(ImVec2(280, 335));
    if (ImGui::BeginPopup("color_popup", ImGuiWindowFlags_Tooltip))
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGuiIO& io = ImGui::GetIO();
        float w = ImGui::GetContentRegionAvail().x;

        float H, S, V;
        ImGui::ColorConvertRGBtoHSV(color[0], color[1], color[2], H, S, V);
        float A = color[3];

        auto send_col = [&]() {
            char buf[64];
            snprintf(buf, sizeof(buf), "color:%s:%d:%d:%d", color_target, (int)(color[0]*255), (int)(color[1]*255), (int)(color[2]*255));
          
        };

        ImGui::PushFont(Montserrat_7);
        ImGui::TextColored(ImVec4(161/255.f,161/255.f,170/255.f,1), "COLOR PICKER");
        ImGui::PopFont();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);

        float sq_h = 160.f;
        ImVec2 sq0 = ImGui::GetCursorScreenPos();
        ImVec2 sq1 = sq0 + ImVec2(w, sq_h);
        ImU32 hue32 = ImColor::HSV(H, 1.f, 1.f);
        dl->AddRectFilledMultiColor(sq0, sq1, IM_COL32(255,255,255,255), hue32, IM_COL32(0,0,0,255), IM_COL32(0,0,0,255));
        dl->AddRect(sq0, sq1, IM_COL32(255,255,255,15), 6.f, ImDrawFlags_RoundCornersAll, 1.f);

        float cx = sq0.x + S * w;
        float cy = sq0.y + (1.f - V) * sq_h;
        dl->AddCircleFilled(ImVec2(cx, cy), 6.f, IM_COL32(255,255,255,255));
        dl->AddCircle(ImVec2(cx, cy), 6.f, IM_COL32(0,0,0,80), 0, 2.f);

        ImGui::SetCursorScreenPos(sq0);
        ImGui::InvisibleButton("##sv_sq", ImVec2(w, sq_h));
        if (ImGui::IsItemActive()) {
            ImVec2 mp = io.MousePos;
            S = ImClamp((mp.x - sq0.x) / w, 0.f, 1.f);
            V = ImClamp(1.f - (mp.y - sq0.y) / sq_h, 0.f, 1.f);
            ImGui::ColorConvertHSVtoRGB(H, S, V, color[0], color[1], color[2]);
            send_col();
        }

        ImGui::SetCursorScreenPos(ImVec2(sq0.x, sq1.y + 12.f));

        float bar_h = 12.f;
        ImVec2 hb0 = ImGui::GetCursorScreenPos();
        ImVec2 hb1 = hb0 + ImVec2(w, bar_h);
        for (int i = 0; i < (int)w; i++) {
            float t = (float)i / w;
            dl->AddRectFilled(ImVec2(hb0.x + i, hb0.y), ImVec2(hb0.x + i + 1, hb1.y), ImColor::HSV(t, 1.f, 1.f));
        }
        dl->AddRect(hb0, hb1, IM_COL32(255,255,255,15), 6.f, ImDrawFlags_RoundCornersAll, 1.f);

        float hx = hb0.x + H * w;
        dl->AddCircleFilled(ImVec2(hx, hb0.y + bar_h * 0.5f), 7.f, IM_COL32(255,255,255,255));
        dl->AddCircle(ImVec2(hx, hb0.y + bar_h * 0.5f), 7.f, IM_COL32(0,0,0,60), 0, 1.5f);

        ImGui::SetCursorScreenPos(hb0);
        ImGui::InvisibleButton("##hue", ImVec2(w, bar_h));
        if (ImGui::IsItemActive()) {
            H = ImClamp((io.MousePos.x - hb0.x) / w, 0.f, 1.f);
            ImGui::ColorConvertHSVtoRGB(H, S, V, color[0], color[1], color[2]);
            send_col();
        }

        ImGui::SetCursorScreenPos(ImVec2(hb0.x, hb1.y + 12.f));

        ImVec2 ab0 = ImGui::GetCursorScreenPos();
        ImVec2 ab1 = ab0 + ImVec2(w, bar_h);
        int chk = 4;
        for (int y = 0; y < (int)bar_h; y += chk)
            for (int x = 0; x < (int)w; x += chk) {
                ImU32 ck = ((x/chk) + (y/chk)) % 2 ? IM_COL32(204,204,204,255) : IM_COL32(255,255,255,255);
                dl->AddRectFilled(ImVec2(ab0.x + x, ab0.y + y), ImVec2(ImMin(ab0.x + x + chk, ab1.x), ImMin(ab0.y + y + chk, ab1.y)), ck);
            }
        ImU32 full_c = IM_COL32((int)(color[0]*255), (int)(color[1]*255), (int)(color[2]*255), 255);
        dl->AddRectFilledMultiColor(ab0, ab1, IM_COL32(0,0,0,0), full_c, full_c, IM_COL32(0,0,0,0));
        dl->AddRect(ab0, ab1, IM_COL32(255,255,255,15), 6.f, ImDrawFlags_RoundCornersAll, 1.f);

        float ax = ab0.x + A * w;
        dl->AddCircleFilled(ImVec2(ax, ab0.y + bar_h * 0.5f), 7.f, IM_COL32(255,255,255,255));
        dl->AddCircle(ImVec2(ax, ab0.y + bar_h * 0.5f), 7.f, IM_COL32(0,0,0,60), 0, 1.5f);

        ImGui::SetCursorScreenPos(ab0);
        ImGui::InvisibleButton("##alpha", ImVec2(w, bar_h));
        if (ImGui::IsItemActive()) {
            A = ImClamp((io.MousePos.x - ab0.x) / w, 0.f, 1.f);
            color[3] = A;
            send_col();
        }

        ImGui::SetCursorScreenPos(ImVec2(ab0.x, ab1.y + 14.f));

        int ri = (int)(color[0]*255+0.5f), gi = (int)(color[1]*255+0.5f), bi = (int)(color[2]*255+0.5f);
        ImU32 preview_fill = IM_COL32(ri, gi, bi, (int)(A*255));

        ImVec2 pv0 = ImGui::GetCursorScreenPos();
        ImVec2 pv1 = pv0 + ImVec2(32, 32);
        for (int y = 0; y < 32; y += 3)
            for (int x = 0; x < 32; x += 3) {
                ImU32 ck = ((x/3) + (y/3)) % 2 ? IM_COL32(204,204,204,255) : IM_COL32(255,255,255,255);
                dl->AddRectFilled(ImVec2(pv0.x + x, pv0.y + y), ImVec2(ImMin(pv0.x + x + 3, pv1.x), ImMin(pv0.y + y + 3, pv1.y)), ck);
            }
        dl->AddRectFilled(pv0, pv1, preview_fill);
        dl->AddRect(pv0, pv1, IM_COL32(44,44,53,255), 4.f, ImDrawFlags_RoundCornersAll, 1.f);

        ImGui::SetCursorScreenPos(ImVec2(pv1.x + 10, pv0.y - 1));
        char hex[8];
        snprintf(hex, sizeof(hex), "%02X%02X%02X", ri, gi, bi);
        ImGui::PushFont(Montserrat_7);
        ImGui::TextColored(ImVec4(161/255.f,161/255.f,170/255.f,1), "HEX");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(228/255.f,228/255.f,231/255.f,1), "#%s", hex);

        ImGui::SetCursorScreenPos(ImVec2(pv1.x + 10, pv0.y + 15));
        ImGui::TextColored(ImVec4(161/255.f,161/255.f,170/255.f,1), "RGBA");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(228/255.f,228/255.f,231/255.f,1), "%d, %d, %d, %.2f", ri, gi, bi, A);
        ImGui::PopFont();

        ImGui::EndPopup();
    }
    ImGui::PopID();

    return pressed;
}

struct space_state {
    float hover_progress = 0.f;
    float check_progress = 0.f;
    float star_pulse = 0.f;
    struct Star { ImVec2 pos; float base_size; float twinkle_offset; float orbit_angle; float orbit_radius; ImU32 color; };
    Star stars[30];
    ImVec2 trail[10];
    ImVec2 nebula_offset;
    bool initialized = false;
};

bool CheckboxSpace(const char* label, bool* v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    static std::map<ImGuiID, space_state> anim;
    auto& state = anim[id];
    if (!state.initialized) {
        unsigned int sd = id;
        for (int i = 0; i < 30; i++) {
            float r1 = (float)((sd + i * 7) % 997) / 997.f;
            float r2 = (float)((sd + i * 13) % 991) / 991.f;
            float r3 = (float)((sd + i * 19) % 983) / 983.f;
            state.stars[i].base_size = 0.5f + r1 * 2.f;
            state.stars[i].twinkle_offset = r2 * 6.2832f;
            state.stars[i].orbit_angle = r3 * 6.2832f;
            state.stars[i].orbit_radius = 20.f + r1 * 60.f;
            state.stars[i].color = IM_COL32(200 + (int)(r1 * 55.f), 220 + (int)(r2 * 35.f), 240 + (int)(r3 * 15.f), 255);
        }
        state.initialized = true;
    }
    const ImVec2 pos = window->DC.CursorPos;
    const ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x - 6.f, 60);
    const ImRect total_bb(pos, pos + size);
    ImGui::ItemSize(total_bb, g.Style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id)) return false;
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) { *v = !(*v); ImGui::MarkItemEdited(id); }
    const float speed = g.IO.DeltaTime * 8.f;
    const float time = g.Time;
    state.hover_progress = ImLerp(state.hover_progress, hovered ? 1.f : 0.f, speed);
    state.check_progress = ImLerp(state.check_progress, *v ? 1.f : 0.f, speed);
    state.star_pulse = 1.f + sinf(time * 3.f) * 0.1f * state.check_progress;
    float hover_eased = 1.f - powf(1.f - state.hover_progress, 3.f);
    float check_eased = 1.f - powf(1.f - state.check_progress, 2.f);
    ImVec2 mouse_rel = (g.IO.MousePos - total_bb.GetCenter()) / 200.f;
    state.nebula_offset = ImLerp(state.nebula_offset, mouse_rel * 15.f * hover_eased, speed);
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    // 1. Nebula background
    for (int i = 5; i >= 0; i--) {
        float t = i / 5.f;
        ImVec4 lc = ImLerp(ImVec4(5/255.f,5/255.f,15/255.f,1.f), ImLerp(ImVec4(20/255.f,10/255.f,40/255.f,1.f), ImVec4(40/255.f,20/255.f,80/255.f,1.f), hover_eased * 0.5f), t * 0.5f);
        float ex = t * 20.f * hover_eased;
        draw->AddRectFilled(total_bb.Min - ImVec2(ex, ex) + state.nebula_offset * t, total_bb.Max + ImVec2(ex, ex) + state.nebula_offset * t, ImGui::GetColorU32(lc), 12.f);
    }
    // 2. Background stars
    unsigned int sd = id;
    for (int i = 0; i < 20; i++) {
        ImVec2 sp = total_bb.Min + ImVec2((float)((sd * 31 + i * 131) % (int)ImMax(size.x, 1.f)), (float)((sd * 37 + i * 157) % (int)ImMax(size.y, 1.f)));
        float tw = 0.5f + 0.5f * sinf(time * 2.f + i);
        draw->AddCircleFilled(sp, 1.f * tw, IM_COL32(255,255,255,150));
    }
    // 3. Orbiting stars
    ImVec2 center = total_bb.GetCenter() + ImVec2(size.x * 0.35f, 0);
    for (int i = 0; i < 30; i++) {
        auto& star = state.stars[i];
        float tr = hover_eased > 0.01f ? (40.f + (i % 3) * 15.f) : star.orbit_radius;
        star.orbit_radius = ImLerp(star.orbit_radius, tr, speed * 0.5f);
        float os = (0.3f + check_eased * 0.7f) * (hover_eased > 0.01f ? 2.f : 1.f);
        star.orbit_angle += g.IO.DeltaTime * os * (i % 2 == 0 ? 1 : -1);
        float ang = star.orbit_angle + hover_eased * i * 0.2f;
        ImVec2 op = center + ImVec2(cosf(ang) * star.orbit_radius, sinf(ang) * star.orbit_radius * 0.6f);
        star.pos = ImLerp(star.pos, op, speed * 2.f);
        float tw = 0.7f + 0.3f * sinf(time * 3.f + star.twinkle_offset);
        float sm = 1.f + hover_eased * 0.5f;
        ImVec4 sc_a = ImLerp(ImVec4(1,1,1,200*tw/255.f), ImVec4(100/255.f,200/255.f,1,255*tw/255.f), check_eased);
        ImU32 sc = ImGui::GetColorU32(sc_a);
        float cr = star.base_size * 3.f * sm;
        draw->AddCircleFilled(star.pos, cr, IM_COL32(255,255,255,30));
        draw->AddCircleFilled(star.pos, star.base_size * sm, sc);
    }
    // 4. Shooting star
    if (check_eased > 0.01f) {
        ImVec2 ss(total_bb.Min.x, total_bb.GetCenter().y - 10);
        ImVec2 se(total_bb.Max.x, total_bb.GetCenter().y + 10);
        float t = fmodf(time * 0.5f + 0.3f, 1.f);
        ImVec2 sp = ImLerp(ss, se, t);
        for (int j = 9; j > 0; j--) state.trail[j] = state.trail[j-1];
        state.trail[0] = sp;
        for (int j = 0; j < 9; j++) {
            float a = (1.f - j / 9.f) * check_eased;
            draw->AddLine(state.trail[j], state.trail[j+1], IM_COL32(255,255,200,(int)(255*a)), (9-j)*0.5f);
        }
        draw->AddCircleFilled(sp, 3.f, IM_COL32(255,255,255,255));
        draw->AddShadowCircle(sp, 3.f, IM_COL32(255,255,200,200), 20.f, ImVec2(0,0), 0, 10);
    }
    // 5. Sun toggle
    float sun_r = 18.f * state.star_pulse;
    ImVec2 sun_pos = center;
    ImVec4 glow_a(1.f, 200/255.f, 100/255.f, 30/255.f);
    ImVec4 glow_b(100/255.f, 200/255.f, 1.f, 80/255.f);
    ImVec4 si_a(80/255.f, 60/255.f, 40/255.f, 1.f);
    ImVec4 si_b(1.f, 240/255.f, 200/255.f, 1.f);
    ImVec4 so_a(40/255.f, 30/255.f, 20/255.f, 1.f);
    ImVec4 so_b(1.f, 180/255.f, 80/255.f, 1.f);
    for (int i = (int)(3 + check_eased * 3); i >= 0; i--) {
        float t = i / (3.f + check_eased * 3.f);
        draw->AddCircleFilled(sun_pos, sun_r * (1.f + t * 2.f), ImGui::GetColorU32(ImLerp(glow_a, glow_b, check_eased)));
    }
    draw->AddCircleFilled(sun_pos, sun_r, ImGui::GetColorU32(ImLerp(si_a, si_b, check_eased)));
    draw->AddCircle(sun_pos, sun_r, ImGui::GetColorU32(ImLerp(so_a, so_b, check_eased)), 32, 2.f);
    // 6. Label
    ImVec2 tp(total_bb.Min.x + 15, total_bb.GetCenter().y - ImGui::CalcTextSize(label).y * 0.5f);
    if (hover_eased > 0.01f)
        draw->AddText(tp + ImVec2(0,-1), IM_COL32(100,200,255,(int)(100*hover_eased)), label);
    draw->AddText(tp, IM_COL32(255,255,255,255), label);
    // 7. Hover rim
    if (hover_eased > 0.01f)
        draw->AddRect(total_bb.Min, total_bb.Max, IM_COL32(100,200,255,(int)(100*hover_eased)), 12.f, ImDrawFlags_RoundCornersAll, 2.f);
    return pressed;
}

} // namespace edited (Button below)

namespace edited {

// ── Button: same visual style as Checkbox (hover lift, glow, particles, accent bar) ──
bool Button(const char* label, ImVec2 size_arg)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const float ch = (size_arg.y > 0.f) ? size_arg.y : 38.f;
    const float cw = (size_arg.x > 0.f) ? size_arg.x : (ImGui::GetContentRegionAvail().x - 6.f);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(cw, ch));
    static std::map<ImGuiID, check_state> anim;
    auto& state = anim[id];

    ImGui::ItemSize(ImVec2(cw, ch), -1.f);
    if (!ImGui::ItemAdd(rect, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

    state.hover_t  = ImLerp(state.hover_t,  hovered ? 1.f : 0.f, g.IO.DeltaTime * 10.f);
    state.press_t  = ImLerp(state.press_t,  held    ? 1.f : 0.f, g.IO.DeltaTime * 18.f);
    state.elevation= ImLerp(state.elevation, hovered ? 10.f : 0.f, g.IO.DeltaTime * 12.f);
    state.row_scale= ImLerp(state.row_scale, hovered ? 1.04f : 1.f, g.IO.DeltaTime * 10.f);
    state.alpha    = ImLerp(state.alpha, pressed ? 1.f : (hovered ? 0.6f : 0.f), g.IO.DeltaTime * 8.f);

    {
        ImVec2 mouse = g.IO.MousePos;
        ImVec2 rc2 = rect.GetCenter();
        float rx2 = ImClamp((mouse.x - rc2.x) / (cw * 0.5f), -1.f, 1.f);
        float ry2 = ImClamp((mouse.y - rc2.y) / (ch * 0.5f), -1.f, 1.f);
        state.rot_x = ImLerp(state.rot_x, hovered ? ry2 * 0.3f : 0.f, g.IO.DeltaTime * 12.f);
        state.rot_y = ImLerp(state.rot_y, hovered ? rx2 * 0.3f : 0.f, g.IO.DeltaTime * 12.f);
    }

    float elevation = state.elevation + state.press_t * 4.f;
    float row_s     = state.row_scale * (1.f - state.press_t * 0.06f);
    float rdx       = state.rot_x;
    float rdy       = state.rot_y - elevation;

    ImVec2 rc2 = rect.GetCenter();
    ImVec2 rh  = ImVec2(cw * 0.5f, ch * 0.5f) * row_s;
    ImVec2 rmin = rc2 - rh + ImVec2(rdx, rdy);
    ImVec2 rmax = rc2 + rh + ImVec2(rdx, rdy);

    bool hover_pop = state.hover_t > 0.01f;
    ImDrawList* dl = hover_pop ? ImGui::GetForegroundDrawList() : window->DrawList;

    if (state.hover_t > 0.01f) {
        float shad_a   = ImLerp(0.f, 40.f, state.hover_t);
        float shad_thk = ImLerp(0.f, 22.f, state.hover_t);
        float shad_off = ImLerp(0.f, 8.f,  state.hover_t);
        dl->AddShadowRect(rmin, rmax, IM_COL32(0,0,0,(int)shad_a), shad_thk, ImVec2(0, shad_off), 0, 8.f);
        float aura_a = ImLerp(0.f, 12.f, state.hover_t);
        dl->AddShadowRect(rmin, rmax, AccentCol(aura_a / 255.f), 30.f, ImVec2(0, 0), 8.f);
    }

    // Background
    ImVec4 btn_bg = ImLerp(ImVec4(0.08f,0.08f,0.11f,1), ImVec4(accent_col.x*0.2f, accent_col.y*0.2f, accent_col.z*0.2f+0.05f, 1), state.hover_t * 0.5f);
    dl->AddRectFilled(rmin, rmax, ImGui::GetColorU32(btn_bg), 8.f);

    // Top shine
    if (state.hover_t > 0.01f) {
        float hl_a = ImLerp(0.f, 30.f, state.hover_t);
        dl->AddRectFilled(rmin, ImVec2(rmax.x, rmin.y + 18.f * row_s), IM_COL32(255,255,255,(int)hl_a), 8.f, ImDrawFlags_RoundCornersTop);
    }

    // Border
    ImVec4 btn_border = ImLerp(ImVec4(0.18f,0.18f,0.22f,1), ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.85f), state.hover_t);
    dl->AddRect(rmin, rmax, ImGui::GetColorU32(btn_border), 8.f, 0, ImLerp(0.6f, 1.2f, state.hover_t));



    // Particles when hovered
    {
        ImVec2 cp = pos + ImVec2(cw * 0.5f, ch * 0.5f) + ImVec2(rdx, rdy);
        if (state.hover_t > 0.01f) {
            for (int p = 0; p < 30; p++) {
                if (state.part_alpha[p] <= 0.f) {
                    state.part_pos[p]   = cp + ImVec2((rand()%200-100)*0.15f, (rand()%200-100)*0.15f);
                    state.part_vel[p]   = ImVec2((rand()%200-100)*0.25f, (rand()%200-100)*0.25f);
                    state.part_rad[p]   = 0.8f + (rand()%20)*0.1f;
                    state.part_alpha[p] = 0.2f + (rand()%60)*0.01f;
                }
                state.part_pos[p]   += state.part_vel[p] * g.IO.DeltaTime;
                state.part_alpha[p] -= g.IO.DeltaTime * 0.8f;
                float pa = ImMin(state.part_alpha[p], state.hover_t * 0.6f);
                if (pa > 0.01f)
                    dl->AddCircleFilled(state.part_pos[p], state.part_rad[p], AccentCol(pa * 0.35f), 12);
            }
        } else {
            for (int p = 0; p < 30; p++) state.part_alpha[p] = 0.f;
        }
    }

    // Label
    ImGui::PushFont(Montserrat_1);
    ImVec2 tsz = ImGui::CalcTextSize(label);
    ImVec2 lbl_pos = ImVec2(rc2.x - tsz.x * 0.5f, rc2.y - tsz.y * 0.5f) + ImVec2(rdx, rdy);
    ImVec4 lbl_base = ImLerp(ImVec4(180/255.f,182/255.f,195/255.f,1), ImVec4(240/255.f,242/255.f,255/255.f,1), state.hover_t);
    if (state.hover_t > 0.01f)
        dl->AddText(lbl_pos + ImVec2(0, 0.5f), IM_COL32(0,0,0,(int)ImLerp(0.f,55.f,state.hover_t)), label);
    dl->AddText(lbl_pos, ImGui::GetColorU32(lbl_base), label);
    ImGui::PopFont();

    return pressed;
}

} // namespace edited




//static void LoadAccentFromConfig()
//{
//    std::ifstream f(GetConfigPath());
//    if (!f.is_open()) return;
//    std::string line;
//    while (std::getline(f, line))
//    {
//        auto eq = line.find('=');
//        if (eq == std::string::npos) continue;
//        std::string key = line.substr(0, eq);
//        std::string val = line.substr(eq + 1);
//        if (key == "accent_r") accent_col.x = ImClamp(std::stoi(val) / 255.f, 0.f, 1.f);
//        if (key == "accent_g") accent_col.y = ImClamp(std::stoi(val) / 255.f, 0.f, 1.f);
//        if (key == "accent_b") accent_col.z = ImClamp(std::stoi(val) / 255.f, 0.f, 1.f);
//    }
//}

// ── AddNotification (used by BypassManager & main UI) ────────────────
void AddNotification(const std::string& message, const std::string& type, float displayTime)
{
    ImColor color = (type == "success") ? ImColor(0,220,100) :
                    (type == "warning")  ? ImColor(255,200,0) :
                                           ImColor(255,70,70);
    notificationSystem.AddNotification("Info", message, color);
}

// ── Chams Fix for Pie 64 ──────────────────────────────────────
static bool DownloadFile(const char* url, const char* path)
{
    HRESULT hr = URLDownloadToFileA(nullptr, url, path, 0, nullptr);
    return SUCCEEDED(hr);
}

static DWORD GetProcId(const char* name)
{
    DWORD id = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) do {
        if (_stricmp(pe.szExeFile, name) == 0) { id = pe.th32ProcessID; break; }
    } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    return id;
}

static bool InjectDLL(DWORD pid, const char* dllPath)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) return false;
    LPVOID mem = VirtualAllocEx(hProc, nullptr, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!mem) { CloseHandle(hProc); return false; }
    WriteProcessMemory(hProc, mem, dllPath, strlen(dllPath) + 1, nullptr);
    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, mem, 0, nullptr);
    if (!hThread) { VirtualFreeEx(hProc, mem, 0, MEM_RELEASE); CloseHandle(hProc); return false; }
    CloseHandle(hThread);
    CloseHandle(hProc);
    return true;
}


static HMODULE g_hBypassDll = nullptr;

static HMODULE LoadEmbeddedDll()
{
    HRSRC hRes = FindResourceA(NULL, MAKEINTRESOURCEA(IDR_BYPASS_DLL), (LPCSTR)RT_RCDATA);
    if (!hRes) hRes = FindResourceA(NULL, MAKEINTRESOURCEA(IDR_BYPASS_DLL), "RCDATA");
    if (!hRes) hRes = FindResourceA(NULL, "IDR_BYPASS_DLL", (LPCSTR)RT_RCDATA);
    if (!hRes) hRes = FindResourceA(NULL, "IDR_BYPASS_DLL", "RCDATA");
    if (!hRes) hRes = FindResourceA(NULL, "101", (LPCSTR)RT_RCDATA);
    if (!hRes) hRes = FindResourceA(NULL, "101", "RCDATA");

    if (!hRes) {
        return NULL;
    }

    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) {
        return NULL;
    }

    DWORD  dwSize = SizeofResource(NULL, hRes);
    LPVOID pData  = LockResource(hData);
    if (!pData || !dwSize) {
        return NULL;
    }

    char tempDir[MAX_PATH], dllPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);
    
    // User requested to keep the exact name for updates
    sprintf_s(dllPath, "%sUIDBypassDll.dll", tempDir);

    HANDLE hFile = CreateFileA(
        dllPath,
        GENERIC_WRITE,
        0, NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        // Fallback: Try writing it to the EXE's directory with the exact same name
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash) *(lastSlash + 1) = '\0';
        sprintf_s(dllPath, "%sUIDBypassDll.dll", exePath);
        
        hFile = CreateFileA(dllPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            MessageBoxA(NULL, "Failed to extract Bypass DLL (UIDBypassDll.dll) to Temp or Exe folder!\nPlease close any running instances, disable Anti-Virus or Run as Administrator.", "Extraction Error", MB_ICONERROR);
            return NULL;
        }
    }

    DWORD written;
    WriteFile(hFile, pData, dwSize, &written, NULL);
    CloseHandle(hFile);

    HMODULE hDll = LoadLibraryA(dllPath);
    if (!hDll) {
        MessageBoxA(NULL, "Extracted Bypass DLL successfully, but failed to load it!", "Load Error", MB_ICONERROR);
    }
    return hDll;
}

int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    g_hBypassDll = LoadEmbeddedDll();
    
    KeyAuthApp.init();
    WNDCLASSEXW wc;
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = NULL;
    wc.cbWndExtra = NULL;
    wc.hInstance = nullptr;
    wc.hIcon = nullptr;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = L"FINEX CORP!";
    wc.lpszClassName = L"FINEX CORP!";
    wc.hIconSm = nullptr;

    RegisterClassExW(&wc);
    hwnd = CreateWindowExW(NULL, wc.lpszClassName, L"FINEX CORP!", WS_POPUP, (GetSystemMetrics(SM_CXSCREEN) / 2) - (current_win_size.x / 2), (GetSystemMetrics(SM_CYSCREEN) / 2) - (current_win_size.y / 2), current_win_size.x, current_win_size.y, 0, 0, 0, 0);

    {
        HRGN rgn = CreateRoundRectRgn(0, 0, (int)current_win_size.x, (int)current_win_size.y, 10, 10);
        SetWindowRgn(hwnd, rgn, TRUE);
        DeleteObject(rgn);
    }

    SetWindowLongA(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    POINT mouse;
    rc = { 0 };
    GetWindowRect(hwnd, &rc);

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
    bool isClickable = true;
    ToggleClickability(isClickable);

    ShowWindow(GetConsoleWindow(), SW_HIDE);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_ChildBg]  = ImVec4(0.04f,0.04f,0.06f,1.f);
    style.Colors[ImGuiCol_FrameBg]  = ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_PopupBg]  = ImVec4(0.05f,0.05f,0.05f,1.f);
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    io.Fonts->AddFontFromMemoryTTF(&PoppinsRegular, sizeof PoppinsRegular, 22, NULL, io.Fonts->GetGlyphRangesCyrillic());

    inter_default = io.Fonts->AddFontFromMemoryTTF(inter_medium, sizeof(inter_medium), 17.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    poppinsreg = io.Fonts->AddFontFromMemoryTTF(&PoppinsRegular, sizeof PoppinsRegular, 22, NULL, io.Fonts->GetGlyphRangesCyrillic());
    nevan = io.Fonts->AddFontFromMemoryTTF(&Nevan, sizeof Nevan, 55, NULL, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* nevanM = io.Fonts->AddFontFromMemoryTTF(&Nevan, sizeof Nevan, 75, NULL, io.Fonts->GetGlyphRangesCyrillic());
    icon_font2 = io.Fonts->AddFontFromMemoryTTF(&icomoon2, sizeof icomoon2, 35, NULL, io.Fonts->GetGlyphRangesCyrillic());
    icon_font = io.Fonts->AddFontFromMemoryTTF(&icomoon, sizeof icomoon, 20, NULL, io.Fonts->GetGlyphRangesCyrillic());
    icon_font3 = io.Fonts->AddFontFromMemoryTTF(&icomoon3, sizeof icomoon3, 28, NULL, io.Fonts->GetGlyphRangesCyrillic());
    second_font = io.Fonts->AddFontFromMemoryTTF(&PoppinsRegular, sizeof PoppinsRegular, 18, NULL, io.Fonts->GetGlyphRangesCyrillic());
    Montserrat = io.Fonts->AddFontFromMemoryTTF(&Main_Font, sizeof Main_Font, 20.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    Montserrat_1 = io.Fonts->AddFontFromMemoryTTF(&Main_Font, sizeof Main_Font, 14.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    Montserrat_6 = io.Fonts->AddFontFromMemoryTTF(&Main_Font, sizeof Main_Font, 13.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    Montserrat_7 = io.Fonts->AddFontFromMemoryTTF(&Main_Font, sizeof Main_Font, 12.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    if (widget_ico == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, widget_icon, sizeof(widget_icon), &info, pump, &widget_ico, 0);
    if (combo == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, combo_icon, sizeof(combo_icon), &info, pump, &combo, 0);
    if (keybind_ico == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, keybind_icon, sizeof(keybind_icon), &info, pump, &keybind_ico, 0);


    ImFont* poppinsreg_small = io.Fonts->AddFontFromMemoryTTF(&PoppinsRegular, sizeof PoppinsRegular, 24, NULL, io.Fonts->GetGlyphRangesCyrillic());

    //LoadAccentFromConfig();
   
    LoadRememberMe();

    // Start emulator detection in background
    certMgr.StartEmulatorMonitoring();

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_SwapChainOccluded &&
            g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth && g_ResizeHeight)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }


        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();


        static int opacity = 255;
        static bool hide = true;

        if (GetAsyncKeyState(VK_INSERT) & 1)
        {
            isClickable = !isClickable;
            hide = !hide;
            ToggleClickability(isClickable);
        }

		if (GetAsyncKeyState(VK_DELETE) & 1)
		{
            ExitProcess(0);
		}

        opacity = ImLerp(opacity, opacity <= 255 && hide ? 300 : 0, ImGui::GetIO().DeltaTime * 8.f);
        if (opacity > 255) opacity = 255;
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), opacity, LWA_ALPHA);


        ImGui::NewFrame();
        {

            ImGui::SetNextWindowPos(ImVec2(0, 0));

            ImGui::SetNextWindowSize(current_win_size);

            ImGui::GetStyle().AntiAliasedLines = true;
            ImGui::GetStyle().AntiAliasedFill = true;

            ImGui::Begin("Bypass", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
            {
                main_window = ImGui::GetCurrentWindow();
                auto draw = ImGui::GetWindowDrawList();
                draw->Flags |= ImDrawListFlags_AntiAliasedLines;
                draw->Flags |= ImDrawListFlags_AntiAliasedFill;
                const auto& pos = ImGui::GetWindowPos();

                section_tint = accent_col;
                scheme_text = ImLerp(accent_col, ImVec4(1,1,1,1), 0.5f);




                // â”€â”€ esp_l: Visual Options panel with backend-connected checkboxes â”€â”€
                // ── Login State Machine ──
                if (authenticating && auth_done) {
                    authenticating = false; auth_done = false;
                    if (auth_result) {
                        if (remember_me) SaveRememberMe();
                        notificationSystem.AddNotification("Success", "Logged in successfully!", ImColor(0, 255, 100));
                        login_phase = SUCCESS;
                    } else {
                        auth_error_msg = auth_error_msg.empty() ? "Authentication failed." : auth_error_msg;
                        notificationSystem.AddNotification("Error", auth_error_msg.c_str(), ImColor(255, 80, 80));
                    }
                }
                auto easeOutCubic = [](float t) { float x = 1.0f - t; return 1.0f - x * x * x; };
                auto easeInCubic = [](float t) { return t * t * t; };
                if (login_phase == SUCCESS) {
                    login_alpha = ImLerp(login_alpha, 0.f, 8.f * ImGui::GetIO().DeltaTime);
                    if (login_alpha < 0.01f) { login_phase = RESIZING; resize_progress = 0.f; }
                }
                if (login_phase == RESIZING) {
                    resize_progress = ImLerp(resize_progress, 1.f, 4.5f * ImGui::GetIO().DeltaTime);
                    float ep = easeOutCubic(ImMin(resize_progress, 1.f));
                    current_win_size = ImVec2(login_size.x + (menu_size.x - login_size.x) * ep, login_size.y + (menu_size.y - login_size.y) * ep);
                    if (resize_progress > 0.999f) { resize_progress = 1.f; current_win_size = menu_size; login_phase = MAIN; }
                    SetWindowPos(hwnd, 0, (GetSystemMetrics(SM_CXSCREEN) - (int)current_win_size.x) / 2, (GetSystemMetrics(SM_CYSCREEN) - (int)current_win_size.y) / 2, (int)current_win_size.x, (int)current_win_size.y, SWP_NOZORDER | SWP_NOACTIVATE);
                    { HRGN rgn = CreateRoundRectRgn(0, 0, (int)current_win_size.x, (int)current_win_size.y, 10, 10); SetWindowRgn(hwnd, rgn, TRUE); DeleteObject(rgn); }
                }
                if (login_phase == LOGGING_OUT) {
                    resize_progress = ImLerp(resize_progress, 0.f, 4.5f * ImGui::GetIO().DeltaTime);
                    float ep = easeInCubic(ImClamp(resize_progress, 0.f, 1.f));
                    current_win_size = ImVec2(login_size.x + (menu_size.x - login_size.x) * ep, login_size.y + (menu_size.y - login_size.y) * ep);
                    if (resize_progress < 0.001f) { resize_progress = 0.f; current_win_size = login_size; login_alpha = 1.f; login_phase = LOGIN; }
                    SetWindowPos(hwnd, 0, (GetSystemMetrics(SM_CXSCREEN) - (int)current_win_size.x) / 2, (GetSystemMetrics(SM_CYSCREEN) - (int)current_win_size.y) / 2, (int)current_win_size.x, (int)current_win_size.y, SWP_NOZORDER | SWP_NOACTIVATE);
                    { HRGN rgn = CreateRoundRectRgn(0, 0, (int)current_win_size.x, (int)current_win_size.y, 10, 10); SetWindowRgn(hwnd, rgn, TRUE); DeleteObject(rgn); }
                }

                // -- LOGIN PAGE - FINEX CORP! DrawList Design --
                if (login_phase == LOGIN || login_phase == SUCCESS)
                {
                    float card_a = login_alpha;
                    int   a2     = (int)(card_a * 255.f);
                    const float CW = current_win_size.x, CH = current_win_size.y;
                    float cx2 = 0.f;
                    float cy2 = 0.f;
                    ImVec2 card_min = pos + ImVec2(cx2, cy2);
                    ImVec2 card_max = card_min + ImVec2(CW, CH);
                    // Scanlines animation
                    static float scan_offset = 0.f; scan_offset += ImGui::GetIO().DeltaTime * 30.f;
                    for (float sy = card_min.y; sy < card_max.y; sy += 4.f) {
                        float soff = fmodf(sy+scan_offset, CH);
                        float sa = (soff < CH*0.15f) ? (1.f - soff/(CH*0.15f)) : 0.f;
                        if (sa > 0.02f) draw->AddRectFilled(ImVec2(card_min.x,sy), ImVec2(card_max.x,sy+1.f), IM_COL32(89,195,255,(int)(12*sa*card_a)));
                    }
                    // Card background
                    draw->AddRectFilled(card_min, card_max, IM_COL32(8,10,16,(int)(240*card_a)), 12.f);
                    draw->AddRectFilled(card_min, ImVec2(card_max.x,card_min.y+80.f), IM_COL32(20,35,55,(int)(60*card_a)), 12.f, ImDrawFlags_RoundCornersTop);
                    // Animated accent border
                    static float border_pulse = 0.f; border_pulse += ImGui::GetIO().DeltaTime * 1.8f;
                    float bpulse = (sinf(border_pulse)*0.5f+0.5f);
                    int ba_r=(int)(accent_col.x*255.f), ba_g=(int)(accent_col.y*255.f), ba_b=(int)(accent_col.z*255.f);
                    draw->AddRect(card_min, card_max, IM_COL32(ba_r,ba_g,ba_b,(int)(ImLerp(40.f,90.f,bpulse)*card_a)), 12.f, 0, 1.2f);
                    draw->AddRect(card_min+ImVec2(1,1), card_max-ImVec2(1,1), IM_COL32(ba_r,ba_g,ba_b,(int)(18*card_a)), 11.f, 0, 0.5f);
                    // Corner tech brackets
                    float clen=14.f, cthk=1.5f; ImU32 ccol=IM_COL32(ba_r,ba_g,ba_b,(int)(200*card_a));
                    draw->AddLine(card_min, card_min+ImVec2(clen,0), ccol, cthk);
                    draw->AddLine(card_min, card_min+ImVec2(0,clen), ccol, cthk);
                    draw->AddLine(ImVec2(card_max.x,card_min.y), ImVec2(card_max.x-clen,card_min.y), ccol, cthk);
                    draw->AddLine(ImVec2(card_max.x,card_min.y), ImVec2(card_max.x,card_min.y+clen), ccol, cthk);
                    draw->AddLine(ImVec2(card_min.x,card_max.y), ImVec2(card_min.x+clen,card_max.y), ccol, cthk);
                    draw->AddLine(ImVec2(card_min.x,card_max.y), ImVec2(card_min.x,card_max.y-clen), ccol, cthk);
                    draw->AddLine(card_max, card_max-ImVec2(clen,0), ccol, cthk);
                    draw->AddLine(card_max, card_max-ImVec2(0,clen), ccol, cthk);
                    // Title FINEX CORP!
                    ImGui::PushFont(Montserrat);
                    ImVec2 tsz2=ImGui::CalcTextSize("FINEX CORP!"); ImVec2 tpos2=ImVec2(card_min.x+(CW-tsz2.x)*0.5f,card_min.y+16.f);
                    draw->AddText(tpos2+ImVec2(1,1), IM_COL32(ba_r,ba_g,ba_b,(int)(120*card_a)), "FINEX CORP!");
                    draw->AddText(tpos2, IM_COL32(235,240,255,a2), "FINEX CORP!"); ImGui::PopFont();
                    ImGui::PushFont(Montserrat_7); const char* sub2="UID BYPASS  |  PREMIUM"; ImVec2 ssz2=ImGui::CalcTextSize(sub2);
                    draw->AddText(ImVec2(card_min.x+(CW-ssz2.x)*0.5f,card_min.y+38.f), IM_COL32(ba_r,ba_g,ba_b,(int)(160*card_a)), sub2); ImGui::PopFont();
                    draw->AddRectFilled(ImVec2(card_min.x+20,card_min.y+56.f), ImVec2(card_max.x-20,card_min.y+57.f), IM_COL32(ba_r,ba_g,ba_b,(int)(60*card_a)));
                    // Matrix decode cyber text
                    static float dec_t=0.f; static int dec_i=0;
                    const char* ctexts[]={"FINEX CORP! BYPASS","100% SAFE MAIN ID","ADVANCED ANTI-BAN","UNDETECTED BYPASS","PREMIUM EXPERIENCE"};
                    dec_t+=ImGui::GetIO().DeltaTime; if(dec_t>3.f){dec_t=0.f;dec_i=(dec_i+1)%5;}
                    std::string cy_t=ctexts[dec_i]; std::string cy_d=cy_t;
                    int lk=(int)((dec_t/1.5f)*cy_t.length()); if(lk>(int)cy_t.length())lk=(int)cy_t.length();
                    const char cy_cs[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&"; int cy_cl=(int)sizeof(cy_cs)-1;
                    for(int ci=lk;ci<(int)cy_t.length();++ci){if(cy_t[ci]==' ')continue;cy_d[ci]=cy_cs[(int)(ImGui::GetTime()*40.f+ci*23)%cy_cl];}
                    ImGui::PushFont(Montserrat_6); ImVec2 ddsz=ImGui::CalcTextSize(cy_d.c_str());
                    draw->AddText(ImVec2(card_min.x+(CW-ddsz.x)*0.5f,card_min.y+66.f), IM_COL32(ba_r,ba_g,ba_b,(int)(200*card_a)), cy_d.c_str()); ImGui::PopFont();
                    // Glass InputText: License Key
                    float fx=card_min.x+20.f, fy=card_min.y+100.f, fw=CW-40.f, fh=42.f;
                    ImGui::PushFont(Montserrat_7); draw->AddText(ImVec2(fx,fy-16.f), IM_COL32(ba_r,ba_g,ba_b,(int)(200*card_a)), "LICENSE KEY"); ImGui::PopFont();
                    static float fht=0.f; bool fhov=ImGui::IsMouseHoveringRect(ImVec2(fx,fy),ImVec2(fx+fw,fy+fh));
                    fht=ImLerp(fht,fhov?1.f:0.f,ImGui::GetIO().DeltaTime*12.f);
                    draw->AddRectFilled(ImVec2(fx,fy),ImVec2(fx+fw,fy+fh),IM_COL32(12,15,22,(int)(240*card_a)),8.f);
                    draw->AddRectFilled(ImVec2(fx,fy),ImVec2(fx+fw,fy+12.f),IM_COL32(255,255,255,(int)(6*card_a)),8.f,ImDrawFlags_RoundCornersTop);
                    draw->AddRect(ImVec2(fx,fy),ImVec2(fx+fw,fy+fh),IM_COL32(ba_r,ba_g,ba_b,(int)(ImLerp(35.f,120.f,fht)*card_a)),8.f,0,ImLerp(0.8f,1.4f,fht));
                    float ffw=fw*fht; if(ffw>0.1f) draw->AddRectFilled(ImVec2(fx+(fw-ffw)*0.5f,fy+fh-2.f),ImVec2(fx+(fw+ffw)*0.5f,fy+fh),AccentCol(card_a));
                    if(key_buf[0]==0){ImGui::PushFont(Montserrat_7);draw->AddText(ImVec2(fx+12.f,fy+(fh-12.f)*0.5f),IM_COL32(80,85,100,(int)(180*card_a)),"Enter your license key...");ImGui::PopFont();}
                    ImGui::SetCursorScreenPos(ImVec2(fx,fy));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0,0,0,0)); ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0,0,0,0));
                    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImVec4(0,0,0,0)); ImGui::PushStyleColor(ImGuiCol_Text,IM_COL32(210,215,230,a2));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,8.f); ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(12.f,(fh-ImGui::GetTextLineHeight())*0.5f));
                    ImGui::SetNextItemWidth(fw); ImGui::InputText("##lkey_fnx",key_buf,sizeof(key_buf));
                    ImGui::PopStyleColor(4); ImGui::PopStyleVar(2);
                    // Authenticate Button (checkbox style via edited::Button)
                    float by2=fy+fh+22.f; ImGui::SetCursorScreenPos(ImVec2(fx,by2));
                    if(authenticating){
                        static float aup=0.f; aup+=ImGui::GetIO().DeltaTime*3.f; float ap2=(sinf(aup)*0.5f+0.5f);
                        draw->AddRectFilled(ImVec2(fx,by2),ImVec2(fx+fw,by2+42.f),IM_COL32(8,12,20,(int)(220*card_a)),8.f);
                        draw->AddRect(ImVec2(fx,by2),ImVec2(fx+fw,by2+42.f),IM_COL32(ba_r,ba_g,ba_b,(int)((60+ap2*80)*card_a)),8.f);
                        ImGui::PushFont(Montserrat_1); ImVec2 asz=ImGui::CalcTextSize("Authenticating...");
                        draw->AddText(ImVec2(fx+(fw-asz.x)*0.5f,by2+(42.f-asz.y)*0.5f),IM_COL32(ba_r,ba_g,ba_b,(int)(180*card_a)),"Authenticating...");
                        ImGui::PopFont(); ImGui::Dummy(ImVec2(fw,42.f));
                    } else {
                        bool pa=edited::Button("  Authenticate  ",ImVec2(fw,42.f));
                        if(!pa) pa=ImGui::IsKeyPressed(ImGuiKey_Enter)||ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
                        if(pa){
                            if(strlen(key_buf)==0){notificationSystem.AddNotification("Error","Please enter a license key.",ImColor(255,80,80));}
                            else{authenticating=true;auth_done=false;std::string key=key_buf;std::thread([key](){try{ KeyAuthApp.license(key);auth_result= KeyAuthApp.response.success;if(!auth_result)auth_error_msg= KeyAuthApp.response.message;}catch(...){auth_result=false;auth_error_msg="Connection error.";}auth_done=true;}).detach();}
                        }
                    }
                    ImGui::SetCursorScreenPos(ImVec2(fx,by2+54.f)); ThemedToggle("Remember Me",&remember_me);
                    ImGui::PushFont(Montserrat_7); const char* vt2="v1.0  |  FINEX CORP! 2025"; ImVec2 vtsze=ImGui::CalcTextSize(vt2);
                    draw->AddText(ImVec2(card_min.x+(CW-vtsze.x)*0.5f,card_max.y-22.f),IM_COL32(ba_r,ba_g,ba_b,(int)(80*card_a)),vt2); ImGui::PopFont();

                    move_window(100.f);
                    ImGui::End();
                    notificationSystem.DrawNotifications();
                    ImGui::Render();
                    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
                    const float clr0[4] = { 0, 0, 0, 0 };
                    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clr0);
                    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                    g_pSwapChain->Present(1, 0);
                    continue;
                }

                // -- Main Menu (Bypass New style) --
                if (login_phase != MAIN && login_phase != LOGIN && login_phase != SUCCESS) {
                    move_window2();
                    ImGui::End();
                    notificationSystem.DrawNotifications();
                    ImGui::Render();
                    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
                    const float clr1[4] = { 0, 0, 0, 0 };
                    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clr1);
                    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                    g_pSwapChain->Present(1, 0);
                    continue;
                }

                // Window background (no visible black box, matches Bypass New)
                ImGui::PushClipRect(ImVec2(0, 0), ImVec2(4000, 4000), false);
                ImGui::GetBackgroundDrawList()->AddRectFilled(pos, pos + current_win_size, IM_COL32(14, 14, 18, 255), 10.f);
                ImGui::GetBackgroundDrawList()->AddRect(pos, pos + current_win_size, IM_COL32(180, 180, 180, 48), 10.f, 0, 1.0f);
                ImGui::PopClipRect();

                // Header title
                ImGui::PushFont(Montserrat);
                ImGui::SetCursorPos(ImVec2(14.f, 13.f));
                ImGui::TextColored(ImVec4(1, 1, 1, 1), "FINEX CORP!");
                ImGui::PopFont();

                // Minimize button
                ImGui::SetCursorPos(ImVec2(current_win_size.x - 45.f, 11.f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,1));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f,0.5f,0.5f,0.5f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f,0.6f,0.6f,0.6f));
                if (ImGui::Button("-", ImVec2(20.f, 20.f))) { std::thread([](){ ShowWindow(hwnd, SW_MINIMIZE); }).detach(); }
                ImGui::PopStyleColor(4);

                // Close button
                ImGui::SetCursorPos(ImVec2(current_win_size.x - 22.f, 11.f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,1));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f,0.1f,0.1f,0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f,0.1f,0.1f,1.f));
                if (ImGui::Button("X", ImVec2(20.f, 20.f))) {
                    int result = MessageBoxA(hwnd,
                        "If You Close The Bypass, The Bypass Will Turn Off And You Will Lose Your Network Connection.\n\nAre you sure you want to close?",
                        "Confirmation", MB_YESNO | MB_ICONWARNING | MB_TOPMOST);
                    if (result == IDYES) { PostQuitMessage(0); exit(0); }
                }
                ImGui::PopStyleColor(4);

                // Thin separator line
                draw->AddRectFilled(pos + ImVec2(0, 38.f), pos + ImVec2(current_win_size.x, 39.f), IM_COL32(180, 180, 180, 40));

                // Drag window
                move_window2();

                // Content area
                const float panelW = current_win_size.x - 20.f;
                const float panelH = current_win_size.y - 50.f;

                ImGui::SetCursorPos(ImVec2(10.f, 42.f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));
                ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 3.f);
                ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(80,80,90,180));
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(100,100,110,200));
                ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(120,120,130,255));
                if (ImGui::BeginChild("##maincontent2", ImVec2(panelW, panelH), false, ImGuiWindowFlags_NoBackground))
                {
                    ImGui::Dummy(ImVec2(0, 4));

                    // STATUS
                    ImGui::PushFont(Montserrat_6);
                    ImGui::TextColored(ImVec4(0.5f,0.5f,0.55f,1), "Status");
                    ImGui::PopFont();
                    ImGui::PushFont(Montserrat_7);
                    {
                        std::string emu_n = certMgr.GetEmulatorName();
                        std::string log_m = certMgr.GetLastError();
                        ImGui::TextColored(ImVec4(0.5f,0.5f,0.6f,1), "Emulator: ");
                        ImGui::SameLine();
                        if (emu_n == "None") ImGui::TextColored(ImVec4(1.f,0.3f,0.3f,1), "Not Detected");
                        else ImGui::TextColored(ImVec4(0.f,1.f,0.4f,1), "%s", emu_n.c_str());
                        if (!log_m.empty()) ImGui::TextColored(ImVec4(0.6f,0.6f,0.7f,1), "%s", log_m.substr(0,55).c_str());
                        
                        ImGui::TextColored(ImVec4(0.5f,0.5f,0.6f,1), "Version:  ");
                        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.7f,0.7f,0.8f,1), "%s", certMgr.GetEmulatorVersion().c_str());
                    }
                    ImGui::PopFont();

                    ImGui::Dummy(ImVec2(0, 6));
                    draw->AddRectFilled(ImGui::GetWindowPos() + ImVec2(0, ImGui::GetCursorPosY()),
                        ImGui::GetWindowPos() + ImVec2(panelW, ImGui::GetCursorPosY() + 1.f), IM_COL32(60,60,70,120));
                    ImGui::Dummy(ImVec2(0, 8));

                    // GAME SELECT & ADB PORT
                    static char adb_port2[16] = "5555";
                    static char proxy_port2[16] = "54233";
                    static int bypass_game_idx = 0;
                    const char* game_versions[] = { "FreeFire", "FreeFire MAX" };

                    // Auto-detect emulator and set ADB port
                    {
                        static bool emulator_found_notified = false;
                        std::string emu_n2 = certMgr.GetEmulatorName();
                        if (emu_n2 != "None") {
                            if (!emulator_found_notified) {
                                emulator_found_notified = true;
                                EmulatorType detectedType = certMgr.GetSelectedEmulator();
                                if (detectedType == EmulatorType::Memu) {
                                    strcpy_s(adb_port2, "21503");
                                    certMgr.Initialize("21503");
                                    notificationSystem.AddNotification("ADB", "MEmu Detected (Port: 21503)", ImColor(0, 255, 100));
                                } else if (detectedType == EmulatorType::BlueStacks5 || detectedType == EmulatorType::MSI5) {
                                    bool portSet = false;
                                    const char* confPaths[] = {
                                        "C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf",
                                        "C:\\ProgramData\\Bluestacks_msi5\\bluestacks.conf"
                                    };
                                    for (const char* path : confPaths) {
                                        std::ifstream bf(path);
                                        if (!bf.is_open()) continue;
                                        std::string line;
                                        while (std::getline(bf, line)) {
                                            auto p = line.find("status.adb_port");
                                            if (p != std::string::npos) {
                                                auto eq = line.find('"', p);
                                                if (eq != std::string::npos) {
                                                    auto end = line.find('"', eq + 1);
                                                    if (end != std::string::npos) {
                                                        std::string port = line.substr(eq + 1, end - eq - 1);
                                                        if (!port.empty() && port != "0") {
                                                            strcpy_s(adb_port2, port.c_str());
                                                            certMgr.Initialize(port);
                                                            notificationSystem.AddNotification("ADB", "Port auto-set: " + port, ImColor(0, 255, 100));
                                                            portSet = true;
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (portSet) break;
                                    }
                                    if (!portSet) {
                                        strcpy_s(adb_port2, "5555");
                                        certMgr.Initialize("5555");
                                    }
                                }
                            }
                        } else {
                            emulator_found_notified = false;
                        }
                    }
                    
                    ImGui::PushFont(Montserrat_6);
                    ImGui::TextColored(ImVec4(0.5f,0.5f,0.55f,1), "Configuration");
                    ImGui::PopFont();
                    ImGui::Dummy(ImVec2(0, 2));

                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
                    // Use menu theme color with low opacity for background
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.15f));
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.4f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(210/255.f, 215/255.f, 230/255.f, 1.f));
                    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.25f));
                    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.35f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, 9.f));

                    // Manual dropdown (BeginCombo crashes inside BeginChild)
                    {
                        static float combo_anim = 0.f;
                        static bool combo_open = false;
                        combo_anim = ImLerp(combo_anim, combo_open ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 12.f);

                        ImVec2 cpos = ImGui::GetCursorScreenPos();
                        float ch = 36.f;
                        ImVec2 cmin = cpos;
                        ImVec2 cmax = ImVec2(cpos.x + panelW, cpos.y + ch);
                        ImU32 bg_col   = ImGui::ColorConvertFloat4ToU32(ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.15f));
                        ImU32 brd_col  = ImGui::ColorConvertFloat4ToU32(ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.5f));
                        ImU32 txt_col  = IM_COL32(210, 215, 230, 255);
                        ImU32 arrow_col = ImGui::ColorConvertFloat4ToU32(ImVec4(accent_col.x, accent_col.y, accent_col.z, 1.f));

                        ImDrawList* ddl = ImGui::GetWindowDrawList();
                        ddl->AddRectFilled(cmin, cmax, bg_col, 6.f);
                        ddl->AddRect(cmin, cmax, brd_col, 6.f, 0, 1.f);

                        // Animated arrow rotation
                        float arrow_rot = combo_anim * 3.14159f; // 0 = down, PI = up
                        float ax = cmax.x - 18.f, ay = cpos.y + ch * 0.5f;
                        float cos_r = cosf(arrow_rot), sin_r = sinf(arrow_rot);
                        // Rotate triangle points around center
                        auto rot_pt = [&](float px, float py) -> ImVec2 {
                            float rx = (px - ax) * cos_r - (py - ay) * sin_r + ax;
                            float ry = (px - ax) * sin_r + (py - ay) * cos_r + ay;
                            return ImVec2(rx, ry);
                        };
                        ddl->AddTriangleFilled(
                            rot_pt(ax - 5.f, ay - 3.f),
                            rot_pt(ax + 5.f, ay - 3.f),
                            rot_pt(ax, ay + 4.f), arrow_col);

                        // Selected text
                        ImGui::PushFont(Montserrat_7);
                        ImVec2 tsz = ImGui::CalcTextSize(game_versions[bypass_game_idx]);
                        ddl->AddText(ImVec2(cpos.x + 10.f, cpos.y + (ch - tsz.y) * 0.5f), txt_col, game_versions[bypass_game_idx]);
                        ImGui::PopFont();

                        // Click to open
                        ImGui::SetCursorScreenPos(cmin);
                        if (ImGui::InvisibleButton("##combo_btn", ImVec2(panelW, ch)))
                            combo_open = !combo_open;

                        // Animated dropdown list
                        if (combo_anim > 0.01f) {
                            float ih = 32.f;
                            float total_h = 2 * ih * combo_anim;
                            // Clip to animated height
                            ddl->PushClipRect(cmin, ImVec2(cmax.x, cmax.y + total_h + 2.f), true);
                            for (int n = 0; n < 2; n++) {
                                ImVec2 imin = ImVec2(cmin.x, cmax.y + n * ih);
                                ImVec2 imax = ImVec2(cmax.x, cmax.y + (n + 1) * ih);
                                bool hov = combo_open && ImGui::IsMouseHoveringRect(imin, imax);
                                ImU32 item_bg = hov
                                    ? ImGui::ColorConvertFloat4ToU32(ImVec4(accent_col.x, accent_col.y, accent_col.z, 0.35f))
                                    : IM_COL32(18, 18, 24, 230);
                                ddl->AddRectFilled(imin, imax, item_bg, (n == 1) ? 6.f : 0.f, (n == 1) ? ImDrawFlags_RoundCornersBottom : ImDrawFlags_None);
                                ddl->AddRect(imin, imax, brd_col, 0.f, 0, 0.5f);
                                ImGui::PushFont(Montserrat_7);
                                ImVec2 itsz = ImGui::CalcTextSize(game_versions[n]);
                                ddl->AddText(ImVec2(imin.x + 10.f, imin.y + (ih - itsz.y) * 0.5f), (bypass_game_idx == n) ? arrow_col : txt_col, game_versions[n]);
                                ImGui::PopFont();
                                if (hov && ImGui::IsMouseClicked(0)) {
                                    bypass_game_idx = n;
                                    certMgr.SetGame(n == 0 ? GameType::FreeFire : GameType::FreeFireMAX);
                                    combo_open = false;
                                }
                            }
                            ddl->PopClipRect();
                            // Close on outside click
                            if (combo_open && ImGui::IsMouseClicked(0) && !ImGui::IsMouseHoveringRect(cmin, ImVec2(cmax.x, cmax.y + 2 * ih)))
                                combo_open = false;
                        }
                       //ImGui::Dummy(ImVec2(0, ch));
                    }
                    
                    ImGui::AlignTextToFramePadding();
                    ImGui::PushFont(Montserrat_6);
                    ImGui::TextColored(ImVec4(0.5f,0.5f,0.55f,1), "ADB Port:         ");
                    ImGui::PopFont();
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(120.f);
                    ImGui::InputText("##adb_port2", adb_port2, sizeof(adb_port2));
                    
                    ImGui::PopStyleVar(3);
                    ImGui::PopStyleColor(5);

                    ImGui::Dummy(ImVec2(0, 4));
                    draw->AddRectFilled(ImGui::GetWindowPos() + ImVec2(0, ImGui::GetCursorPosY()),
                        ImGui::GetWindowPos() + ImVec2(panelW, ImGui::GetCursorPosY() + 1.f), IM_COL32(60,60,70,120));
                    ImGui::Dummy(ImVec2(0, 4));

                    // BYPASS BUTTONS
                    ImGui::PushFont(Montserrat_6);
                    ImGui::TextColored(ImVec4(0.5f,0.5f,0.55f,1), "Bypass Controls");
                    ImGui::PopFont();
                    ImGui::Dummy(ImVec2(0, 4));
                    {
                        float bw = (panelW - 8.f) * 0.5f, bh = 34.f;
                        if (edited::Button("Install Cert", ImVec2(bw, bh))) {
                            std::string port(adb_port2);
                            std::thread([port]() {
                                notificationSystem.AddNotification("Install Cert", "Installing Certificate...", ImColor(90, 180, 255));
                                certMgr.Initialize(port);
                                if (certMgr.IsADBConnected()) certMgr.DisconnectADB();
                                if (!certMgr.ConnectADB()) {
                                    notificationSystem.AddNotification("Install Cert", "Failed to connect to ADB", ImColor(255, 80, 80));
                                    return;
                                }
                                if (certMgr.IsCertificateInstalled()) certMgr.UninstallCertificate();
                                if (certMgr.InstallCertificate())
                                    notificationSystem.AddNotification("Install Cert", "Certificate Installed Successfully", ImColor(0, 255, 100));
                                else
                                    notificationSystem.AddNotification("Install Cert", "Failed to install Certificate", ImColor(255, 80, 80));
                                certMgr.DisconnectADB();
                            }).detach();
                        }
                        
                        ImGui::SameLine(0, 8.f);
                        
                        if (edited::Button("Patch Emulator", ImVec2(bw, bh))) {
                            std::string port(adb_port2);
                            std::thread([port]() {
                                notificationSystem.AddNotification("Patch Emulator", "Patching Emulator...", ImColor(90, 180, 255));
                                certMgr.Initialize(port);
                                EmulatorType emuToStart = certMgr.GetSelectedEmulator();
                                if (certMgr.RequestAccess()) {
                                    std::this_thread::sleep_for(std::chrono::seconds(2));
                                    certMgr.StartEmulator(emuToStart);
                                    notificationSystem.AddNotification("Patch Emulator", "Emulator Patched & Started", ImColor(0, 255, 100));
                                } else {
                                    notificationSystem.AddNotification("Patch Emulator", "Failed to patch Emulator", ImColor(255, 80, 80));
                                }
                            }).detach();
                        }
                        
                        ImGui::Dummy(ImVec2(0, 4));
                        
                        if (edited::Button("Apply Bypass", ImVec2(bw, bh))) {
                            std::string port(adb_port2);
                            std::string proxy(proxy_port2);
                            int game_idx = bypass_game_idx;
                            std::thread([port, proxy, game_idx]() {
                                notificationSystem.AddNotification("Apply Bypass", "Applying Bypass...", ImColor(90, 180, 255));
                                certMgr.Initialize(port);
                                certMgr.SetProxyPort(proxy);
                                certMgr.SetGame(game_idx == 0 ? GameType::FreeFire : GameType::FreeFireMAX);
                                if (certMgr.InstallProxy())
                                    notificationSystem.AddNotification("Apply Bypass", "Bypass Applied Successfully", ImColor(0, 255, 100));
                                else
                                    notificationSystem.AddNotification("Apply Bypass", std::string("Bypass Failed: ") + certMgr.GetLastError(), ImColor(255, 80, 80));
                            }).detach();
                        }
                        
                        ImGui::SameLine(0, 8.f);
                        
                        if (edited::Button("Remove Bypass", ImVec2(bw, bh))) {
                            std::string port(adb_port2);
                            std::thread([port]() {
                                notificationSystem.AddNotification("Remove Bypass", "Removing Bypass...", ImColor(90, 180, 255));
                                certMgr.Initialize(port);
                                if (certMgr.UninstallProxy())
                                    notificationSystem.AddNotification("Remove Bypass", "Bypass Removed Successfully", ImColor(0, 255, 100));
                                else
                                    notificationSystem.AddNotification("Remove Bypass", std::string("Bypass Remove Failed: ") + certMgr.GetLastError(), ImColor(255, 80, 80));
                            }).detach();
                        }
                    }

               


                }
                ImGui::EndChild();
                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar(2);

                ImGui::End();
            }

            notificationSystem.DrawNotifications();


        }

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        const float clear_col[4] = { 0, 0, 0, 0 };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_col);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// FINEX CORP! - UID Bypass System
// Developed by FINEX CORP! - All rights reserved

