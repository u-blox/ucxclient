import os
if os.name == 'nt':
    from .win_port_enum import WinPortEnumerator as _PortEnum
elif os.name == 'test':
    class _PortEnum:
        @staticmethod
        def enumerate_devices() -> List[PortDevice]:
            return []
else:
    from .linux_port_enum import LinuxPortEnumerator as _PortEnum

from .port import Port, PortDevice

def enumerate_devices():
    return _PortEnum.enumerate_devices()

def find_port(port: str = None, location: str = None, serial: str = None) -> Port:
    """Find a port based on port name, location or serial number.
    If you don't set any of the port/location/serial args, then the first port
    found will be returned (or None if no port is found)
    """
    dev_list = _PortEnum.enumerate_devices()
    if not (port or location or serial):
        # No filter is given - return first device (if any)
        return None if len(dev_list) == 0 else dev_list[0].get_at_port()

    # First check if any port match
    for dev in dev_list:
        for p in dev.ports:
            if port and p.port == port:
                return p
            if location and str(p.location) == location:
                return p
            if serial and p.serial == serial:
                return p

        # No port matched, now check for device match
        if location and str(dev.location) == location:
            return dev.get_at_port()
        if serial and dev.serial == serial:
            return dev.get_at_port()

    if port:
        # We have not found any matching port
        # However, if the 'port' filter is used we still want to return a Port
        # as user may want to open a device that the enumerator cannot find.
        dev = PortDevice("Unknown")
        devPort = Port(port, dev)
        dev.ports = devPort
        return devPort
    return None
