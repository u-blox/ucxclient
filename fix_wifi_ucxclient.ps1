# Replace WiFi with Wi-Fi in ucxclient_win64.c (user-facing text only)
$file = "c:\u-blox\ucxclient\examples\ucxclient_win64.c"
$content = Get-Content $file -Raw

# Replace WiFi/wifi in printf/fprintf strings only (not in variable/function names)
# Pattern: find printf or fprintf, then within the quoted string replace WiFi/wifi
$content = $content -replace '(printf\([^)]*?"[^"]*?)WiFi([^"]*?"[^)]*?\))', '$1Wi-Fi$2'
$content = $content -replace '(fprintf\([^)]*?"[^"]*?)WiFi([^"]*?"[^)]*?\))', '$1Wi-Fi$2'

# Handle multi-line strings by doing multiple passes
for ($i = 0; $i -lt 5; $i++) {
    $content = $content -replace '(printf\([^)]*?"[^"]*?)WiFi([^"]*?"[^)]*?\))', '$1Wi-Fi$2'
    $content = $content -replace '(fprintf\([^)]*?"[^"]*?)WiFi([^"]*?"[^)]*?\))', '$1Wi-Fi$2'
}

# Replace in comments (but not in #include or function names)
$content = $content -replace '(//[^\r\n]*?)WiFi', '$1Wi-Fi'
$content = $content -replace '(/\*[^\*]*?)WiFi([^\*]*?\*/)', '$1Wi-Fi$2'

Set-Content $file $content -NoNewline
Write-Host "Updated ucxclient_win64.c"
