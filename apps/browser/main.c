/* Copyright (C) 2023 John Törnblom
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3, or (at your option) any later
 * version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef PS5_URL
#define PS5_URL "https://v.qq.com"
#endif

int sceUserServiceInitialize(void*);
int sceUserServiceTerminate(void);
int sceSystemServiceLaunchWebBrowser(const char* uri, void*);

__attribute__((constructor)) static void
constructor(void) {
  if (sceUserServiceInitialize(0)) {
    perror("sceUserServiceInitialize");
  }
}

int
main(void) {
  if (sceSystemServiceLaunchWebBrowser(PS5_URL, 0)) {
    perror("sceSystemServiceLaunchWebBrowser");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

__attribute__((destructor)) static void
destructor(void) {
  sceUserServiceTerminate();
}
