/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 Arm Limited and/or its
 * affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
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

/****************************************************************************\
 *               Main application file for ARM NPU on MPS3 board             *
\****************************************************************************/

#include "hal.h"                    /* our hardware abstraction api */
#include "log_macros.h"
#include "TensorFlowLiteMicro.hpp"  /* our inference logic api */

#include <cstdio>

extern void main_loop();

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
    __ASM(" .global __ARM_use_no_argv\n");
#endif

/* Print application information. */
static void print_application_intro()
{
    info("%s\n", PRJ_DES_STR);
    info("Version %s Build date: " __DATE__ " @ " __TIME__ "\n", PRJ_VER_STR);
    info("Copyright 2021-2024 Arm Limited and/or its affiliates <open-source-office@arm.com>\n\n");
}

int mlevk_go(void)
{
    if (hal_platform_init())
    {
        /* Application information, UART should have been initialised. */
        print_application_intro();

        /* Enable TensorFlow Lite Micro logging. */
        EnableTFLMLog();

        /* Run the application. */
        main_loop();
    }

    /* This is unreachable without errors. */
    info("program terminating...\n");

    /* Release platform. */
    hal_platform_release();

    return 0;
}
MSH_CMD_EXPORT(mlevk_go, Start MLEVK);


#if defined(MLEVK_UC_LIVE_DEMO)

#define THREAD_PRIORITY   20
#define THREAD_STACK_SIZE 4096
#define THREAD_TIMESLICE  5

void mlevk_entry(void *p)
{
    mlevk_go();
}

int mlevk_worker(void)
{
    rt_thread_t mlevk_thread = rt_thread_create("mlevk",
                               mlevk_entry,
                               RT_NULL,
                               THREAD_STACK_SIZE,
                               THREAD_PRIORITY,
                               THREAD_TIMESLICE);

    if (mlevk_thread != RT_NULL)
        rt_thread_startup(mlevk_thread);

    return 0;
}
INIT_APP_EXPORT(mlevk_worker);

#endif

