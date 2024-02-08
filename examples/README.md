# Examples

This directory contains application examples of how to use ucxclient. The examples make use of the [example ports](port/README.md).

| Files                | Description |
| -------------------- | ----------- |
| http_example.c       | Example of doing a HTTP GET request using the uCx API. |
| http_example_no_os.c | Same example as http_example.c but illustrating how it could be done on a bare-metal system with no OS. |

## Building

To build all the examples on a Linux based system, just execute the following in repo root if you prefer Makefile:

```sh
> cmake -S . -B build -G "Unix Makefiles"
> make -C build
```

or if you prefer Ninja:

```sh
> cmake -S . -B build -G "Ninja"
> ninja -C build
```

## Running

### http_example

If you have built http_example using the steps above it will be built for Linux and located in `build/http_example`.
To start it you will need to pass some arguments:

```
http_example <device> <SSID> <WPA_PSK>
  device:  the UART device that is connected to a u-connectXpress module
  SSID:    the Wi-Fi SSID to connect to
  WPA_PSK: the Wi-Fi WPA PSK
```

Example:

```sh
> build/http_example /dev/ttyUSB0 MySSID MyWiFiPasswd
```

### http_example_no_os

Just like http_example this example will also be compiled for Linux, but in this case the UART device, Wi-Fi SSID and PSK are configured using defines.
To set these defines using CMake you can either use `cmake-gui`:

![cmake-gui](/images/cmake-gui.png)

or from command line:

```sh
> cmake -S . -B build -D U_EXAMPLE_UART="/dev/ttyUSB0" -D U_EXAMPLE_SSID="MySSID" -D U_EXAMPLE_WPA_PSK="MyWiFiPasswd"
```

Now you should be able to start the example using:

```sh
> build/http_example_no_os
```
