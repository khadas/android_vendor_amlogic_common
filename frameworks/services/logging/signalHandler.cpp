/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include<string.h>

#include <utils/Log.h>
#include <utils/Looper.h>

#include "signalHandler.h"

#define LOG_TAG "logging"

static int signal_write_fd = -1;
static int signal_read_fd = -1;

static void SIGCHLD_handler(int) {
    int status;
    int pid = waitpid(-1, &status, WNOHANG);
    ALOGD("waited pid: %d\n", pid);
}

void signalHandlerInit() {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIGCHLD_handler;
    act.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &act, 0);

}
