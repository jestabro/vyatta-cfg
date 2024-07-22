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

#ifndef _VY_ADAPTER_H_
#define _VY_ADAPTER_H_
#ifdef __cplusplus
extern "C" {
#endif

uint64_t vy_cstore_init(void);
void vy_cstore_free(uint64_t);
int vy_in_session(uint64_t);
int vy_set_path(uint64_t, const void **, size_t);
int vy_delete_path(uint64_t, const void **, size_t);

#ifdef __cplusplus
}
#endif
#endif
