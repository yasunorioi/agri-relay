// usb_ncm_init.cpp — Override TinyUSB_Device_Init via --wrap linker flag
// The linker renames TinyUSB_Device_Init → __real_TinyUSB_Device_Init
// and our __wrap_TinyUSB_Device_Init becomes the active implementation
//
// We replicate the logic from Adafruit_USBD_Device::begin() because:
//   1. begin() calls clearConfiguration() which would erase NCM
//   2. _desc_device is private with no public setter for device class
// So we set IAD class fields via offsetof on the device descriptor.

#include <Adafruit_TinyUSB.h>

extern void TinyUSB_Port_InitDevice(uint8_t rhport);
extern Adafruit_USBD_CDC SerialTinyUSB;
extern bool usb_ncm_dev_begin(void);

extern "C" {

void __wrap_TinyUSB_Device_Init(uint8_t rhport) {
  // 1. Clear descriptor
  TinyUSBDevice.clearConfiguration();

  // 2. Set IAD composite device class (required for multi-function USB)
  //    Adafruit_USBD_Device::_desc_device is the first data member (no vtable),
  //    so it starts at offset 0 of the TinyUSBDevice object.
  //    tusb_desc_device_t is packed 18 bytes: bDeviceClass at offset 4.
  tusb_desc_device_t *dd = reinterpret_cast<tusb_desc_device_t *>(&TinyUSBDevice);
  dd->bDeviceClass    = TUSB_CLASS_MISC;        // 0xEF
  dd->bDeviceSubClass = MISC_SUBCLASS_COMMON;   // 0x02
  dd->bDeviceProtocol = MISC_PROTOCOL_IAD;      // 0x01

  // 3. CDC-ACM (Serial)
  SerialTinyUSB.begin(115200);

  // 4. NCM interface — MUST be before tud_init()
  usb_ncm_dev_begin();

  // 5. Init USB hardware — descriptors frozen after this
  TinyUSB_Port_InitDevice(rhport);
}

}  // extern "C"
