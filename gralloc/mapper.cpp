/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"

#include <ion/ion.h>
#include <linux/ion.h>

/*****************************************************************************/

static int gralloc_map(gralloc_module_t const* module, buffer_handle_t handle)
{
    private_handle_t* hnd = (private_handle_t*)handle;

    void* mappedAddress = mmap(0, hnd->size, PROT_READ|PROT_WRITE, MAP_SHARED, 
                               hnd->fd, 0);
    if (mappedAddress == MAP_FAILED) {
        ALOGE("%s: could not mmap %s", __func__, strerror(errno));
        return -errno;
    }
    ALOGV("%s: base %p %d %d %d %d\n", __func__, mappedAddress, hnd->size,
          hnd->width, hnd->height, hnd->stride);
    hnd->base = mappedAddress;
    return 0;
}

static int gralloc_unmap(gralloc_module_t const* module, buffer_handle_t handle)
{
    private_handle_t* hnd = (private_handle_t*)handle;

    if (!hnd->base)
        return 0;

    if (munmap(hnd->base, hnd->size) < 0) {
        ALOGE("%s :could not unmap %s %p %d", __func__, strerror(errno),
              hnd->base, hnd->size);
    }
    ALOGV("%s: base %p %d %d %d %d\n", __func__, hnd->base, hnd->size,
          hnd->width, hnd->height, hnd->stride);
    hnd->base = 0;
    return 0;
}

/*****************************************************************************/

int grallocMap(gralloc_module_t const* module, private_handle_t *hnd)
{
    return gralloc_map(module, hnd);
}

int grallocUnmap(gralloc_module_t const* module, private_handle_t *hnd)	
{
    return gralloc_unmap(module, hnd);
}

static pthread_mutex_t sMapLock = PTHREAD_MUTEX_INITIALIZER; 

/*****************************************************************************/

int gralloc_register_buffer(gralloc_module_t const* module,
                            buffer_handle_t handle)
{
    int err;
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    err = gralloc_map(module, handle);

    private_handle_t* hnd = (private_handle_t*)handle;
    ALOGV("%s: base %p %d %d %d %d\n", __func__, hnd->base, hnd->size,
          hnd->width, hnd->height, hnd->stride);
    return err;
}

int gralloc_unregister_buffer(gralloc_module_t const* module,
                              buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;
    ALOGV("%s: base %p %d %d %d %d\n", __func__, hnd->base, hnd->size,
          hnd->width, hnd->height, hnd->stride);

    gralloc_unmap(module, handle);

    return 0;
}

int gralloc_lock(gralloc_module_t const* module,
                 buffer_handle_t handle, int usage,
                 int l, int t, int w, int h,
                 void** vaddr)
{
    // this is called when a buffer is being locked for software
    // access. in thin implementation we have nothing to do since
    // not synchronization with the h/w is needed.
    // typically this is used to wait for the h/w to finish with
    // this buffer if relevant. the data cache may need to be
    // flushed or invalidated depending on the usage bits and the
    // hardware.

    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;
    *vaddr = (void*)hnd->base;
    return 0;
}

int gralloc_unlock(gralloc_module_t const* module, 
                   buffer_handle_t handle)
{
    // we're done with a software buffer. nothing to do in this
    // implementation. typically this is used to flush the data cache.

    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;
    return 0;
}
