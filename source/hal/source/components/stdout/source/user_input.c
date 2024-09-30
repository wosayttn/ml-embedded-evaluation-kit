/*
 * SPDX-FileCopyrightText: Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
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
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

unsigned int GetLine(char *user_input, unsigned int size)
{
    static unsigned int counter = 0;
    rt_kprintf("%s %s %d\n", __FILE__, __func__, __LINE__);

#if 0
    if (NULL != fgets(user_input, size, stdin))
    {
        rt_kprintf("%s %s %d\n", __FILE__, __func__, __LINE__);

        return 1;
    }
    rt_kprintf("%s %s %d\n", __FILE__, __func__, __LINE__);
    return 0;
#else

    user_input[0] = '1' + (counter % 5);
    user_input[1] = (char)0;
    counter++;

    return 1;
#endif
}

#ifdef __cplusplus
}
#endif
