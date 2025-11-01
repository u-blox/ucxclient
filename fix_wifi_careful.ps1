# Careful replacement of WiFi -> Wi-Fi in user-facing text only
# Preserves API names (uCxWifi*), header files (u_cx_wifi.h), and variable names (gWifiSsid, wifi_ssid, etc.)

$file = "c:\u-blox\ucxclient\examples\ucxclient_win64.c"
$content = Get-Content $file -Raw

# List of exact replacements for user-visible strings
# We'll use specific string replacements to avoid breaking code

$replacements = @(
    @('// Last WiFi SSID', '// Last Wi-Fi SSID'),
    @('// Last WiFi password', '// Last Wi-Fi password'),
    @('  - WiFi: Use \[8\] WiFi menu to scan and connect', '  - Wi-Fi: Use [8] Wi-Fi menu to scan and connect'),
    @('\(requires WiFi\)', '(requires Wi-Fi)'),
    @('NORA-W36 has BT\+WiFi', 'NORA-W36 has BT+Wi-Fi'),
    @('WIFI OPERATIONS:', 'WI-FI OPERATIONS:'),
    @('\[8\] WiFi menu', '[8] Wi-Fi menu'),
    @('Scan for WiFi networks', 'Scan for Wi-Fi networks'),
    @('Connect to WiFi \(SSID and password saved\)', 'Connect to Wi-Fi (SSID and password saved)'),
    @('Active WiFi connection', 'Active Wi-Fi connection'),
    @('Last WiFi SSID and password', 'Last Wi-Fi SSID and password'),
    @('WiFi not working\?', 'Wi-Fi not working?'),
    @('Ensure WiFi is connected', 'Ensure Wi-Fi is connected'),
    @('WiFi:        Available', 'Wi-Fi:        Available'),
    @('Show WiFi status', 'Show Wi-Fi status'),
    @('--- WiFi Menu ---', '--- Wi-Fi Menu ---'),
    @('Requires active WiFi connection!', 'Requires active Wi-Fi connection!'),
    @('WiFi credentials found', 'Wi-Fi credentials found'),
    @('Reconnect to WiFi\?', 'Reconnect to Wi-Fi?'),
    @('WIFI API', 'WI-FI API'),
    @('Set WiFi connection params', 'Set Wi-Fi connection params'),
    @('Connect to WiFi network', 'Connect to Wi-Fi network'),
    @('Disconnect from WiFi', 'Disconnect from Wi-Fi'),
    @('Get WiFi connection status', 'Get Wi-Fi connection status'),
    @('--- WiFi Status ---', '--- Wi-Fi Status ---'),
    @('Failed to get WiFi status', 'Failed to get Wi-Fi status'),
    @('--- WiFi Network Scan ---', '--- Wi-Fi Network Scan ---'),
    @('--- WiFi Connect ---', '--- Wi-Fi Connect ---'),
    @('--- WiFi Disconnect ---', '--- Wi-Fi Disconnect ---'),
    @('Disconnecting from WiFi', 'Disconnecting from Wi-Fi'),
    @('// WiFi test', '// Wi-Fi test'),
    @('// Check if this is a WiFi-capable device', '// Check if this is a Wi-Fi-capable device'),
    @('// Register URC handlers for WiFi network events', '// Register URC handlers for Wi-Fi network events'),
    @('// If WiFi credentials saved', '// If Wi-Fi credentials saved'),
    @('// Auto-connect to saved WiFi', '// Auto-connect to saved Wi-Fi'),
    @('// Save WiFi credentials', '// Save Wi-Fi credentials')
)

foreach ($replacement in $replacements) {
    $content = $content -replace $replacement[0], $replacement[1]
}

Set-Content $file $content -NoNewline
Write-Host "Updated: $file"
