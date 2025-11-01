# Script to replace WiFi/wifi with Wi-Fi in user-visible text only
# Excludes API function names, variable names, and file names

$files = @(
    "c:\u-blox\ucxclient\examples\ucxclient_win64.c",
    "c:\u-blox\ucxclient\examples\windows_basic.c",
    "c:\u-blox\ucxclient\examples\README.md",
    "c:\u-blox\ucxclient\examples\README_windows_app.md",
    "c:\u-blox\ucxclient\IMPROVEMENTS.md",
    "c:\u-blox\ucxclient\examples\http_example.c",
    "c:\u-blox\ucxclient\examples\http_example_no_os.c",
    "c:\u-blox\ucxclient\build_scripts\build_windows.bat"
)

foreach ($file in $files) {
    if (Test-Path $file) {
        $content = Get-Content $file -Raw
        
        # Replace in comments (// ... WiFi ...) and (/* ... WiFi ... */)
        $content = $content -replace '(//[^\r\n]*?)WiFi', '$1Wi-Fi'
        $content = $content -replace '(//[^\r\n]*?)wifi', '$1Wi-Fi'
        $content = $content -replace '(/\*.*?\*/)' -replace 'WiFi', 'Wi-Fi'
        
        # Replace in printf/echo statements - but not uCxWifi or u_cx_wifi
        $content = $content -replace '(printf\([^)]*?)WiFi', '$1Wi-Fi'
        $content = $content -replace '(printf\([^)]*?)wifi', '$1Wi-Fi'
        $content = $content -replace '(echo[^\r\n]*?)WiFi', '$1Wi-Fi'
        $content = $content -replace '(echo[^\r\n]*?)wifi', '$1Wi-Fi'
        
        # Replace in menu text and user messages
        $content = $content -replace '(["''].*?)WiFi(.*?["''])', '$1Wi-Fi$2'
        $content = $content -replace '(["''].*?)wifi(.*?["''])', '$1Wi-Fi$2'
        
        # Markdown headers and text
        if ($file -match '\.md$') {
            # In markdown, replace WiFi/wifi that's not part of code blocks
            $content = $content -replace '(?<!`)(?<!`)WiFi(?!`)(?!`)', 'Wi-Fi'
            $content = $content -replace '(?<!`)(?<!`)wifi(?!`)(?!`)', 'Wi-Fi'
        }
        
        Set-Content $file $content -NoNewline
        Write-Host "Updated: $file"
    }
}

Write-Host "Done! Please review the changes."
