from serial import Serial
from serial.serialutil import SerialException  # Import SerialException
import logging

# Import utilities for prettier error messages
try:
    from utils import println, AnsiColor
    UTILS_AVAILABLE = True
except ImportError:
    UTILS_AVAILABLE = False

# Import TCP wrapper for virtual devices
try:
    from tcp_serial_wrapper import create_tcp_connection
    TCP_SUPPORT_AVAILABLE = True
except ImportError:
    TCP_SUPPORT_AVAILABLE = False

KNOWN_EVKS = {
    'EVK-NORA-W36': {
        'AT': 0,
        'SPI': 1,
        'LOG': 2
    }
}

class PortDevice:
    def __init__(self, desc: str, serial: str = None, location: str = None):
        self.desc = desc
        self.serial = serial
        self.location = location
        self.ports: list[Port] = []
        self.attributes = KNOWN_EVKS[self.desc] if self.desc in KNOWN_EVKS else {}
        # Used for Linux:
        self.bus = -1
        self.device = -1
        self.usb_dev_path = None

    def __str__(self):
        fields = []
        if self.location:
            fields.append(f'location: {self.location}')
        if self.serial:
            fields.append(f'serial: {self.serial}')
        ret = self.desc
        if len(fields):
            ret += f' - {", ".join(fields)}'
        return ret

    def get_interface(self, index: int):
        for p in self.ports:
            if p.iface_number == index:
                return p
            if index == 0 and p.iface_number == None:
                return p
        return None

    def get_at_port(self):
        iface_nbr =  self.attributes.get("AT", None)
        if iface_nbr:
            return self.get_interface(iface_nbr)
        # If device is unknown, just return first port
        return self.ports[0]

    def get_spi_iface_nbr(self):
        return self.attributes.get("SPI", None)

    def print(self):
        print(self)
        for p in self.ports:
            attr_str = None
            for attr, iface_nbr in self.attributes.items():
                if iface_nbr == p.iface_number:
                    attr_str = f'{attr}'
                    break
            print(f'   {p.to_str(attr_str)}')


class Port:
    def __init__(self, port: str, parent: PortDevice,
                 iface_number: int = None,
                 serial: str = None, location: str = None):
        self.port = port
        self.parent = parent
        self.iface_number = iface_number
        self.serial = serial
        self.location = location

    def to_str(self, attr = None):
        fields = []
        if self.location:
            fields.append(f'location: {self.location}')
        if self.serial:
            fields.append(f'serial: {self.serial}')
        attr_str = f' ({attr})' if attr else ''
        ret = f'{self.port}{attr_str}'
        if len(fields):
            ret += f' - {", ".join(fields)}'
        return ret

    def __str__(self):
        return self.to_str()

    def print(self):
        print(self)

    def open(self, baudrate, rtscts, timeout) -> Serial:
        try:
            # Check if this is a TCP connection
            if self.port.startswith("TCP://"):
                if not TCP_SUPPORT_AVAILABLE:
                    raise SerialException("TCP support not available - tcp_serial_wrapper not found")
                # Create TCP connection using wrapper
                tcp_conn = create_tcp_connection(self.port)
                logging.info(f"Opened TCP connection to {self.port}")
                return tcp_conn
            else:
                # Regular serial connection
                return Serial(self.port, baudrate=baudrate, rtscts=rtscts, timeout=timeout)
        except SerialException as e:
            # Enhance the exception with more user-friendly information
            # but don't print anything here - let the calling code handle the display
            
            # Check if it's a permission/access error
            if "Access is denied" in str(e) or "Permission denied" in str(e):
                # Create a more specific exception with helpful context
                enhanced_msg = f"Port {self.port} is busy or in use by another application"
                # Preserve the original exception as the cause
                new_exception = SerialException(enhanced_msg)
                new_exception.__cause__ = e
                new_exception.is_permission_error = True
                raise new_exception
            elif "cannot find" in str(e).lower() or "no such file" in str(e).lower():
                # Port doesn't exist
                enhanced_msg = f"Port {self.port} not found - device may be disconnected"
                new_exception = SerialException(enhanced_msg)
                new_exception.__cause__ = e
                new_exception.is_not_found_error = True
                raise new_exception
            else:
                # Other serial errors - add context but preserve original message
                enhanced_msg = f"Failed to open {self.port}: {str(e)}"
                new_exception = SerialException(enhanced_msg)
                new_exception.__cause__ = e
                raise new_exception


# Special TCP device for virtual connections
class TCPPortDevice(PortDevice):
    def __init__(self, tcp_address: str):
        super().__init__(
            desc=f"Virtual NORA-W36 ({tcp_address})"
        )
        # Set standard NORA-W36 attributes
        self.attributes = {'AT': 0, 'SPI': 1, 'LOG': 2}
        # Create a virtual port for the TCP connection
        tcp_port = Port(f"TCP://{tcp_address}", parent=self)
        self.ports = [tcp_port]
