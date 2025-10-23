from typing import Dict, List
from serial.tools import list_ports

from .port import Port, PortDevice

def read_file_as_str(file):
    with open(file) as f:
        return f.readline().strip()

def read_file_as_int(file):
    return int(read_file_as_str(file))

class LinuxPortEnumerator:
    @staticmethod
    def enumerate_devices() -> List[PortDevice]:
        dev_list: List[PortDevice] = []
        usb_dev_dict: Dict[str, PortDevice] = {}
        for p in list_ports.comports():
            if p.usb_device_path is None:
                continue
            if p.usb_device_path not in usb_dev_dict:
                desc = read_file_as_str(f'{p.usb_device_path}/product')

                dev = PortDevice(desc, p.serial_number, p.location.split(':')[0])
                dev_list.append(dev)
                usb_dev_dict[p.usb_device_path] = dev
                # USB bus and device number are needed when opening device using pyftdi
                dev.bus = read_file_as_int(f'{p.usb_device_path}/busnum')
                dev.device = read_file_as_int(f'{p.usb_device_path}/devnum')
                dev.usb_dev_path = p.usb_device_path
            dev = usb_dev_dict[p.usb_device_path]
            # A USB device can have several port. We identify each port using the USB interface number
            # Some USB devices don't have interface number, in this case we leave it empty
            if "." in p.location:
                iface_number = p.location.split('.')[-1]
                p_loc = f'{dev.location}/{iface_number}' if dev.location else None
                p_ser = f'{dev.serial}/{iface_number}' if dev.serial else None
            else:
                iface_number = ''
                p_loc = f'{dev.location}' if dev.location else None
                p_ser = f'{dev.serial}' if dev.serial else None
            dev.ports.append(Port(p.device, dev, iface_number, location=p_loc, serial=p_ser))

        for dev in dev_list:
            # Sort device ports by location
            dev.ports = sorted(dev.ports, key=lambda port: port.location)

        return dev_list
