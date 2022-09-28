/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/


#ifndef USE_SFA3X_H
#define USE_SFA3X_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_SFA3X_INTERVAL_SECS
#define USE_SFA3X_INTERVAL_SECS (30U)
#endif

#ifndef USE_SFA3X_THREAD_PRIORITY
#define USE_SFA3X_THREAD_PRIORITY (1U)
#endif

#ifndef USE_SFA3X_CHANNEL
#define USE_SFA3X_CHANNEL (1U)
#endif

#define USE_SFA3X_BUFFER_SIZE (4U)

void init_use_sfa3x(kernel_pid_t mainpid, mutex_t * sender_mutex);

#ifdef __cplusplus
}
#endif

#endif /* USE_SFA3X_H */
