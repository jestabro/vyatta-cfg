/* Copyright 2024 VyOS maintainers and contributors <maintainers@vyos.io>
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <cassert>

#include <cstore/cstore.hpp>
#include <vy_adapter.h>

using namespace cstore;

static uint64_t uint_of_voidptr(void* p)
{
  assert (((uintptr_t) p & 1) == 0);
  return (uint64_t) p | 1;
}

static void *voidptr_of_uint(uint64_t v)
{
  return (void *)(uintptr_t)(v & ~1);
}

static Cstore *cstore_of_handle(uint64_t handle)
{
    return (Cstore *) voidptr_of_uint(handle);
}

uint64_t
vy_cstore_init(void)
{
    Cstore *handle = Cstore::createCstore(false);
    return uint_of_voidptr(handle);
}

void
vy_cstore_free(uint64_t handle)
{
    Cstore *h = cstore_of_handle(handle);
    delete h;
}

int
vy_in_session(uint64_t handle)
{
    Cstore *h = cstore_of_handle(handle);
    return h->inSession() ? 1 : 0;
}

int
vy_set_path(uint64_t handle, const void** path_ptr, size_t len)
{
    Cstore *cstore = cstore_of_handle(handle);
    const char **path = (const char **) path_ptr;
    Cpath path_comps = Cpath(path, len);
    int res;

    res = cstore->validateSetPath(path_comps);
    if (!res) {
        return 1;
    }

    res = cstore->setCfgPath(path_comps);
    if (!res) {
        return 2;
    }

    return 0;
}

int
vy_delete_path(uint64_t handle, const void** path_ptr, size_t len)
{
    Cstore *cstore = cstore_of_handle(handle);
    const char **path = (const char **) path_ptr;
    Cpath path_comps = Cpath(path, len);
    int res;

    res = cstore->deleteCfgPath(path_comps);
    if (!res) {
        return 1;
    }

    return 0;
}
