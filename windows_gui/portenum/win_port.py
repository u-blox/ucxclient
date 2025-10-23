import time
from serial import Serial
from ftd2xx import ftd2xx

from .port import Port

class WinPort(Port):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.ftdi_loc = None
        if self.location:
            self.ftdi_loc = self.location.to_ftdi_loc()

    def open(self, baudrate, rtscts, timeout) -> Serial:
        return SerialWrapper(self, baudrate=baudrate, rtscts=rtscts, timeout=timeout)

class SerialWrapper:
    """The purpose of this wrapper is to check if the serial port that is about to be
    opened is an FTDI device. If this is the case this wrapper will use the FTDI D2XX
    driver instead of PySerial. By using D2XX we can set the FTDI latency timer which
    is not available for PySerial.

    For Linux this isn't needed as there we can use PySerial.set_low_latency_mode()
    """
    def __init__(self, winport: WinPort, baudrate, rtscts, timeout):
        self.pyser: Serial = None
        self.ftdev: ftd2xx.FTD2XX = None
        self.winport: WinPort = winport
        self._timeout = timeout
        self._write_timeout = 0
        ftdev = None
        ftdi_loc = self.winport.ftdi_loc
        if ftdi_loc:
            try:
                ftdev = ftd2xx.openEx(ftdi_loc, ftd2xx.defines.OPEN_BY_LOCATION)
            except:
                pass

        if ftdev:
            self.ftdev = ftdev
            ftdev.resetDevice()
            ftdev.setBaudRate(baudrate)

            # Configure flow control
            FT_FLOW_NONE = 0x0000
            FT_FLOW_RTS_CTS = 0x0100
            flow = FT_FLOW_RTS_CTS if rtscts else FT_FLOW_NONE
            ftdev.setFlowControl(flow)

            # Configure data format
            FT_BITS_8 = 8
            FT_STOP_BITS_1 = 0
            FT_PARITY_NONE = 0
            ftdev.setDataCharacteristics(FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE)

            # This will line will set both read and write timeout
            self._update_ftdi_timeout()
        else:
            self.pyser = Serial(port=winport.port, baudrate=baudrate, rtscts=rtscts, timeout=timeout)

    def _update_ftdi_timeout(self):
        read_timeout = int(self._timeout * 1000)
        write_timeout = int(self._write_timeout * 1000)
        self.ftdev.setTimeouts(read_timeout, write_timeout)

    @property
    def write_timeout(self):
        """Get the current write timeout setting."""
        return self._write_timeout

    @write_timeout.setter
    def write_timeout(self, timeout):
        """Change write timeout setting."""
        self._write_timeout = timeout
        if self.ftdev:
            self._update_ftdi_timeout()
        else:
            self.pyser.write_timeout = timeout

    @property
    def timeout(self):
        """Get the current read timeout setting."""
        return self._timeout

    @property
    def rts(self):
        raise NotImplemented()

    @rts.setter
    def rts(self, value):
        if self.pyser:
            self.pyser.rts = value
        else:
            if value:
                self.ftdev.setRts()
            else:
                self.ftdev.clrRts()

    @property
    def dtr(self):
        raise NotImplemented()

    @dtr.setter
    def dtr(self, value):
        if self.pyser:
            self.pyser.dtr = value
        else:
            if value:
                self.ftdev.setDtr()
            else:
                self.ftdev.clrDtr()

    @timeout.setter
    def timeout(self, timeout):
        """Change read timeout setting."""
        self._timeout = timeout
        if self.ftdev:
            self._update_ftdi_timeout()
        else:
            if self.pyser.timeout != timeout:
                # Workaround for Windows
                # If you change timeout directly after a write the outgoing data can
                # be corrupted since modifying timeout will reinitialize the serial port:
                # https://github.com/pyserial/pyserial/issues/394
                # So just make sure we sleep a little before changing the timeout
                time.sleep(0.1)
                self.pyser.timeout = timeout

    def reset_input_buffer(self):
        """Clear input buffer, discarding all that is in the buffer."""
        if self.ftdev:
            byte_cnt = self.ftdev.getQueueStatus()
            self.ftdev.read(byte_cnt)
        else:
            self.pyser.reset_input_buffer()

    def write(self, data):
        """Output the given byte string over the serial port."""
        if self.ftdev:
            self.ftdev.write(bytes(data))
        else:
            self.pyser.write(data)

    def read(self, size=1):
        """\
        Read size bytes from the serial port. If a timeout is set it may
        return less characters as requested. With no timeout it will block
        until the requested number of bytes is read.
        """
        if self.ftdev:
            return self.ftdev.read(size)
        else:
            return self.pyser.read(size)

    def close(self):
        """Close port"""
        if self.ftdev:
            self.ftdev.close()
        else:
            self.pyser.close()

    def set_low_latency_mode(self, low_latency_settings):
        if self.ftdev:
            if low_latency_settings:
                self.ftdev.setLatencyTimer(1)
            else:
                self.ftdev.setLatencyTimer(16)
