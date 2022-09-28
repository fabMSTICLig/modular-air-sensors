/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/


#ifndef USE_GMXXX_H
#define USE_GMXXX_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_GMXXX_INTERVAL_SECS
#define USE_GMXXX_INTERVAL_SECS (30U)
#endif

#ifndef USE_GMXXX_THREAD_PRIORITY
#define USE_GMXXX_THREAD_PRIORITY (4U)
#endif

#ifndef USE_GMXXX_CHANNEL
#define USE_GMXXX_CHANNEL (4U)
#endif

#define USE_GMXXX_BUFFER_SIZE (10U)

void init_use_gmxxx(kernel_pid_t mainpid, mutex_t * sender_mutex);

#ifdef __cplusplus
}
#endif

#endif /* USE_GMXXX_H */
