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

struct cout_redirect {
    public:
        cout_redirect() {
            old = std::cout.rdbuf( buffer.rdbuf() ); // redirect cout to buffer stream
        }
        std::string get_string() {
            return buffer.str(); // get string
        }
        ~cout_redirect( ) {
            std::cout.rdbuf( old ); // reverse redirect
            std::cout << "cout_redirect destructor" << std::endl;
        }
    private:
        std::stringstream buffer;
        std::streambuf * old;
};

out_data_t *out_data_init()
{
    out_data_t *out_data = (out_data_t *) malloc(sizeof(out_data_t));
    out_data->length = 0;
    return out_data;
}

out_data_t *out_data_copy(std::string msg)
{
    out_data_t *out_data = (out_data_t *) malloc(sizeof(out_data_t) + msg.length());
    out_data->length = msg.length();
    msg.copy(out_data->data, out_data->length);
    return out_data;
}

void out_data_free(out_data_t *out_data)
{
    std::cout << "free out_data" << std::endl;
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

out_data_t *
vy_set_path(void *handle, const char *path[], size_t len)
{
    Cstore *cstore = (Cstore *)handle;
    Cpath path_comps = Cpath(path, len);
    out_data_t *out_data;
    int res;

    cout_redirect redir = cout_redirect();

    res = cstore->validateSetPath(path_comps);
    if (!res) {
        std::cout << "invalid set path" << std::endl;
        out_data = out_data_copy(redir.get_string());
        goto out;
    }
    else
        out_data = out_data_init();

    res = cstore->setCfgPath(path_comps);
    if (!res) {
        std::cout << "set config path failed" << std::endl;
        out_data = out_data_copy(redir.get_string());
        goto out;
    }
    else
        out_data = out_data_init();

out:
    return out_data;
}
