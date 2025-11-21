#!/bin/bash
# Setup udev rules for ST-Link debugging without sudo
# Run with: sudo ./setup_stlink_udev.sh

set -e

echo "Installing udev rules for ST-Link devices..."

cat << 'EOF' > /etc/udev/rules.d/49-stlinkv2.rules
# ST-Link v2 and v2-1
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666", TAG+="uaccess"
# ST-Link v2-1
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666", TAG+="uaccess"
# ST-Link v3
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374d", MODE="0666", TAG+="uaccess"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374e", MODE="0666", TAG+="uaccess"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374f", MODE="0666", TAG+="uaccess"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3753", MODE="0666", TAG+="uaccess"
EOF

echo "Reloading udev rules..."
udevadm control --reload-rules
udevadm trigger

echo "Done! Please unplug and replug your ST-Link device."
echo "You should now be able to use st-util without sudo."
