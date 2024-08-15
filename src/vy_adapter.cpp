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
#include <fcntl.h>
#include <unistd.h>

#include <cstore/cstore.hpp>
#include <vy_adapter.h>

using namespace cstore;

enum PIPES { READ, WRITE };

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
            dup2(out_pipe[WRITE], fileno(stderr));
            redirecting = true;
        }

        std::string get_redirected_output() {
            if (!redirecting) {
                return "";
            }

            char buffer[1024];
            ssize_t bytesRead = 0;
            std::string redirected_output = "";
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
            close(out_pipe[READ]);
            close(out_pipe[WRITE]);
        }

    private:
        int out_pipe[2];
        int old_stdout;
        bool redirecting;
};

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
    int res;

    stdout_redirect redirect = stdout_redirect();

    res = cstore->validateSetPath(path_comps);
    if (!res) {
        out_data_copy(redirect.get_redirected_output());
        goto out;
    }

    res = cstore->setCfgPath(path_comps);
    if (!res) {
        out_data_copy(redirect.get_redirected_output());
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
    int res;

    stdout_redirect redirect = stdout_redirect();

    res = cstore->deleteCfgPath(path_comps);
    if (!res) {
        out_data_copy(redirect.get_redirected_output());
    }

    return out_data;
}
