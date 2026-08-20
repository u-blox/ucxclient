# List serial ports with friendly names.
Get-CimInstance Win32_PnPEntity |
    Where-Object { $_.Name -match 'COM\d+' } |
    ForEach-Object { $_.Name }
