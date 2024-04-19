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

typedef enum {
    VY_SUCCESS =   0,
    VY_SET_INVAL = 1 << 0,
    VY_CFG_INVAL = 1 << 1,
    VY_DEL_INVAL = 1 << 2
} VY_ERR;

void *vy_cstore_init(void);
void vy_cstore_free(void *);
int vy_in_session(void *);
VY_ERR vy_set_path(void *, const char **, size_t);
VY_ERR vy_delete_path(void *, const char **, size_t);

#ifdef __cplusplus
}
#endif
#endif
