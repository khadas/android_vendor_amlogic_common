/*
 * @Author: your name
 * @Date: 2021-07-01 15:50:33
 * @LastEditTime: 2021-07-02 14:14:19
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \uwe5621dsy:\r\vendor\amlogic\common\wifi_bt\wifi\wifi_info\wifi_info.h
 */
/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WIFI_INFO_H
#define WIFI_INFO_H

typedef struct load_info{
    const char *chip_id;
    const char *pcie_id;
    const char *wifi_module_name;
    const char *wifi_module_path;
    const char *wifi_module_arg;
    const char *wifi_name;
    int  wifi_pid;
    const char *wifi_path;
    const char *bt_module_name;
    bool is_bt;
    bool share_power;
} dongle_info;

typedef struct bt_load_info{
    const char *bt_module_name;
    const char *bt_module_path;
    const char *bt_name;
    int bt_pid;
} bt_dongle_info;

int get_wifi_info (dongle_info *ext_info);
int get_bt_info (bt_dongle_info *ext_info);

#endif // WIFI_INFO
