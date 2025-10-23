import re
import ctypes
import winreg
from typing import Dict, List, Tuple
from ctypes import wintypes, Structure, byref, sizeof, POINTER

from .port import PortDevice
from .win_port import WinPort

HDEVINFO = ctypes.c_void_p
PCTSTR = ctypes.c_wchar_p
DEVINST = wintypes.DWORD
REGSAM = wintypes.DWORD
BYTE = ctypes.c_ubyte

class GUID(Structure):
    _fields_ = [
        ("Data1", wintypes.DWORD),
        ("Data2", wintypes.WORD),
        ("Data3", wintypes.WORD),
        ("Data4", BYTE * 8)
    ]
    def __str__(self):
        data4 = ''.join([f"{b:02x}" for b in self.Data4])
        return f"{self.Data1:08x}-{self.Data2:04x}-{self.Data3:04x}-{data4}"

class SP_DEVINFO_DATA(ctypes.Structure):
    _fields_ = [
        ('cbSize', wintypes.DWORD),
        ('ClassGuid', GUID),
        ('DevInst', DEVINST),
        ('Reserved', POINTER(wintypes.ULONG)),
    ]
class SP_DEVPROPKEY(ctypes.Structure):
    _fields_ = [
        ('fmtid', GUID),
        ('pid', wintypes.ULONG),
    ]

INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value

CR_SUCCESS = 0

DIGCF_PRESENT = 0x2
DIGCF_DEVICEINTERFACE = 0x10

DICS_FLAG_GLOBAL = 1

DIREG_DEV = 0x1

KEY_READ = 0x20019

CM_DRP_LOCATION_INFORMATION = 0x0E
CM_DRP_ADDRESS              = 0x1D
CM_DRP_LOCATION_PATHS       = 0x24
CM_DRP_HARDWAREID           = 0x02
CM_DRP_DEVICEDESC           = 0x01

GUID_USB_HOST_CONTROLLER = GUID(0x3ABF6F2D, 0x71C4, 0x462A, (0x8A, 0x92, 0x1E, 0x68, 0x61, 0xE6, 0xAF, 0x27))

GUID_CLASS_PORTS =  GUID(0x4d36e978, 0xe325, 0x11ce, (0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18))
GUID_CLASS_MODEMS = GUID(0x4d36e96d, 0xe325, 0x11ce, (0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18))

GUID_DEVINTERFACE_COMPORT = GUID(0x86E0D1E0, 0x8089, 0x11D0, (0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73))

DEVPKEY_Device_LocationPaths =         SP_DEVPROPKEY(GUID(0xa45c254e, 0xdf1c, 0x4efd, (0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0)), 37)
DEVPKEY_Device_BusReportedDeviceDesc = SP_DEVPROPKEY(GUID(0x540b947e, 0x8b40, 0x45bc, (0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2)), 4)

PSP_DEVPROPKEY = POINTER(SP_DEVPROPKEY)
PSP_DEVINFO_DATA = POINTER(SP_DEVINFO_DATA)
PDWORD = POINTER(wintypes.DWORD)
PULONG = POINTER(wintypes.ULONG)

class CheckException(Exception):
    def __init__(self, func_name, ret_val):
        super().__init__(f'{func_name}() returned: {ret_val}')

def check_handle(value, func, arguments):
    if value == INVALID_HANDLE_VALUE:
        raise ctypes.WinError()
    return value

def check_success(value, func, arguments):
    if value != CR_SUCCESS:
        raise CheckException(func.__name__, value)
    return value

#-------------------------------------------------------------------------
# cfgmgr32 binding
#-------------------------------------------------------------------------
cfgmgr32 = ctypes.windll.LoadLibrary("cfgmgr32")

CM_Get_DevNode_Registry_Property = cfgmgr32.CM_Get_DevNode_Registry_PropertyW
CM_Get_DevNode_Registry_Property.argtypes = [wintypes.DWORD, wintypes.DWORD, wintypes.LPDWORD, ctypes.c_void_p, wintypes.PDWORD, wintypes.DWORD]
CM_Get_DevNode_Registry_Property.restype = wintypes.LONG
CM_Get_DevNode_Registry_Property.errcheck = check_success

CM_Get_DevNode_Property = cfgmgr32.CM_Get_DevNode_PropertyW
CM_Get_DevNode_Property.argtypes = [wintypes.DWORD, PSP_DEVPROPKEY, wintypes.PULONG, ctypes.c_void_p, wintypes.PULONG, wintypes.ULONG]
CM_Get_DevNode_Property.restype = wintypes.LONG
CM_Get_DevNode_Property.errcheck = check_success


CM_Get_Parent = cfgmgr32.CM_Get_Parent
CM_Get_Parent.argtypes = [wintypes.PDWORD, wintypes.DWORD, wintypes.ULONG]
CM_Get_Parent.restype = wintypes.LONG
CM_Get_Parent.errcheck = check_success

CM_Get_Device_IDW = cfgmgr32.CM_Get_Device_IDW
CM_Get_Device_IDW.argtypes = [wintypes.DWORD, ctypes.c_void_p, wintypes.ULONG, wintypes.ULONG]
CM_Get_Device_IDW.restype = wintypes.LONG
CM_Get_Parent.errcheck = check_success

#-------------------------------------------------------------------------
# setupapi binding
#-------------------------------------------------------------------------
setupapi = ctypes.windll.LoadLibrary("setupapi")

SetupDiGetClassDevs = setupapi.SetupDiGetClassDevsW
SetupDiGetClassDevs.argtypes = [POINTER(GUID), PCTSTR, wintypes.HWND, wintypes.DWORD]
SetupDiGetClassDevs.restype = HDEVINFO
SetupDiGetClassDevs.errcheck = check_handle

SetupDiEnumDeviceInfo = setupapi.SetupDiEnumDeviceInfo
SetupDiEnumDeviceInfo.argtypes = [HDEVINFO, wintypes.DWORD, PSP_DEVINFO_DATA]
SetupDiEnumDeviceInfo.restype = wintypes.BOOL

SetupDiOpenDevRegKey = setupapi.SetupDiOpenDevRegKey
SetupDiOpenDevRegKey.argtypes = [HDEVINFO, PSP_DEVINFO_DATA, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, REGSAM]
SetupDiOpenDevRegKey.restype = wintypes.HKEY
SetupDiOpenDevRegKey.errcheck = check_handle

SetupDiGetDeviceInstanceId = setupapi.SetupDiGetDeviceInstanceIdW
SetupDiGetDeviceInstanceId.argtypes = [HDEVINFO, PSP_DEVINFO_DATA, ctypes.c_void_p, wintypes.DWORD, PDWORD]
SetupDiGetDeviceInstanceId.restype = wintypes.BOOL


def get_dev_property(devinst, prop):
    """Get a CM_DRP_xxx device property"""
    szLocInfo = ctypes.create_unicode_buffer(1024)
    size = wintypes.DWORD(sizeof(szLocInfo) - 1)
    CM_Get_DevNode_Registry_Property(devinst, prop, None, byref(szLocInfo), byref(size), 0)
    return szLocInfo.value


def get_usb_serial_number(devinst, vid=None, pid=None):
    """Get USB serial number by iterating parents until it's found"""
    for _ in range(0,6):
        dev_id = ctypes.create_unicode_buffer(250)

        try:
            CM_Get_Device_IDW(devinst, dev_id, ctypes.sizeof(dev_id) - 1, 0)
        except:
            return None

        m = re.search(r'VID_([0-9A-F]{4})&PID_([0-9A-F]{4})?(&MI_\d{2})?\\(.*)?',
                      dev_id.value.upper(), re.I)

        m_vid = None
        m_pid = None
        serial_number = None
        if m and len(m.groups()) >= 2:
            m_vid = int(m.group(1), 16)
            m_pid = int(m.group(2), 16)
            if vid is None:
                vid = m_vid
            if pid is None:
                pid = m_pid
            if m.group(4):
                serial_number = m.group(4)
                if vid == m_vid and pid == m_pid and not ('&' in serial_number):
                    return serial_number
        # Get the parent device instance.
        parent_devinst = wintypes.DWORD()
        try:
            CM_Get_Parent(ctypes.byref(parent_devinst), devinst, 0)
        except:
            return None
    return None

def enumerate_usb_host_controllers() -> Dict[str, int]:
    """Returns a dict of host controllers.
    Each entry with pci_loc as key and host controller index as value"""
    dev_handle = SetupDiGetClassDevs(byref(GUID_USB_HOST_CONTROLLER), None, None,
                                        (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE))

    devinfo_data = SP_DEVINFO_DATA()
    devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA)
    index = 0
    loc_path_list = []
    while SetupDiEnumDeviceInfo(dev_handle, index, byref(devinfo_data)):
        index += 1
        loc_path = get_dev_property(devinfo_data.DevInst, CM_DRP_LOCATION_PATHS)
        loc_path_list.append(loc_path)

    # To get the host controllers in correct order we must sort them based on location path
    loc_path_list.sort()
    pci_dict = {}
    index = 0
    for loc_path in loc_path_list:
        index += 1
        pci_loc = WinLocation.parse(loc_path).pci_loc
        pci_dict[pci_loc] = index

    return pci_dict

class WinLocation:
    """Helper class for a HW location"""
    usb_ctrl_dict: Dict[str,int] = None

    def __init__(self, pci_loc: str, usb_loc: str = None, interface: int = None):
        self.pci_loc = pci_loc
        self.usb_loc = usb_loc
        self.interface = interface

    def __str__(self):
        loc = self.pci_loc
        if self.usb_loc is not None:
            loc += '-' + self.usb_loc
        if self.interface is not None:
            loc += '/' + str(self.interface)
        return loc

    def to_ftdi_loc(self):
        ftdi_loc = None
        if WinLocation.usb_ctrl_dict is None:
            WinLocation.usb_ctrl_dict = enumerate_usb_host_controllers()

        if self.pci_loc in WinLocation.usb_ctrl_dict:
            ftdi_loc_str = str(WinLocation.usb_ctrl_dict[self.pci_loc])
            ftdi_loc_str += self.usb_loc
            if self.interface is not None:
                ftdi_loc_str += str(self.interface + 1)

            if len(ftdi_loc_str) % 2 != 0:
                # bytes.fromhex() expect each byte to be 2 characters
                # so just pad with leading 0 if needed
                ftdi_loc_str = '0' + ftdi_loc_str
            ftdi_loc = int(ftdi_loc_str, 16)

        return ftdi_loc

    @staticmethod
    def parse(loc_str: str):
        # Example loc_str: "PCIROOT(0)#PCI(1400)#PCI(0000)#USBROOT(4)#USB(2)#USB(1)#USBMI(1)"
        if not loc_str.upper().startswith("PCIROOT"):
            return None
        # Split into separate PCI and USB string (USB is optional)
        # Also replace "#USBROOT" with "#USB",  and "#PCIROOT" with "#PCI"
        # This will give us more homogeneous strings like this:
        # pci_str = "PCI(0)#PCI(1400)#PCI(0000)"
        # usb_str = "USB(4)#USB(2)#USB(1)#USBMI(1)"
        split_str = loc_str.upper().split("#USBROOT")
        pci_str = split_str[0].replace("PCIROOT", "PCI")
        usb_str = f'USB{split_str[1]}' if len(split_str) > 1 else None
        pci_addr = []
        for entry in pci_str.split("#"):
            m = re.search(r'^PCI\(([0-9A-F]+)\)$', entry, re.I)
            if m is None or len(m.groups()) < 1:
                print(f"Unknown PCI location path: {loc_str}")
                return None
            nbr = int(m.group(1), 16)
            pci_addr.append(f'{nbr:X}')
        pci_loc = '.'.join(pci_addr)

        usb_loc = None
        interface = None
        if usb_str:
            usb_loc = ''
            # "#USBMI(xx)" corresponds to the interface so treat that separately
            tmp = usb_str.split("#USBMI")
            first = True
            for entry in tmp[0].split("#"):
                if first:
                    # Skip USB root - it's not used by FTDI USB location
                    first = False
                    continue
                m = re.search(r'^USB\(([0-9A-F]+)\)$', entry, re.I)
                if m is None or len(m.groups()) < 1:
                    print(f"Unknown USB location in: {loc_str}")
                    return None
                usb_loc += f'{int(m.group(1), 16):X}'
            # Now parse USB interface
            if len(tmp) > 1:
                m = re.search(r'^\(([0-9A-F]+)\)$', tmp[1], re.I)
                if m is None or len(m.groups()) < 1:
                    print(f"Unknown USB interface in: {loc_str}")
                    return None
                interface = int(m.group(1))

        return WinLocation(pci_loc, usb_loc, interface)

def get_location(devinst) -> Tuple[WinLocation, int]:
    """Try to find the location path by looking in the 'Location paths' device property
    If no valid location path is found, then check its parent instead.

    Returns the found location and devinst or (None, None) if not found
    """
    for _ in range(0,6):
        try:
            loc_str = get_dev_property(devinst, CM_DRP_LOCATION_PATHS)
            # Try to parse the location string
            # If it has unexpected format we may be looking on a device specific driver instance
            # hence we try again with its parent
            loc = WinLocation.parse(loc_str)
            return loc, devinst
        except:
            pass
        devinst_parent = wintypes.DWORD()
        CM_Get_Parent(byref(devinst_parent), devinst, 0)
        devinst = devinst_parent
    return None, None

def get_description(devinst):
    """Returns the 'Bus reported device description'"""
    prop_buffer = ctypes.create_unicode_buffer(250)
    buffer_size = wintypes.ULONG(sizeof(prop_buffer))
    prop_type = wintypes.ULONG()

    CM_Get_DevNode_Property(devinst,
                            ctypes.byref(DEVPKEY_Device_BusReportedDeviceDesc),
                            ctypes.byref(prop_type),
                            ctypes.byref(prop_buffer),
                            ctypes.byref(buffer_size),
                            0)
    return prop_buffer.value

def get_serial(devinst, devinfo_data):
    """Try to find the serial number"""
    instance_id = ctypes.create_unicode_buffer(250)
    size = wintypes.DWORD(sizeof(instance_id) - 1)
    if SetupDiGetDeviceInstanceId(devinst, byref(devinfo_data), byref(instance_id), size, None):
        instance_id_str: str = instance_id.value.upper()
        m = re.search(r'FTDIBUS\\VID_([0-9A-F]{4})\+PID_([0-9A-F]{4})\+([^\\]*)?\\.*', instance_id_str, re.I)
        if m:
            serial = m.group(3)
            # If no serial is set Windows will generate an ID that looks like this: '5&343580DE&0&4&1'
            # Check first two and last two characters if this is the case
            if re.search(r'^[0-9A-F]\&.*\&[0-9A-F]$', serial):
                return None
            return serial
        elif instance_id_str.startswith("USB"):
            return get_usb_serial_number(devinfo_data.DevInst)
    return None


class WinPortEnumerator:
    @staticmethod
    def enumerate_devices() -> List[PortDevice]:
        desc = None
        dev_handle = SetupDiGetClassDevs(byref(GUID_CLASS_PORTS), None, None, DIGCF_PRESENT)
        devinfo_data = SP_DEVINFO_DATA()
        devinfo_data.cbSize = sizeof(devinfo_data)
        index = 0
        dev_list: List[PortDevice] = []
        usb_dev_dict: Dict[str, PortDevice] = {}
        # Iterate all devices
        while SetupDiEnumDeviceInfo(dev_handle, index, byref(devinfo_data)):
            index += 1

            # Get COM-port name
            hkey = SetupDiOpenDevRegKey(dev_handle,
                                        byref(devinfo_data),
                                        DICS_FLAG_GLOBAL,
                                        0,
                                        DIREG_DEV,
                                        KEY_READ)

            com_port = winreg.QueryValueEx(hkey, "PortName")[0]
            winreg.CloseKey(hkey)

            try:
                # Get port HW location
                location, loc_devinst = get_location(devinfo_data.DevInst)
                # Next get device location (a device can have several ports)
                dev_location = location
                for _ in range(0,4):
                    if dev_location is None or dev_location.interface is None:
                        break
                    # This is an USB composite device. To get device description we need to find the composite parent
                    loc_devinst_parent = wintypes.DWORD()
                    CM_Get_Parent(byref(loc_devinst_parent), loc_devinst, 0)
                    loc_devinst = loc_devinst_parent
                    dev_location, loc_devinst = get_location(loc_devinst_parent)

                desc = get_description(loc_devinst)

            except CheckException:
                loc_devinst = devinfo_data.DevInst
                # Since we can't find the parent, use the port description instead
                desc = get_dev_property(devinfo_data.DevInst, CM_DRP_DEVICEDESC)
                location = None
                dev_location = None

            serial = get_serial(dev_handle, devinfo_data)
            if dev_location:
                dev_loc_str = str(dev_location)
                if dev_loc_str not in usb_dev_dict:
                    dev = PortDevice(desc, None, dev_location) # We'll set the serial later
                    dev_list.append(dev)
                    usb_dev_dict[dev_loc_str] = dev

                dev = usb_dev_dict[dev_loc_str]

            else:
                # device location not found so we can't use usb_dev_dict lookup
                # just create a new device instead
                dev = PortDevice(desc, serial, None)
                dev_list.append(dev)
            iface_number = location.interface if location else None
            dev.ports.append(WinPort(com_port, dev, location=location, serial=serial, iface_number=iface_number))

        for dev in dev_list:
            # Sort device ports by location
            dev.ports = sorted(dev.ports, key=lambda port: str(port.location))

            # FTDI adds a character in the end of serial indicating the the interface number.
            # Ie. for a two channel FTDI device the serial will be:
            # Port 0: UBX8T88E7A
            # Port 1: UBX8T88E7B
            # Check if this is the case and if so remove last char to get the device serial
            dev_serial = dev.ports[0].serial
            if len(dev.ports) > 1 and dev_serial != dev.ports[1].serial:
                if dev_serial[:-1] == dev.ports[1].serial[:-1]:
                    dev_serial = dev_serial[:-1]
                else:
                    dev_serial = 'Unknown'
            dev.serial = dev_serial

        return dev_list
