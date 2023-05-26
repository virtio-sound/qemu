/*
 * Vhost-user sound PCI Bindings
 *
 * Copyright 2020 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.  See the COPYING file in the
 * top-level directory.
 */

#include "qemu/osdep.h"

#include "hw/virtio/virtio-pci.h"
#include "hw/qdev-properties.h"
#include "hw/virtio/vhost-user-sound.h"
#include "qom/object.h"

typedef struct VHostUserSoundPCI VHostUserSoundPCI;

/*
 * vhost-user-sound-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VHOST_USER_SOUND_PCI "vhost-user-sound-pci-base"
DECLARE_INSTANCE_CHECKER(VHostUserSoundPCI, VHOST_USER_SOUND_PCI,
                         TYPE_VHOST_USER_SOUND_PCI)

struct VHostUserSoundPCI {
    VirtIOPCIProxy parent_obj;
    VHostUserSound vdev;
};

static Property vhost_user_sound_pci_properties[] = {
    DEFINE_PROP_UINT32("vectors", VirtIOPCIProxy, nvectors, 3),
    DEFINE_PROP_END_OF_LIST(),
};

static void vhost_user_sound_pci_instance_init(Object *obj)
{
    VHostUserSoundPCI *dev = VHOST_USER_SOUND_PCI(obj);

    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_VHOST_USER_SOUND);
}

static void vhost_user_sound_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
    VHostUserSoundPCI *dev = VHOST_USER_SOUND_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);

    /* set modern only */
    virtio_pci_force_virtio_1(vpci_dev);

    qdev_realize(vdev, BUS(&vpci_dev->bus), errp);
}

static void vhost_user_sound_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);
    k->realize = vhost_user_sound_pci_realize;
    set_bit(DEVICE_CATEGORY_SOUND, dc->categories);
    device_class_set_props(dc, vhost_user_sound_pci_properties);
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_SOUND;
    pcidev_k->revision = 0x00;
    pcidev_k->class_id = PCI_CLASS_MULTIMEDIA_AUDIO;
}

static const VirtioPCIDeviceTypeInfo vhost_user_sound_pci_info = {
    .base_name             = TYPE_VHOST_USER_SOUND_PCI,
    .generic_name          = "vhost-user-sound-pci",
    .non_transitional_name = "vhost-user-sound-pci-non-transitional",
    .instance_size = sizeof(VHostUserSoundPCI),
    .instance_init = vhost_user_sound_pci_instance_init,
    .class_init    = vhost_user_sound_pci_class_init,
};

static void virtio_pci_vhost_register(void)
{
    virtio_pci_types_register(&vhost_user_sound_pci_info);
}

type_init(virtio_pci_vhost_register)