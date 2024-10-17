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
#include <algorithm>
#include <list>
#include <fcntl.h>
#include <unistd.h>

#include <cstore/cstore.hpp>
#include <vy_adapter.h>

using namespace cstore;

enum PIPES { READ, WRITE };

extern FILE *out_stream;

class stdout_redirect {
    public:
        stdout_redirect(): old_stdout(0), redirecting(false) {
            out_pipe[READ] = 0;
            out_pipe[WRITE] = 0;
            if (pipe(out_pipe) == -1) {
                return;
            }

            old_stdout = dup(fileno(stdout));
            setbuf(stdout, NULL);
            dup2(out_pipe[WRITE], fileno(stdout));

            redirecting = true;
        }

        std::string get_redirected_output() {
            if (!redirecting) {
                return "";
            }

            char buffer[1024];
            ssize_t bytesRead = 0;
            std::string redirected_output = "\n";
            fcntl(out_pipe[READ], F_SETFL, O_NONBLOCK);
            while ((bytesRead = read(out_pipe[READ], buffer, sizeof(buffer))) > 0) {
                redirected_output.append(buffer, bytesRead);
            }

            return redirected_output;
        }

        ~stdout_redirect() {
            if (!redirecting) {
                return;
            }

            dup2(old_stdout, fileno(stdout));

            if (old_stdout > 0) {
                close(old_stdout);
            }
            if (out_pipe[READ] > 0) {
                close(out_pipe[READ]);
            }
            if (out_pipe[WRITE] > 0) {
                close(out_pipe[WRITE]);
            }
        }

    private:
        int out_pipe[2];
        int old_stdout;
        bool redirecting;
};

struct cstore_obj {
    Cstore *cstore;
    std::list<char *> output;
};

static struct cstore_obj* create_handle() {
    struct cstore_obj *h  = new cstore_obj;
    h->cstore = Cstore::createCstore(false);
    return h;
};

const char *out_data_copy(std::string msg, cstore_obj *o)
{
    size_t len = msg.length();
    char *out_data = (char *) malloc(len + 1);
    o->output.push_back(out_data);
    msg.copy(out_data, len);
    out_data[len] = '\0';
    return out_data;
}

static uint64_t uint_of_voidptr(void* p)
{
  assert (((uintptr_t) p & 1) == 0);
  return (uint64_t) p | 1;
}

static void *voidptr_of_uint(uint64_t v)
{
  return (void *)(uintptr_t)(v & ~1);
}

static cstore_obj *cstore_obj_of_handle(uint64_t handle)
{
    return (cstore_obj *) voidptr_of_uint(handle);
}

uint64_t
vy_cstore_init(void)
{
    cstore_obj *handle = create_handle();
    return uint_of_voidptr(handle);
}

void
vy_cstore_free(uint64_t handle)
{
    cstore_obj *h = cstore_obj_of_handle(handle);
    for (char * x: h->output) {
        free(x);
    }
    delete h->cstore;
    delete h;
}

int
vy_in_session(uint64_t handle)
{
    Cstore *h = cstore_obj_of_handle(handle)->cstore;
    return h->inSession() ? 1 : 0;
}

const char *
vy_validate_path(uint64_t handle, const void** path_ptr, size_t len)
{
    cstore_obj *obj = cstore_obj_of_handle(handle);
    Cstore *cstore = obj->cstore;
    const char **path = (const char **) path_ptr;
    Cpath path_comps = Cpath(path, len);
    const char *out_data;
    std::string out_str = "";
    int res;

    out_stream = stdout;
    stdout_redirect redirect = stdout_redirect();

    res = cstore->validateSetPath(path_comps);
    if (!res) {
        out_str = "\nInvalid set path: " + path_comps.to_string() + "\n";
        out_str.append(redirect.get_redirected_output());
    }

    out_data = out_data_copy(out_str, obj);
    out_stream = NULL;
    return out_data;
}

const char *
vy_set_path(uint64_t handle, const void** path_ptr, size_t len)
{
    cstore_obj *obj = cstore_obj_of_handle(handle);
    Cstore *cstore = obj->cstore;
    const char **path = (const char **) path_ptr;
    Cpath path_comps = Cpath(path, len);
    const char *out_data;
    std::string out_str = "";
    int res;

    out_stream = stdout;
    stdout_redirect redirect = stdout_redirect();

    res = cstore->setCfgPath(path_comps);
    if (!res) {
        out_str = "\nSet config path failed: " + path_comps.to_string() + "\n";
        out_str.append(redirect.get_redirected_output());
    }

    out_data = out_data_copy(out_str, obj);
    out_stream = NULL;
    return out_data;
}

const char *
vy_legacy_set_path(uint64_t handle, const void** path_ptr, size_t len)
{
    cstore_obj *obj = cstore_obj_of_handle(handle);
    Cstore *cstore = obj->cstore;
    const char **path = (const char **) path_ptr;
    Cpath path_comps = Cpath(path, len);
    const char *out_data;
    std::string out_str = "";
    int res;

    out_stream = stdout;
    stdout_redirect redirect = stdout_redirect();

    res = cstore->validateSetPath(path_comps);
    if (!res) {
        out_str = "\nInvalid set path: " + path_comps.to_string() + "\n";
        out_str.append(redirect.get_redirected_output());
        goto out;
    }

    res = cstore->setCfgPath(path_comps);
    if (!res) {
        out_str = "\nSet config path failed: " + path_comps.to_string() + "\n";
        out_str.append(redirect.get_redirected_output());
    }

out:
    out_data = out_data_copy(out_str, obj);
    out_stream = NULL;
    return out_data;
}

const char *
vy_delete_path(uint64_t handle, const void** path_ptr, size_t len)
{
    cstore_obj *obj = cstore_obj_of_handle(handle);
    Cstore *cstore = obj->cstore;
    const char **path = (const char **) path_ptr;
    Cpath path_comps = Cpath(path, len);
    const char *out_data;
    std::string out_str = "";
    int res;

    out_stream = stdout;
    stdout_redirect redirect = stdout_redirect();

    res = cstore->deleteCfgPath(path_comps);
    if (!res) {
        out_str = "\nDelete failed: " + path_comps.to_string() + "\n";
        out_str.append(redirect.get_redirected_output());
    }

    out_data = out_data_copy(out_str, obj);
    out_stream = NULL;
    return out_data;
}
