/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/


#ifndef USE_SVM40_H
#define USE_SVM40_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_SVM40_INTERVAL_SECS
#define USE_SVM40_INTERVAL_SECS (30U)
#endif

#ifndef USE_SVM40_THREAD_PRIORITY
#define USE_SVM40_THREAD_PRIORITY (1U)
#endif

#ifndef USE_SVM40_CHANNEL
#define USE_SVM40_CHANNEL (1U)
#endif

#define USE_SVM40_BUFFER_SIZE (12U)

void init_use_svm40(kernel_pid_t mainpid, mutex_t * sender_mutex);

#ifdef __cplusplus
}
#endif

#endif /* USE_SVM40_H */
