/* This file is part of vcube.
 *
 * Copyright (C) 2018 Andrew Skalski
 *
 * vcube is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vcube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vcube.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "alloc.h"

// Missing in the header
#ifndef SHM_HUGE_SHIFT
#define SHM_HUGE_SHIFT MAP_HUGE_SHIFT
#endif

using namespace vcube;

static size_t num_pages(size_t n, size_t page_size) {
	size_t mask = (1LL << page_size) - 1;
	return (n >> page_size) + !!(n & mask);
}

static void * map_huge(size_t n, size_t page_size, int prot, int flags) {
	if (page_size) {
		n = num_pages(n, page_size);
		flags |= MAP_HUGETLB | (page_size << MAP_HUGE_SHIFT);
	}
	void *mem = mmap(NULL, n << page_size, prot, flags, -1, 0);
	return (mem == MAP_FAILED) ? NULL : mem;
}

void * alloc::huge_impl(size_t n) {
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;

	void *mem = map_huge(n, 30, prot, flags);   // 1GB pages
	if (!mem) {
		mem = map_huge(n, 21, prot, flags); // 2MB pages
	}
	if (!mem) {
		mem = map_huge(n, 0, prot, flags);  // Standard pages
	}

	return mem;
}

static void * map_shared(size_t n, uint32_t key, bool rdwr, size_t page_size) {
	int flags = 0600;
	if (rdwr) {
		flags |= IPC_CREAT | IPC_EXCL;
	}
	if (page_size) {
		n = num_pages(n, page_size);
		flags |= SHM_HUGETLB | (page_size << SHM_HUGE_SHIFT);
	}
	int shm = shmget(key, n << page_size, flags);
	if (shm == -1) {
		return NULL;
	}
	void *mem = shmat(shm, NULL, rdwr ? 0 : SHM_RDONLY);
	return (mem == (void *) -1) ? NULL : mem;
}

void * alloc::shared_impl(size_t n, uint32_t key, bool rdwr) {
	void *mem = NULL;
	if (rdwr) {
		// Huge page setting is relevant only on create
		mem = map_shared(n, key, rdwr, 30);         // 1GB pages
		if (!mem) {
			mem = map_shared(n, key, rdwr, 21); // 2MB pages
		}
	}
	if (!mem) {
		mem = map_shared(n, key, rdwr, 0);
	}
	return mem;
}
