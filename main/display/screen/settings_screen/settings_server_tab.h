#pragma once

#include "lvgl.h"

// 「服务器」设置页：查看 / 修改 WebSocket 服务器地址（NVS 持久化）。
void SettingsServerTab_Build(lv_obj_t* tab);
void SettingsServerTab_Reset();
