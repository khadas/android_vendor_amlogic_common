/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "vendor.amlogic.vmx_webclient-service"

#include <android-base/logging.h>
#include <android/binder_ibinder_platform.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "VmxWebClient.h"

using aidl::vendor::amlogic::hardware::vmx_webclient::implementation::VmxWebClient;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(8);

    std::shared_ptr<VmxWebClient> webClient = ::ndk::SharedRefBase::make<VmxWebClient>();
    AIBinder_setRequestingSid(webClient->asBinder().get(), true);

    const std::string Instance =
            std::string() + VmxWebClient::descriptor + "/default";

    binder_status_t status =
            AServiceManager_addService(webClient->asBinder().get(), Instance.c_str());
    CHECK(status == STATUS_OK)
        << "Failed to add VmxWebClient Factory, status=" << status;

    ABinderProcess_joinThreadPool();
}
