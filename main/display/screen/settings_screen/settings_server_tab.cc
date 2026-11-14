#include "settings_server_tab.h"

#include <esp_log.h>

#include <string>

#include "i18n.h"
#include "screen_util.h"
#include "settings.h"
#include "settings_common.h"

LV_FONT_DECLARE(font_puhui_20_4);
LV_FONT_DECLARE(font_puhui_30_4);

namespace {

constexpr const char* TAG = "SettingsServer";
constexpr int kMaxUrlLen = 128;

struct ServerUi {
    lv_obj_t* overlay = nullptr;
    lv_obj_t* textarea = nullptr;
    lv_obj_t* current_lbl = nullptr;
};
ServerUi s_ui;

// 用户自定义地址（覆盖服务器下发值）；存 NVS 命名空间 "websocket"、键 "url_user"。
std::string GetUserUrl() {
    Settings settings("websocket", false);
    return settings.GetString("url_user");
}

// 实际生效地址：优先用户自定义，其次 OTA/激活流程下发的 "url"。
std::string GetActiveUrl() {
    Settings settings("websocket", false);
    std::string url = settings.GetString("url_user");
    if (url.empty()) {
        url = settings.GetString("url");
    }
    return url;
}

void UpdateCurrentLabel() {
    if (s_ui.current_lbl == nullptr) {
        return;
    }
    const std::string url = GetActiveUrl();
    lv_label_set_text(s_ui.current_lbl, url.empty() ? I18n::T("未配置") : url.c_str());
}

void ClosePopup() {
    if (s_ui.overlay != nullptr) {
        lv_obj_delete(s_ui.overlay);
    }
    s_ui.overlay = nullptr;
    s_ui.textarea = nullptr;
}

void OnKbEvent(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CANCEL) {
        ClosePopup();
    }
}

std::string Trimmed(const char* text) {
    std::string s = text != nullptr ? text : "";
    const char* ws = " \t\r\n";
    const size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    const size_t e2 = s.find_last_not_of(ws);
    return s.substr(b, e2 - b + 1);
}

void OnSaveClicked(lv_event_t* /*e*/) {
    std::string url = Trimmed(lv_textarea_get_text(s_ui.textarea));
    if (!url.empty()) {
        // 容错：没写协议前缀时默认 ws://
        if (url.rfind("ws://", 0) != 0 && url.rfind("wss://", 0) != 0) {
            url = "ws://" + url;
        }
        {
            Settings settings("websocket", true);
            settings.SetString("url_user", url);
        }
        ESP_LOGI(TAG, "user WS url saved: %s", url.c_str());
    }
    UpdateCurrentLabel();
    ClosePopup();
}

void OnCancelClicked(lv_event_t* /*e*/) { ClosePopup(); }

void OnEditClicked(lv_event_t* /*e*/) {
    lv_obj_t* scr = lv_screen_active();
    if (scr == nullptr || s_ui.overlay != nullptr) {
        return;
    }

    lv_obj_t* mask = lv_obj_create(scr);
    screen_strip_obj_chrome(mask);
    lv_obj_set_size(mask, kPanelSize, kPanelSize);
    lv_obj_set_pos(mask, 0, 0);
    lv_obj_set_style_bg_color(mask, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mask, LV_OPA_70, LV_PART_MAIN);
    lv_obj_remove_flag(mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(mask, LV_OBJ_FLAG_CLICKABLE);
    screen_swipe_back_ignore(mask, true);
    s_ui.overlay = mask;

    lv_obj_t* card = lv_obj_create(mask);
    screen_strip_obj_chrome(card);
    lv_obj_set_size(card, kPanelSize - 60, 300);
    lv_obj_set_pos(card, 30, 30);
    lv_obj_set_style_bg_color(card, lv_color_hex(kColorCard), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 20, LV_PART_MAIN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, I18n::T("输入 WebSocket 地址"));
    lv_obj_set_style_text_color(title, lv_color_hex(kColorText), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &font_puhui_30_4, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* hint = lv_label_create(card);
    lv_label_set_text(hint, I18n::T("例如 ws://192.168.1.10:8000/xiaozhi/v1/"));
    lv_obj_set_style_text_color(hint, lv_color_hex(kColorSubtle), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint, &font_puhui_20_4, LV_PART_MAIN);
    lv_obj_align_to(hint, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 12);

    lv_obj_t* ta = lv_textarea_create(card);
    s_ui.textarea = ta;
    lv_obj_set_size(ta, kPanelSize - 60 - 40, 60);
    lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 0, 110);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, kMaxUrlLen);
    const std::string current = GetUserUrl().empty() ? GetActiveUrl() : GetUserUrl();
    if (!current.empty()) {
        lv_textarea_set_text(ta, current.c_str());
    }
    lv_obj_set_style_text_font(ta, &font_puhui_20_4, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(kColorText), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x121726), LV_PART_MAIN);
    lv_obj_set_style_radius(ta, 10, LV_PART_MAIN);
    lv_obj_add_state(ta, LV_STATE_FOCUSED);
    screen_swipe_back_ignore(ta, true);

    lv_obj_t* cancel = lv_button_create(card);
    lv_obj_set_size(cancel, 160, 56);
    lv_obj_align(cancel, LV_ALIGN_TOP_RIGHT, -180, 200);
    lv_obj_set_style_radius(cancel, 16, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(kColorCard), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(cancel, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(cancel, OnCancelClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* cancel_lbl = lv_label_create(cancel);
    lv_label_set_text(cancel_lbl, I18n::T("取消"));
    lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(kColorText), LV_PART_MAIN);
    lv_obj_set_style_text_font(cancel_lbl, &font_puhui_20_4, LV_PART_MAIN);
    lv_obj_center(cancel_lbl);
    screen_swipe_back_ignore(cancel, true);

    lv_obj_t* save = lv_button_create(card);
    lv_obj_set_size(save, 160, 56);
    lv_obj_align(save, LV_ALIGN_TOP_RIGHT, 0, 200);
    lv_obj_set_style_radius(save, 16, LV_PART_MAIN);
    lv_obj_set_style_bg_color(save, lv_color_hex(kColorAccent), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(save, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(save, OnSaveClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* save_lbl = lv_label_create(save);
    lv_label_set_text(save_lbl, I18n::T("保存"));
    lv_obj_set_style_text_color(save_lbl, lv_color_hex(kColorText), LV_PART_MAIN);
    lv_obj_set_style_text_font(save_lbl, &font_puhui_20_4, LV_PART_MAIN);
    lv_obj_center(save_lbl);
    screen_swipe_back_ignore(save, true);

    lv_obj_t* kb = lv_keyboard_create(mask);
    lv_obj_set_size(kb, kPanelSize, 390);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_event_cb(kb, OnKbEvent, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(kb, OnKbEvent, LV_EVENT_CANCEL, nullptr);
    screen_swipe_back_ignore(kb, true);
}

void OnResetClicked(lv_event_t* /*e*/) {
    {
        Settings settings("websocket", true);
        settings.EraseKey("url_user");
    }
    ESP_LOGI(TAG, "user WS url cleared, fall back to server-provided/default");
    UpdateCurrentLabel();
}

lv_obj_t* MakeButton(lv_obj_t* parent, const char* text, uint32_t color,
                     lv_event_cb_t cb) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, 240, 64);
    lv_obj_set_style_radius(btn, 14, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(kColorText), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, &font_puhui_20_4, LV_PART_MAIN);
    lv_obj_center(lbl);
    screen_swipe_back_ignore(btn, true);
    return btn;
}

}  // namespace

void SettingsServerTab_Build(lv_obj_t* tab) {
    lv_obj_set_style_pad_all(tab, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_row(tab, 16, LV_PART_MAIN);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* hint = lv_label_create(tab);
    lv_label_set_text(hint, I18n::T("设置语音对话服务器地址"));
    lv_obj_set_style_text_color(hint, lv_color_hex(kColorSubtle), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint, &font_puhui_20_4, LV_PART_MAIN);

    lv_obj_t* card = lv_obj_create(tab);
    screen_strip_obj_chrome(card);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, 150);
    lv_obj_set_style_bg_color(card, lv_color_hex(kColorCard), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 20, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* cap = lv_label_create(card);
    const bool custom = !GetUserUrl().empty();
    lv_label_set_text(cap, custom ? I18n::T("当前地址（自定义）")
                                  : I18n::T("当前地址（服务器下发）"));
    lv_obj_set_style_text_color(cap, lv_color_hex(kColorSubtle), LV_PART_MAIN);
    lv_obj_set_style_text_font(cap, &font_puhui_20_4, LV_PART_MAIN);
    lv_obj_align(cap, LV_ALIGN_TOP_LEFT, 20, 14);

    lv_obj_t* cur = lv_label_create(card);
    s_ui.current_lbl = cur;
    lv_label_set_long_mode(cur, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(cur, kPanelSize - 48 - 220);
    lv_obj_set_style_text_color(cur, lv_color_hex(kColorValue), LV_PART_MAIN);
    lv_obj_set_style_text_font(cur, &font_puhui_20_4, LV_PART_MAIN);
    lv_obj_align(cur, LV_ALIGN_BOTTOM_LEFT, 20, -16);
    UpdateCurrentLabel();

    lv_obj_t* edit = MakeButton(card, I18n::T("修改地址"), kColorAccent, OnEditClicked);
    lv_obj_align(edit, LV_ALIGN_TOP_RIGHT, -16, 12);

    lv_obj_t* reset = MakeButton(tab, I18n::T("恢复默认"), kColorCard, OnResetClicked);

    lv_obj_t* foot = lv_label_create(tab);
    lv_label_set_text(foot, I18n::T("保存后下次对话连接生效"));
    lv_obj_set_style_text_color(foot, lv_color_hex(kColorSubtle), LV_PART_MAIN);
    lv_obj_set_style_text_font(foot, &font_puhui_20_4, LV_PART_MAIN);
}

void SettingsServerTab_Reset() {
    s_ui.overlay = nullptr;
    s_ui.textarea = nullptr;
    s_ui.current_lbl = nullptr;
}
