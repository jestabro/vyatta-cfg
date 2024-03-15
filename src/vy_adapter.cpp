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

out_data_t *out_data_copy(std::string msg)
{
    out_data_t *out_data = (out_data_t *) malloc(sizeof(out_data_t) + msg.length());
    out_data->length = msg.length();
    msg.copy(out_data->data, out_data->length);
    return out_data;
}

void out_data_free(out_data_t *out_data)
{
    free(out_data);
}

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

out_data_t *
vy_set_path(void *handle, const char *path[], size_t len)
{
    Cstore *cstore = (Cstore *)handle;
    Cpath path_comps = Cpath(path, len);
    out_data_t *out_data = NULL;
    std::string out_str;
    int res;

    res = cstore->validateSetPath(path_comps);
    if (!res) {
        out_str = "invalid set path: " + path_comps.to_string();
        out_data = out_data_copy(out_str);
        goto out;
    }

    res = cstore->setCfgPath(path_comps);
    if (!res) {
        out_str = "set config path failed: " + path_comps.to_string();
        out_data = out_data_copy(out_str);
        goto out;
    }

out:
    return out_data;
}

out_data_t *
vy_delete_path(void *handle, const char *path[], size_t len)
{
    Cstore *cstore = (Cstore *)handle;
    Cpath path_comps = Cpath(path, len);
    out_data_t *out_data = NULL;
    std::string out_str;
    int res;

    res = cstore->deleteCfgPath(path_comps);
    if (!res) {
        out_str = "delete failed: " + path_comps.to_string();
        out_data = out_data_copy(out_str);
    }

    return out_data;
}
