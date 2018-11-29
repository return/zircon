// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_RIGHTS_H_
#define ZIRCON_RIGHTS_H_

#include <stdint.h>

typedef uint32_t zx_rights_t;
#define ZX_RIGHT_NONE             ((zx_rights_t)0u)
#define ZX_RIGHT_DUPLICATE        ((zx_rights_t)1u << 0)
#define ZX_RIGHT_TRANSFER         ((zx_rights_t)1u << 1)
#define ZX_RIGHT_READ             ((zx_rights_t)1u << 2)
#define ZX_RIGHT_WRITE            ((zx_rights_t)1u << 3)
#define ZX_RIGHT_EXECUTE          ((zx_rights_t)1u << 4)
#define ZX_RIGHT_MAP              ((zx_rights_t)1u << 5)
#define ZX_RIGHT_GET_PROPERTY     ((zx_rights_t)1u << 6)
#define ZX_RIGHT_SET_PROPERTY     ((zx_rights_t)1u << 7)
#define ZX_RIGHT_ENUMERATE        ((zx_rights_t)1u << 8)
#define ZX_RIGHT_DESTROY          ((zx_rights_t)1u << 9)
#define ZX_RIGHT_SET_POLICY       ((zx_rights_t)1u << 10)
#define ZX_RIGHT_GET_POLICY       ((zx_rights_t)1u << 11)
#define ZX_RIGHT_SIGNAL           ((zx_rights_t)1u << 12)
#define ZX_RIGHT_SIGNAL_PEER      ((zx_rights_t)1u << 13)
#define ZX_RIGHT_WAIT             ((zx_rights_t)1u << 14)
#define ZX_RIGHT_INSPECT          ((zx_rights_t)1u << 15)
#define ZX_RIGHT_MANAGE_JOB       ((zx_rights_t)1u << 16)
#define ZX_RIGHT_MANAGE_PROCESS   ((zx_rights_t)1u << 17)
#define ZX_RIGHT_MANAGE_THREAD    ((zx_rights_t)1u << 18)
#define ZX_RIGHT_APPLY_PROFILE    ((zx_rights_t)1u << 19)
#define ZX_RIGHT_SAME_RIGHTS      ((zx_rights_t)1u << 31)

// Convenient names for commonly grouped rights.
#define ZX_RIGHTS_BASIC (ZX_RIGHT_TRANSFER | ZX_RIGHT_DUPLICATE | ZX_RIGHT_WAIT | ZX_RIGHT_INSPECT)

#define ZX_RIGHTS_IO (ZX_RIGHT_READ | ZX_RIGHT_WRITE)

#define ZX_RIGHTS_PROPERTY (ZX_RIGHT_GET_PROPERTY | ZX_RIGHT_SET_PROPERTY)

#define ZX_RIGHTS_POLICY (ZX_RIGHT_GET_POLICY | ZX_RIGHT_SET_POLICY)

#define ZX_DEFAULT_CHANNEL_RIGHTS                                                                  \
    ((ZX_RIGHTS_BASIC & (~ZX_RIGHT_DUPLICATE)) | ZX_RIGHTS_IO | ZX_RIGHT_SIGNAL |                  \
     ZX_RIGHT_SIGNAL_PEER)

#define ZX_DEFAULT_EVENT_RIGHTS (ZX_RIGHTS_BASIC | ZX_RIGHT_SIGNAL)

#define ZX_DEFAULT_EVENTPAIR_RIGHTS (ZX_RIGHTS_BASIC | ZX_RIGHT_SIGNAL | ZX_RIGHT_SIGNAL_PEER)

#define ZX_DEFAULT_FIFO_RIGHTS \
    (ZX_RIGHTS_BASIC | ZX_RIGHTS_IO |\
     ZX_RIGHT_SIGNAL | ZX_RIGHT_SIGNAL_PEER)

#define ZX_DEFAULT_GUEST_RIGHTS \
    (ZX_RIGHT_TRANSFER | ZX_RIGHT_DUPLICATE | ZX_RIGHT_WRITE |\
     ZX_RIGHT_INSPECT | ZX_RIGHT_MANAGE_PROCESS)

#define ZX_DEFAULT_INTERRUPT_RIGHTS (ZX_RIGHTS_BASIC | ZX_RIGHTS_IO | ZX_RIGHT_SIGNAL)

#define ZX_DEFAULT_IO_MAPPING_RIGHTS \
    (ZX_RIGHT_READ | ZX_RIGHT_INSPECT)

#define ZX_DEFAULT_JOB_RIGHTS                                                                      \
    (ZX_RIGHTS_BASIC | ZX_RIGHTS_IO | ZX_RIGHTS_PROPERTY | ZX_RIGHTS_POLICY | ZX_RIGHT_ENUMERATE | \
     ZX_RIGHT_DESTROY | ZX_RIGHT_SIGNAL | ZX_RIGHT_MANAGE_JOB | ZX_RIGHT_MANAGE_PROCESS)

#define ZX_DEFAULT_LOG_RIGHTS \
    (ZX_RIGHTS_BASIC | ZX_RIGHT_WRITE | ZX_RIGHT_SIGNAL)

#define ZX_DEFAULT_PCI_DEVICE_RIGHTS \
    (ZX_RIGHTS_BASIC | ZX_RIGHTS_IO)

#define ZX_DEFAULT_PCI_INTERRUPT_RIGHTS \
    (ZX_RIGHTS_BASIC  | ZX_RIGHTS_IO | ZX_RIGHT_SIGNAL)

#define ZX_DEFAULT_PORT_RIGHTS \
    ((ZX_RIGHTS_BASIC & (~ZX_RIGHT_WAIT)) | ZX_RIGHTS_IO)

#define ZX_DEFAULT_PROCESS_RIGHTS                                                                  \
    (ZX_RIGHTS_BASIC | ZX_RIGHTS_IO | ZX_RIGHTS_PROPERTY | ZX_RIGHT_ENUMERATE | ZX_RIGHT_DESTROY | \
     ZX_RIGHT_SIGNAL | ZX_RIGHT_MANAGE_PROCESS | ZX_RIGHT_MANAGE_THREAD)

#define ZX_DEFAULT_RESOURCE_RIGHTS \
    (ZX_RIGHT_TRANSFER | ZX_RIGHT_DUPLICATE | ZX_RIGHT_WRITE |\
     ZX_RIGHT_INSPECT)

#define ZX_DEFAULT_SOCKET_RIGHTS \
    (ZX_RIGHTS_BASIC | ZX_RIGHTS_IO | ZX_RIGHT_GET_PROPERTY |\
     ZX_RIGHT_SET_PROPERTY | ZX_RIGHT_SIGNAL | ZX_RIGHT_SIGNAL_PEER)

#define ZX_DEFAULT_THREAD_RIGHTS \
    (ZX_RIGHTS_BASIC | ZX_RIGHTS_IO | ZX_RIGHTS_PROPERTY |\
     ZX_RIGHT_DESTROY | ZX_RIGHT_SIGNAL | ZX_RIGHT_MANAGE_THREAD)

#define ZX_DEFAULT_TIMER_RIGHTS (ZX_RIGHTS_BASIC | ZX_RIGHT_WRITE | ZX_RIGHT_SIGNAL)

#define ZX_DEFAULT_VCPU_RIGHTS \
    (ZX_RIGHTS_BASIC | ZX_RIGHTS_IO | ZX_RIGHT_EXECUTE | ZX_RIGHT_SIGNAL)

#define ZX_DEFAULT_VMAR_RIGHTS \
    (ZX_RIGHTS_BASIC & (~ZX_RIGHT_WAIT))

#define ZX_DEFAULT_VMO_RIGHTS\
    (ZX_RIGHTS_BASIC | ZX_RIGHTS_IO | ZX_RIGHTS_PROPERTY |\
     ZX_RIGHT_EXECUTE | ZX_RIGHT_MAP | ZX_RIGHT_SIGNAL)

#define ZX_DEFAULT_IOMMU_RIGHTS \
    (ZX_RIGHTS_BASIC & (~ZX_RIGHT_WAIT))

#define ZX_DEFAULT_BTI_RIGHTS \
    ((ZX_RIGHTS_BASIC & (~ZX_RIGHT_WAIT)) | ZX_RIGHTS_IO | ZX_RIGHT_MAP)

#define ZX_DEFAULT_PROFILE_RIGHTS \
    ((ZX_RIGHTS_BASIC & (~ZX_RIGHT_WAIT)) | ZX_RIGHT_APPLY_PROFILE)

#define ZX_DEFAULT_PMT_RIGHTS \
    (ZX_RIGHT_INSPECT)

#define ZX_DEFAULT_SUSPEND_TOKEN_RIGHTS \
    (ZX_RIGHT_TRANSFER | ZX_RIGHT_INSPECT)

#endif // ZIRCON_RIGHTS_H_
