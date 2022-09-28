/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/


#ifndef USE_SPS30_H
#define USE_SPS30_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_SPS30_INTERVAL_SECS
#define USE_SPS30_INTERVAL_SECS (30U)
#endif

#ifndef USE_SPS30_THREAD_PRIORITY
#define USE_SPS30_THREAD_PRIORITY (2U)
#endif

#ifndef USE_SPS30_CHANNEL
#define USE_SPS30_CHANNEL (2U)
#endif

#define USE_SPS30_BUFFER_SIZE (10U)

void init_use_sps30(kernel_pid_t sender_pid, mutex_t * sender_mutex);

#ifdef __cplusplus
}
#endif

#endif /* USE_SPS30_H */
