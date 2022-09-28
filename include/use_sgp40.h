/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/


#ifndef USE_SGP40_H
#define USE_SGP40_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_SGP40_INTERVAL_SECS
#define USE_SGP40_INTERVAL_SECS (30U)
#endif

#ifndef USE_SGP40_THREAD_PRIORITY
#define USE_SGP40_THREAD_PRIORITY (3U)
#endif

#ifndef USE_SGP40_CHANNEL
#define USE_SGP40_CHANNEL (3U)
#endif

#define USE_SGP40_BUFFER_SIZE (4U)

void init_use_sgp40(kernel_pid_t senderpid, mutex_t * sender_mutex);

#ifdef __cplusplus
}
#endif

#endif /* USE_SGP40_H */
