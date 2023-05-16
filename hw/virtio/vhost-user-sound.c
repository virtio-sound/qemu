/*
 * Vhost-user sound virtio device
 *
 * Copyright 2020 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.  See the COPYING file in the
 * top-level directory.
 */

#include "qemu/osdep.h"

#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/virtio/vhost-user-sound.h"
#include "standard-headers/linux/virtio_ids.h"

static Property vus_properties[] = {
    DEFINE_PROP_CHR("chardev", VHostUserSound, conf.chardev),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vus_vmstate = {
    .name = "vhost-user-sound",
    .unmigratable = 1,
};

static void vus_snd_handle_output(VirtIODevice *vdev, VirtQueue *vq)
{
    /*
     * Not normally called; it's the daemon that handles the queue;
     * however virtio's cleanup path can call this.
     */
}

static void vus_device_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VHostUserSound *snd = VHOST_USER_SOUND(dev);
    int ret;

    if (!snd->conf.chardev.chr) {
        error_setg(errp, "missing chardev");
        return;
    }
    if (!vhost_user_init(&snd->vhost_user, &snd->conf.chardev, errp)) {
        return;
    }

    virtio_init(vdev, VIRTIO_ID_SOUND, sizeof(snd->config));

    /* add queues */
    snd->ctrl_vq = virtio_add_queue(vdev, 64, vus_snd_handle_output);
    snd->event_vq = virtio_add_queue(vdev, 64, vus_snd_handle_output);
    snd->tx_vq = virtio_add_queue(vdev, 64, vus_snd_handle_output);
    snd->rx_vq = virtio_add_queue(vdev, 64, vus_snd_handle_output);
    snd->vhost_dev.nvqs = 4;
    snd->vhost_dev.vqs = g_new0(struct vhost_virtqueue, snd->vhost_dev.nvqs);
    ret = vhost_dev_init(&snd->vhost_dev, &snd->vhost_user,
                         VHOST_BACKEND_TYPE_USER, 0, errp);
    if (ret < 0) {
        error_setg_errno(errp, -ret, "vhost_dev_init() failed");
        goto vhost_dev_init_failed;
    }

    return;

vhost_dev_init_failed:
    vhost_user_cleanup(&snd->vhost_user);
    virtio_delete_queue(snd->ctrl_vq);
    virtio_delete_queue(snd->event_vq);
    virtio_delete_queue(snd->tx_vq);
    virtio_delete_queue(snd->rx_vq);
    virtio_cleanup(vdev);
    g_free(snd->vhost_dev.vqs);
    snd->vhost_dev.vqs = NULL;
}

static void vus_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

    device_class_set_props(dc, vus_properties);
    dc->vmsd = &vus_vmstate;
    vdc->realize = vus_device_realize;
    /*
    TODO: add these elements!
    vdc->unrealize = vuv_device_unrealize;
    vdc->get_features = vuv_get_features;
    vdc->get_config = vuv_get_config;
    vdc->set_status = vuv_set_status;
    */
}

static const TypeInfo vus_info = {
    .name = TYPE_VHOST_USER_SOUND,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VHostUserSound),
    .class_init = vus_class_init,
};

static void vus_register_types(void)
{
    type_register_static(&vus_info);
}

type_init(vus_register_types)