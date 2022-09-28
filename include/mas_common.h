/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/

#ifndef AIRQUAL_COMMON_H
#define AIRQUAL_COMMON_H

#include <errno.h>
#include "mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t * buffer;
  mutex_t * mutex;
} airmsg_t;

#define MSG_BUFFER(msg) (((airmsg_t *)msg.content.ptr)->buffer)
#define MSG_MUTEX(msg) (((airmsg_t *)msg.content.ptr)->mutex)

#ifdef __cplusplus
}
#endif

#endif /* AIRQUAL_COMMON_H */

