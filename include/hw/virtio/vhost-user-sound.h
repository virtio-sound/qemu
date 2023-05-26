/*
 * Vhost-user sound virtio device
 *
 * Copyright 2020 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.  See the COPYING file in the
 * top-level directory.
 */

#ifndef QEMU_VHOST_USER_SOUND_H
#define QEMU_VHOST_USER_SOUND_H

#include "hw/virtio/vhost.h"
#include "hw/virtio/vhost-user.h"
#include "qom/object.h"
#include "standard-headers/linux/virtio_snd.h"

#define TYPE_VHOST_USER_SOUND "vhost-user-sound"
OBJECT_DECLARE_SIMPLE_TYPE(VHostUserSound, VHOST_USER_SOUND)

typedef struct {
    CharBackend chardev;
} VHostUserSoundConf;

struct VHostUserSound {
    /*< private >*/
    VirtIODevice parent;
    VhostUserState vhost_user;
    VHostUserSoundConf conf;
    struct virtio_snd_config config;
    struct vhost_dev vhost_dev;
    VirtQueue *ctrl_vq;
    VirtQueue *event_vq;
    VirtQueue *tx_vq;
    VirtQueue *rx_vq;
    /*< public >*/
};

#endif /* QEMU_VHOST_USER_VSOCK_H */



