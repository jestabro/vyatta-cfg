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

#include <cstore/cstore.hpp>
#include <vy_adapter.h>

using namespace cstore;

void *
vy_cstore_init(void)
{
    Cstore *handle = Cstore::createCstore(false);
    return (void *) handle;
}

void
vy_cstore_free(void *handle)
{
    Cstore *h = (Cstore *) handle;
    delete h;
}

int
vy_in_session(void *handle)
{
    Cstore *h = (Cstore *) handle;
    return h->inSession() ? 1 : 0;
}

VY_ERR
vy_set_path(void *handle, const char *path[], size_t len)
{
    Cstore *cstore = (Cstore *)handle;
    Cpath path_comps = Cpath(path, len);
    std::string out_str;
    int res;

    res = cstore->validateSetPath(path_comps);
    if (!res) {
        return VY_SET_INVAL;
    }

    res = cstore->setCfgPath(path_comps);
    if (!res) {
        return VY_CFG_INVAL;
    }

    return VY_SUCCESS;
}

VY_ERR
vy_delete_path(void *handle, const char *path[], size_t len)
{
    Cstore *cstore = (Cstore *)handle;
    Cpath path_comps = Cpath(path, len);
    std::string out_str;
    int res;

    res = cstore->deleteCfgPath(path_comps);
    if (!res) {
        return VY_DEL_INVAL;
    }

    return VY_SUCCESS;
}
