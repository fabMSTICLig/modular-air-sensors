/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/


#ifndef USE_SCD30_H
#define USE_SCD30_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_SCD30_INTERVAL_SECS
#define USE_SCD30_INTERVAL_SECS (30U)
#endif

#ifndef USE_SCD30_THREAD_PRIORITY
#define USE_SCD30_THREAD_PRIORITY (1U)
#endif

#ifndef USE_SCD30_CHANNEL
#define USE_SCD30_CHANNEL (1U)
#endif

#define USE_SCD30_BUFFER_SIZE (4U)

#ifdef USE_SCD30_TH_ON

#ifndef USE_SCD30_TH_CHANNEL
#define USE_SCD30_TH_CHANNEL (2U)
#endif

#define USE_SCD30_TH_BUFFER_SIZE (6U)

#endif

void init_use_scd30(kernel_pid_t mainpid, mutex_t * sender_mutex);

#ifdef __cplusplus
}
#endif

#endif /* USE_SCD30_H */
