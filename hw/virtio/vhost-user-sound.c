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
#include "hw/virtio/virtio-bus.h"
#include "hw/qdev-properties-system.h"
#include "hw/virtio/vhost-user-sound.h"
#include "standard-headers/linux/virtio_ids.h"

static const int user_feature_bits[] = {
    VIRTIO_F_VERSION_1,
    VIRTIO_RING_F_INDIRECT_DESC,
    VIRTIO_RING_F_EVENT_IDX,
    VIRTIO_F_NOTIFY_ON_EMPTY,
    VHOST_INVALID_FEATURE_BIT
};

static Property vus_properties[] = {
    DEFINE_PROP_CHR("chardev", VHostUserSound, conf.chardev),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vus_vmstate = {
    .name = "vhost-user-sound",
    .unmigratable = 1,
};

static void vus_get_config(VirtIODevice *vdev, uint8_t *config)
{
    VHostUserSound *snd = VHOST_USER_SOUND(vdev);

    memcpy(config, &snd->config, sizeof(struct virtio_snd_config));
}

static void vus_start(VirtIODevice *vdev)
{
    VHostUserSound *snd = VHOST_USER_SOUND(vdev);
    BusState *qbus = BUS(qdev_get_parent_bus(DEVICE(vdev)));
    VirtioBusClass *k = VIRTIO_BUS_GET_CLASS(qbus);
    int ret;
    int i;

    if (!k->set_guest_notifiers) {
        error_report("binding does not support guest notifiers");
        return;
    }

    ret = vhost_dev_enable_notifiers(&snd->vhost_dev, vdev);
    if (ret < 0) {
        error_report("Error enabling host notifiers: %d", -ret);
        return;
    }

    ret = k->set_guest_notifiers(qbus->parent, snd->vhost_dev.nvqs, true);
    if (ret < 0) {
        error_report("Error binding guest notifier: %d", -ret);
        goto err_host_notifiers;
    }

    snd->vhost_dev.acked_features = vdev->guest_features;
    ret = vhost_dev_start(&snd->vhost_dev, vdev, true);
    if (ret < 0) {
        error_report("Error starting vhost: %d", -ret);
        goto err_guest_notifiers;
    }

    /*
     * guest_notifier_mask/pending not used yet, so just unmask
     * everything here.  virtio-pci will do the right thing by
     * enabling/disabling irqfd.
     */
    for (i = 0; i < snd->vhost_dev.nvqs; i++) {
        vhost_virtqueue_mask(&snd->vhost_dev, vdev, i, false);
    }

    return;

err_guest_notifiers:
    k->set_guest_notifiers(qbus->parent, snd->vhost_dev.nvqs, false);
err_host_notifiers:
    vhost_dev_disable_notifiers(&snd->vhost_dev, vdev);
    return;
}

static void vus_stop(VirtIODevice *vdev)
{
    VHostUserSound *snd = VHOST_USER_SOUND(vdev);
    BusState *qbus = BUS(qdev_get_parent_bus(DEVICE(vdev)));
    VirtioBusClass *k = VIRTIO_BUS_GET_CLASS(qbus);
    int ret;

    if (!k->set_guest_notifiers) {
        return;
    }

    vhost_dev_stop(&snd->vhost_dev, vdev, true);

    ret = k->set_guest_notifiers(qbus->parent, snd->vhost_dev.nvqs, false);
    if (ret < 0) {
        error_report("vhost guest notifier cleanup failed: %d", ret);
        return;
    }

    vhost_dev_disable_notifiers(&snd->vhost_dev, vdev);
}

static void vus_set_status(VirtIODevice *vdev, uint8_t status)
{
    VHostUserSound *snd = VHOST_USER_SOUND(vdev);
    bool should_start = virtio_device_should_start(vdev, status);

    if (vhost_dev_is_started(&snd->vhost_dev) == should_start) {
        return;
    }

    if (should_start) {
        vus_start(vdev);
    } else {
        vus_stop(vdev);
    }
}

static uint64_t vus_get_features(VirtIODevice *vdev,
                                 uint64_t features,
                                 Error **errp)
{
    VHostUserSound *snd = VHOST_USER_SOUND(vdev);

    return vhost_get_features(&snd->vhost_dev, user_feature_bits, features);
}

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
    return;
}

static void vus_device_unrealize(DeviceState *dev)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VHostUserSound *snd = VHOST_USER_SOUND(dev);

    /* This will stop vhost backend if appropriate. */
    vus_set_status(vdev, 0);

    vhost_dev_cleanup(&snd->vhost_dev);

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
    vdc->unrealize = vus_device_unrealize;
    vdc->get_features = vus_get_features;
    vdc->get_config = vus_get_config;
    vdc->set_status = vus_set_status;
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