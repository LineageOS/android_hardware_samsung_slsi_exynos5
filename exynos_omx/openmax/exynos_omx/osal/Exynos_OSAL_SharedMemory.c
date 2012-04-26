/*
 *
 * Copyright 2010 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file        Exynos_OSAL_SharedMemory.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Taehwan Kim (t_h.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <fcntl.h>

#include "Exynos_OSAL_SharedMemory.h"
#include "ion.h"

#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


static int mem_cnt = 0;


struct SEC_OMX_SHAREDMEM_LIST;
typedef struct _SEC_OMX_SHAREDMEM_LIST
{
    OMX_U32 ion_buffer;
    OMX_PTR mapAddr;
    OMX_U32 allocSize;
    struct _SEC_OMX_SHAREDMEM_LIST *nextMemory;
} SEC_OMX_SHAREDMEM_LIST;

typedef struct _EXYNOS_OMX_SHARE_MEMORY
{
    OMX_HANDLETYPE pIONHandle;
    SEC_OMX_SHAREDMEM_LIST *pAllocMemory;
    OMX_HANDLETYPE SMMutex;
} EXYNOS_OMX_SHARE_MEMORY;


OMX_HANDLETYPE Exynos_OSAL_SharedMemory_Open()
{
    EXYNOS_OMX_SHARE_MEMORY *pHandle = NULL;
    ion_client IONClient = 0;
    pHandle = (EXYNOS_OMX_SHARE_MEMORY *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_SHARE_MEMORY));
    Exynos_OSAL_Memset(pHandle, 0, sizeof(EXYNOS_OMX_SHARE_MEMORY));
    if (pHandle == NULL)
        goto EXIT;

    IONClient = (OMX_HANDLETYPE)ion_client_create();
    if (IONClient <= 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ion_client_create Error: %d", IONClient);
        Exynos_OSAL_Free((void *)pHandle);
        pHandle = NULL;
        goto EXIT;
    }

    pHandle->pIONHandle = IONClient;

    Exynos_OSAL_MutexCreate(&pHandle->SMMutex);

EXIT:
    return (OMX_HANDLETYPE)pHandle;
}

void Exynos_OSAL_SharedMemory_Close(OMX_HANDLETYPE handle)
{
    EXYNOS_OMX_SHARE_MEMORY *pHandle = (EXYNOS_OMX_SHARE_MEMORY *)handle;

    Exynos_OSAL_MutexTerminate(pHandle->SMMutex);
    pHandle->SMMutex = NULL;

    ion_client_destroy((ion_client)pHandle->pIONHandle);
    pHandle->pIONHandle = NULL;

    Exynos_OSAL_Free(pHandle);

    return;
}

OMX_PTR Exynos_OSAL_SharedMemory_Alloc(OMX_HANDLETYPE handle, OMX_U32 size, MEMORY_TYPE memoryType)
{
    EXYNOS_OMX_SHARE_MEMORY *pHandle = (EXYNOS_OMX_SHARE_MEMORY *)handle;
    SEC_OMX_SHAREDMEM_LIST *SMList = NULL;
    SEC_OMX_SHAREDMEM_LIST *Element = NULL;
    SEC_OMX_SHAREDMEM_LIST *currentElement = NULL;
    ion_buffer IONBuffer = 0;
    OMX_PTR pBuffer = NULL;

    if (pHandle == NULL)
        goto EXIT;

    Element = (SEC_OMX_SHAREDMEM_LIST *)Exynos_OSAL_Malloc(sizeof(SEC_OMX_SHAREDMEM_LIST));
    Exynos_OSAL_Memset(Element, 0, sizeof(SEC_OMX_SHAREDMEM_LIST));

    if (SECURE_MEMORY == memoryType)
        IONBuffer = (OMX_PTR)ion_alloc((ion_client)pHandle->pIONHandle, size, 0, ION_HEAP_EXYNOS_VIDEO_MASK);
    else
        IONBuffer = (OMX_PTR)ion_alloc((ion_client)pHandle->pIONHandle, size, 0, ION_HEAP_EXYNOS_MASK);

    if (IONBuffer <= 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ion_alloc Error: %d", IONBuffer);
        Exynos_OSAL_Free((void*)Element);
        goto EXIT;
    }

    pBuffer = ion_map(IONBuffer, size, 0);
    if (pBuffer == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ion_map Error");
        ion_free(IONBuffer);
        Exynos_OSAL_Free((void*)Element);
        goto EXIT;
    }

    Element->ion_buffer = IONBuffer;
    Element->mapAddr = pBuffer;
    Element->allocSize = size;
    Element->nextMemory = NULL;

    Exynos_OSAL_MutexLock(pHandle->SMMutex);
    SMList = pHandle->pAllocMemory;
    if (SMList == NULL) {
        pHandle->pAllocMemory = SMList = Element;
    } else {
        currentElement = SMList;
        while (currentElement->nextMemory != NULL) {
            currentElement = currentElement->nextMemory;
        }
        currentElement->nextMemory = Element;
    }
    Exynos_OSAL_MutexUnlock(pHandle->SMMutex);

    mem_cnt++;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SharedMemory alloc count: %d", mem_cnt);

EXIT:
    return pBuffer;
}

void Exynos_OSAL_SharedMemory_Free(OMX_HANDLETYPE handle, OMX_PTR pBuffer)
{
    EXYNOS_OMX_SHARE_MEMORY *pHandle = (EXYNOS_OMX_SHARE_MEMORY *)handle;
    SEC_OMX_SHAREDMEM_LIST *SMList = NULL;
    SEC_OMX_SHAREDMEM_LIST *currentElement = NULL;
    SEC_OMX_SHAREDMEM_LIST *deleteElement = NULL;

    if (pHandle == NULL)
        goto EXIT;

    Exynos_OSAL_MutexLock(pHandle->SMMutex);
    SMList = pHandle->pAllocMemory;
    if (SMList == NULL) {
        Exynos_OSAL_MutexUnlock(pHandle->SMMutex);
        goto EXIT;
    }

    currentElement = SMList;
    if (SMList->mapAddr == pBuffer) {
        deleteElement = SMList;
        pHandle->pAllocMemory = SMList = SMList->nextMemory;
    } else {
        while ((currentElement != NULL) && (((SEC_OMX_SHAREDMEM_LIST *)(currentElement->nextMemory))->mapAddr != pBuffer))
            currentElement = currentElement->nextMemory;

        if (((SEC_OMX_SHAREDMEM_LIST *)(currentElement->nextMemory))->mapAddr == pBuffer) {
            deleteElement = currentElement->nextMemory;
            currentElement->nextMemory = deleteElement->nextMemory;
        } else if (currentElement == NULL) {
            Exynos_OSAL_MutexUnlock(pHandle->SMMutex);
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Exynos_OSAL_MutexUnlock(pHandle->SMMutex);

    if (ion_unmap(deleteElement->mapAddr, deleteElement->allocSize)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ion_unmap fail");
        goto EXIT;
    }
    deleteElement->mapAddr = NULL;
    deleteElement->allocSize = 0;

    ion_free(deleteElement->ion_buffer);
    deleteElement->ion_buffer = 0;

    Exynos_OSAL_Free(deleteElement);

    mem_cnt--;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SharedMemory free count: %d", mem_cnt);

EXIT:
    return;
}

int Exynos_OSAL_SharedMemory_VirtToION(OMX_HANDLETYPE handle, OMX_PTR pBuffer)
{
    EXYNOS_OMX_SHARE_MEMORY *pHandle = (EXYNOS_OMX_SHARE_MEMORY *)handle;
    SEC_OMX_SHAREDMEM_LIST *SMList = NULL;
    SEC_OMX_SHAREDMEM_LIST *currentElement = NULL;
    SEC_OMX_SHAREDMEM_LIST *findElement = NULL;
    int ion_addr = 0;
    if (pHandle == NULL)
        goto EXIT;

    Exynos_OSAL_MutexLock(pHandle->SMMutex);
    SMList = pHandle->pAllocMemory;
    if (SMList == NULL) {
        Exynos_OSAL_MutexUnlock(pHandle->SMMutex);
        goto EXIT;
    }

    currentElement = SMList;
    if (SMList->mapAddr == pBuffer) {
        findElement = SMList;
    } else {
        while ((currentElement != NULL) && (((SEC_OMX_SHAREDMEM_LIST *)(currentElement->nextMemory))->mapAddr != pBuffer))
            currentElement = currentElement->nextMemory;

        if (((SEC_OMX_SHAREDMEM_LIST *)(currentElement->nextMemory))->mapAddr == pBuffer) {
            findElement = currentElement->nextMemory;
        } else if (currentElement == NULL) {
            Exynos_OSAL_MutexUnlock(pHandle->SMMutex);
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Exynos_OSAL_MutexUnlock(pHandle->SMMutex);

    ion_addr = findElement->ion_buffer;

EXIT:
    return ion_addr;
}

OMX_PTR Exynos_OSAL_SharedMemory_IONToVirt(OMX_HANDLETYPE handle, int ion_addr)
{
    EXYNOS_OMX_SHARE_MEMORY *pHandle = (EXYNOS_OMX_SHARE_MEMORY *)handle;
    SEC_OMX_SHAREDMEM_LIST *SMList = NULL;
    SEC_OMX_SHAREDMEM_LIST *currentElement = NULL;
    SEC_OMX_SHAREDMEM_LIST *findElement = NULL;
    OMX_PTR pBuffer = NULL;
    if (pHandle == NULL)
        goto EXIT;

    Exynos_OSAL_MutexLock(pHandle->SMMutex);
    SMList = pHandle->pAllocMemory;
    if (SMList == NULL) {
        Exynos_OSAL_MutexUnlock(pHandle->SMMutex);
        goto EXIT;
    }

    currentElement = SMList;
    if (SMList->ion_buffer == ion_addr) {
        findElement = SMList;
    } else {
        while ((currentElement != NULL) && (((SEC_OMX_SHAREDMEM_LIST *)(currentElement->nextMemory))->ion_buffer != ion_addr))
            currentElement = currentElement->nextMemory;

        if (((SEC_OMX_SHAREDMEM_LIST *)(currentElement->nextMemory))->ion_buffer == ion_addr) {
            findElement = currentElement->nextMemory;
        } else if (currentElement == NULL) {
            Exynos_OSAL_MutexUnlock(pHandle->SMMutex);
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Exynos_OSAL_MutexUnlock(pHandle->SMMutex);

    pBuffer = findElement->mapAddr;

EXIT:
    return pBuffer;
}

