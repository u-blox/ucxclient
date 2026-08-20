<#
.SYNOPSIS
    STM32 helper: flash, reset, or inspect a hung target via ST-LINK.
.EXAMPLE
    powershell -File stm32_tool.ps1 -Action flash -File2 e:\path\image.hex
    powershell -File stm32_tool.ps1 -Action pc
#>
param(
    [ValidateSet('flash','reset','pc','regs','mem')]
    [string]$Action = 'pc',
    [string]$HexFile = '',
    [string]$Addr = '0x40011000',
    [int]$Count = 8
)

$ErrorActionPreference = 'Stop'
$cli = 'C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe'

switch ($Action) {
    'flash' {
        & $cli -c port=SWD -w $HexFile -v 2>&1 | Select-Object -Last 3
    }
    'reset' {
        & $cli -c port=SWD -rst 2>&1 | Select-Object -Last 2
    }
    'pc' {
        # Read PC and active exception without resetting the target
        & $cli -c port=SWD mode=HOTPLUG -coreReg PC -r32 0xE000ED04 1 2>&1 |
            Select-String -Pattern 'PC|0xE000ED04'
    }
    'regs' {
        & $cli -c port=SWD mode=HOTPLUG -coreReg R0 -coreReg R1 -coreReg R2 -coreReg R3 -coreReg LR -coreReg PC -coreReg SP 2>&1 |
            Select-String -Pattern '='
    }
    'mem' {
        & $cli -c port=SWD mode=HOTPLUG -r32 $Addr $Count 2>&1 |
            Select-String -Pattern '0x'
    }
}
