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

#include "hw/virtio/vhost-user.h"
#include "qom/object.h"

#define TYPE_VHOST_USER_SOUND "vhost-user-sound"
OBJECT_DECLARE_SIMPLE_TYPE(VHostUserSound, VHOST_USER_SOUND)

typedef struct {
    CharBackend chardev;
} VHostUserSoundConf;

struct VHostUserSound {
    /*< private >*/
    // Shoud I use common like in vsock? What is the difference?
    VirtIODevice parent;
    /*
    VhostUserState vhost_user;
    */
    VHostUserSoundConf conf;
    /*
    struct virtio_vsock_config vsockcfg;
    */
    /*< public >*/
};

#endif /* QEMU_VHOST_USER_VSOCK_H */



