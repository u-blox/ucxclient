#!/usr/bin/env python3
"""
Apply API fixes to ucxclient-x64.c based on the new UCX API structure changes.
This script fixes the structure member access patterns and comments out unavailable APIs.
"""

import re

def apply_fixes(filename):
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Fix 1: Wi-Fi status structure access - change rspWifiStatusIdInt to rsp.WifiStatusIdInt
    content = re.sub(r'\.rspWifiStatusIdInt\.', r'.rsp.WifiStatusIdInt.', content)
    
    # Fix 2: Wi-Fi status structure access - change rspWifiStatusIdStr to rsp.WifiStatusIdStr  
    content = re.sub(r'\.rspWifiStatusIdStr\.', r'.rsp.WifiStatusIdStr.', content)
    
    # Fix 3: Wi-Fi status structure access - change rspWifiStatusIdMac to rsp.WifiStatusIdMac
    content = re.sub(r'\.rspWifiStatusIdMac\.', r'.rsp.WifiStatusIdMac.', content)
    
    # Fix 4: AP security structure access - change rspSecurityModeWpaVersion to rsp.SecurityModeWpaVersion
    content = re.sub(r'\.rspSecurityModeWpaVersion\.', r'.rsp.SecurityModeWpaVersion.', content)
    
    # Fix 5: AP security structure access - change rspSecurityMode to rsp.SecurityMode  
    content = re.sub(r'\.rspSecurityMode\.', r'.rsp.SecurityMode.', content)
    
    # Fix 6: HTTP function renames
    content = content.replace('uCxHttpHttpSetPath', 'uCxHttpSetRequestPath')
    content = content.replace('uCxHttpHeaderGet_t', 'uCxHttpGetHeader_t')
    content = content.replace('uCxHttpHeaderGet1Begin', 'uCxHttpGetHeader1Begin')
    
    # Fix 7: NTP function renames
    content = content.replace('uCxNetworkTimeGetNtpClientStatus', 'uCxNetworkTimeGetClientEnabled')
    content = content.replace('uCxNetworkTimeSetNtpClientStatus', 'uCxNetworkTimeSetClientEnabled')
    
    with open(filename, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"✓ Applied API fixes to {filename}")
    print("  - Fixed Wi-Fi status structure access patterns")
    print("  - Fixed AP security structure access patterns")
    print("  - Fixed HTTP function names")
    print("  - Fixed NTP function names")

if __name__ == '__main__':
    apply_fixes('examples/ucxclient-x64.c')
