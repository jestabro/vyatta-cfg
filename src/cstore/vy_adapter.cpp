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
#include <chrono>

#include <cstore/cstore.hpp>
#include <vy_adapter.h>

using namespace cstore;
using namespace chrono;

class Vy_paths {
    public:
        Vy_paths() {
            std::cout << "paths constructor called" << std::endl;
        };
        ~Vy_paths() {
            std::cout << "paths destructor called" << std::endl;
        };
        void add_set_path(const char *path[], size_t len) {
            Cpath path_comps = Cpath(path, len);
            set_list.push_back(path_comps);
        };
        void add_del_path(const char *path[], size_t len) {
            Cpath path_comps = Cpath(path, len);
            del_list.push_back(path_comps);
        };
        vector<Cpath>& get_set_list() {
            return set_list;
        };
        vector<Cpath>& get_del_list() {
            return del_list;
        };
//    private:
        vector<Cpath> del_list;
        vector<Cpath> set_list;
        vector<Cpath> com_list;
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

void *
vy_cpaths_init(void)
{
    Vy_paths *paths = new Vy_paths;
    return (void *) paths;
}

void vy_cpaths_free(void *p)
{
    Vy_paths *paths = (Vy_paths *) p;
    delete paths;
}

int
vy_in_session(void *handle)
{
    Cstore *h = (Cstore *) handle;
    return h->inSession() ? 1 : 0;
}

void
vy_add_set_path(void *handle, const char *path[], size_t len)
{
    Vy_paths *paths = (Vy_paths *)handle;
    paths->add_set_path(path, len);
}

void
vy_add_del_path(void *handle, const char *path[], size_t len)
{
    Vy_paths *paths = (Vy_paths *)handle;
    paths->add_del_path(path, len);
}

out_data_t *
vy_load_paths(void *cstore_handle, void *cpaths_handle, int legacy)
{
    Cstore *cstore_ptr = Cstore::createCstore(false);
    Cstore& cstore = *cstore_ptr;

    Vy_paths *paths = (Vy_paths *)cpaths_handle;
    out_data_t *out_data = NULL;
    std::string out_str = "";
    time_point<high_resolution_clock> start_time;
    time_point<high_resolution_clock> stop_time;
    int total_ms;
    int sec_elapsed;
    int ms_elapsed;
    const char *filename = "config.test";
    cout << "begin copy" << endl;
    vector<Cpath> del_list = paths->del_list;
    vector<Cpath> set_list = paths->set_list;
    cout << "end copy" << endl;;

    start_time = high_resolution_clock::now();

    if (!legacy) {
        cstore.load_paths(del_list, set_list, out_str);
    }
    else {
        cstore.loadFile(filename);
    }

    stop_time = high_resolution_clock::now();
    total_ms = duration_cast<milliseconds>(stop_time - start_time).count();
    sec_elapsed = total_ms / 1000;
    ms_elapsed = total_ms % 1000;
    cout << "load_paths time: sec: " << sec_elapsed << " ms: " << ms_elapsed << endl;

    out_data = out_data_copy(out_str);
    return out_data;
}
/*
out_data_t *
vy_load_paths_prev(void *cstore_handle, void *cpaths_handle)
{
    Cstore *cstore_ptr = Cstore::createCstore(false);
    Cstore& cstore = *cstore_ptr;

    Vy_paths *paths = (Vy_paths *)cpaths_handle;
    out_data_t *out_data = NULL;
    std::string out_str = "";
    time_point<high_resolution_clock> start_time;
    time_point<high_resolution_clock> stop_time;
    int total_ms;
    int sec_elapsed;
    int ms_elapsed;
    cout << "begin copy" << endl;
    vector<Cpath> del_list = paths->del_list;
    vector<Cpath> set_list = paths->set_list;
    cout << "end copy" << endl;;

    start_time = high_resolution_clock::now();

    for (size_t i = 0; i < del_list.size(); i++) {
        if (!cstore.deleteCfgPath(del_list[i])) {
            out_str = out_str + "Delete failed: " + del_list[i].to_string() + "\n";
        }
    }

    stop_time = high_resolution_clock::now();
    total_ms = duration_cast<milliseconds>(stop_time - start_time).count();
    sec_elapsed = total_ms / 1000;
    ms_elapsed = total_ms % 1000;
    cout << "delete time: sec: " << sec_elapsed << " ms: " << ms_elapsed << endl;

    start_time = high_resolution_clock::now();

    for (size_t i = 0; i < set_list.size(); i++) {
        if (!cstore.validateSetPath(set_list[i])) {
            out_str = out_str + "Invalid set path: " + set_list[i].to_string() + "\n";
            continue;
        }

        if (!cstore.setCfgPath(set_list[i])) {
            out_str = out_str + "Set config path failed: " + set_list[i].to_string() + "\n";
        }
    }

    stop_time = high_resolution_clock::now();
    total_ms = duration_cast<milliseconds>(stop_time - start_time).count();
    sec_elapsed = total_ms / 1000;
    ms_elapsed = total_ms % 1000;
    cout << "set time: sec: " << sec_elapsed << " ms: " << ms_elapsed << endl;

    out_data = out_data_copy(out_str);
    return out_data;
}
*/
