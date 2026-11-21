/* Copyright (C) 2023 John Törnblom
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3, or (at your option) any later
 * version.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct notify_request {
  char useless1[45];
  char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int, notify_request_t*, size_t, int);

int
main(void) {
  int i;

  for (i = 0; i < 5; i++) {
    notify_request_t req;

    memset(&req, 0, sizeof req);
    snprintf(req.message, sizeof req.message,
             "PS5 dev environment OK (%d/5)", i + 1);

    sceKernelSendNotificationRequest(0, &req, sizeof req, 0);
    sleep(3);
  }

  return 0;
}
