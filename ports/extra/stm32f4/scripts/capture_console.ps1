<#
.SYNOPSIS
    Capture serial console output from an STM32 board, optionally resetting it first.
.EXAMPLE
    powershell -NoProfile -File capture_console.ps1 -Port COM138 -Seconds 15 -Reset
#>
param(
    [string]$Port = 'COM138',
    [int]$Baud = 115200,
    [int]$Seconds = 15,
    [switch]$Reset
)

$ErrorActionPreference = 'Stop'
$cli = 'C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe'

$p = New-Object System.IO.Ports.SerialPort $Port, $Baud, 'None', 8, 'One'
try {
    $p.DtrEnable = $true
    $p.Open()
    Write-Host "[capture] $Port open at $Baud"
    $p.DiscardInBuffer()

    if ($Reset) {
        Write-Host '[capture] Resetting target via ST-LINK...'
        & $cli -c port=SWD -rst *> $null
        Write-Host "[capture] Reset done (exit $LASTEXITCODE)"
    }

    $end = (Get-Date).AddSeconds($Seconds)
    $sb = New-Object System.Text.StringBuilder
    while ((Get-Date) -lt $end) {
        $chunk = $p.ReadExisting()
        if ($chunk) { [void]$sb.Append($chunk); Write-Host -NoNewline $chunk }
        Start-Sleep -Milliseconds 50
    }
    Write-Host ''
    Write-Host "[capture] Total bytes: $($sb.Length)"
}
catch {
    Write-Host "[capture] EXCEPTION: $($_.Exception.Message)"
    exit 1
}
finally {
    if ($p.IsOpen) { $p.Close() }
}
