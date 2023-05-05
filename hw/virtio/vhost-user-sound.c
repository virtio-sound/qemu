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

static Property vus_properties[] = {
    DEFINE_PROP_CHR("chardev", VHostUserSound, conf.chardev),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vus_vmstate = {
    .name = "vhost-user-sound",
    .unmigratable = 1,
};

static void vus_device_realize(DeviceState *dev, Error **errp)
{
    error_report("holaaaa");
    /*
    VHostVSockCommon *vvc = VHOST_VSOCK_COMMON(dev);
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VHostUserVSock *vsock = VHOST_USER_VSOCK(dev);
    int ret;

    if (!vsock->conf.chardev.chr) {
        error_setg(errp, "missing chardev");
        return;
    }

    if (!vhost_user_init(&vsock->vhost_user, &vsock->conf.chardev, errp)) {
        return;
    }

    vhost_vsock_common_realize(vdev);

    vhost_dev_set_config_notifier(&vvc->vhost_dev, &vsock_ops);

    ret = vhost_dev_init(&vvc->vhost_dev, &vsock->vhost_user,
                         VHOST_BACKEND_TYPE_USER, 0, errp);
    if (ret < 0) {
        goto err_virtio;
    }

    ret = vhost_dev_get_config(&vvc->vhost_dev, (uint8_t *)&vsock->vsockcfg,
                               sizeof(struct virtio_vsock_config), errp);
    if (ret < 0) {
        goto err_vhost_dev;
    }

    return;

err_vhost_dev:
    vhost_dev_cleanup(&vvc->vhost_dev);
err_virtio:
    vhost_vsock_common_unrealize(vdev);
    vhost_user_cleanup(&vsock->vhost_user);
    return;
    */
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