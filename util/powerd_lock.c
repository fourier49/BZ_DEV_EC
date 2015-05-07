/*
 * This file is ported from the flashrom project.
 *
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * powerd_lock.c: power management routines
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "powerd_lock.h"

/*
 * Path to a file containing flashrom's PID. While present, powerd avoids
 * suspending or shutting down the system.
 */
static const char lock_file_path[] = "/var/lock/battery_tool_powerd.lock";

int disable_power_management()
{
	FILE *lock_file;
	int rc = 0;

	lock_file = fopen(lock_file_path, "w");
	if (!lock_file) {
		printf("%s: Failed to open %s for writing: %s\n",
			__func__, lock_file_path, strerror(errno));
		return 1;
	}

	if (fprintf(lock_file, "%ld", (long)getpid()) < 0) {
		printf("%s: Failed to write PID to %s: %s\n",
			__func__, lock_file_path, strerror(errno));
		rc = 1;
	}

	if (fclose(lock_file) != 0) {
		printf("%s: Failed to close %s: %s\n",
			__func__, lock_file_path, strerror(errno));
	}
	return rc;

}

int restore_power_management()
{
	int result = 0;

	result = unlink(lock_file_path);
	if (result != 0 && errno != ENOENT)  {
		printf("%s: Failed to unlink %s: %s\n",
			__func__, lock_file_path, strerror(errno));
		return 1;
	}
	return 0;
}
