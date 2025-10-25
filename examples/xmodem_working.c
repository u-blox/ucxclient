/**
 * @file xmodem.c
 * @brief XMODEM Sender for NORA-W36 Firmware Updates
 *
 * Simple XMODEM-1K sender tested with real NORA-W36 hardware.
 * Includes robust error handling and buffer management.
 *
 * Compilation:
 *   GCC/MinGW:  gcc -o xmodem xmodem.c -lkernel32
 *   Visual Studio: cl /Fe:xmodem.exe xmodem.c kernel32.lib
 *
 * Directory listing: ls (Linux/macOS) or dir (Windows)
 * Usage: xmodem.exe COM3 NORA-W36X-SW-3.1.0-150.bin [115200]
 */
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
// XMODEM Protocol Constants
#define SOH 0x01         // Start of Header (128-byte blocks)
#define STX 0x02         // Start of Text (1K blocks)
#define EOT 0x04         // End of Transmission
#define ACK 0x06         // Acknowledge
#define NAK 0x15         // Negative Acknowledge
#define CAN 0x18         // Cancel
#define SUB 0x1A         // Padding character
#define C_CHAR 0x43      // Request CRC mode ('C')
#define BLOCK_SIZE 1024
#define MAX_RETRIES 10
#define TIMEOUT_MS 3000
static uint16_t calculate_crc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}
static bool serial_write(HANDLE hSerial, const uint8_t* data, size_t length) {
    DWORD bytesWritten;
    if (!WriteFile(hSerial, data, (DWORD)length, &bytesWritten, NULL)) {
        return false;
    }
    FlushFileBuffers(hSerial);
    return bytesWritten == length;
}
static bool serial_read_byte(HANDLE hSerial, uint8_t* byte, DWORD timeout_ms) {
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = timeout_ms;
    timeouts.ReadTotalTimeoutConstant = timeout_ms;
    SetCommTimeouts(hSerial, &timeouts);
    DWORD bytesRead;
    return ReadFile(hSerial, byte, 1, &bytesRead, NULL) && bytesRead == 1;
}
static void serial_flush_input(HANDLE hSerial) {
    PurgeComm(hSerial, PURGE_RXCLEAR);
}
static char* format_port_name(const char* port_name) {
    static char formatted_name[256];
    // Check if already formatted or if it's a low COM port
    if (strncmp(port_name, "\\\\.\\", 4) == 0) {
        strcpy_s(formatted_name, sizeof(formatted_name), port_name);
        return formatted_name;
    }
    // For COM ports 1-9, use simple format
    if (strlen(port_name) == 4 && strncmp(port_name, "COM", 3) == 0 &&
        port_name[3] >= '1' && port_name[3] <= '9') {
        strcpy_s(formatted_name, sizeof(formatted_name), port_name);
        return formatted_name;
    }
    // For COM10 and higher, use Windows extended format
    if (strncmp(port_name, "COM", 3) == 0) {
        snprintf(formatted_name, sizeof(formatted_name), "\\\\.\\%s", port_name);
        return formatted_name;
    }
    // For other formats, assume it's correct
    strcpy_s(formatted_name, sizeof(formatted_name), port_name);
    return formatted_name;
}
static bool wait_for_start_signal(HANDLE hSerial) {
    printf("Waiting for receiver ready signal...\n");
    serial_flush_input(hSerial);
    uint8_t byte;
    DWORD start_time = GetTickCount();
    while ((GetTickCount() - start_time) < 60000) {    // 60 second timeout
        if (serial_read_byte(hSerial, &byte, 1000)) {
            if (byte == C_CHAR) {
                printf("Receiver ready (CRC mode)\n");
                return true;
            } else if (byte == NAK) {
                printf("Receiver ready (checksum mode)\n");
                return true;
            } else if (byte == CAN) {
                printf("Transfer cancelled by receiver\n");
                return false;
            } else {
                printf("Unexpected response: 0x%02X\n", byte);
                Sleep(100);
                serial_flush_input(hSerial);
            }
        }
    }
    printf("Timeout waiting for start signal\n");
    return false;
}
static bool send_block(HANDLE hSerial, uint8_t block_num, const uint8_t* data) {
    uint8_t block[3 + BLOCK_SIZE + 2];       // Header + data + CRC
    uint8_t block_complement = ~block_num;    // Bitwise complement for XMODEM protocol
    // Build block: STX + block_num + complement + data + CRC16
    block[0] = STX;
    block[1] = block_num;
    block[2] = block_complement;
    memcpy(&block[3], data, BLOCK_SIZE);
    uint16_t crc = calculate_crc16(data, BLOCK_SIZE);
    block[3 + BLOCK_SIZE] = (crc >> 8) & 0xFF;
    block[4 + BLOCK_SIZE] = crc & 0xFF;
    // Send block with retries
    for (int retry = 0; retry < MAX_RETRIES; retry++) {
        printf("[XMODEM] Sending block %u, try %d\n", block_num, retry + 1);
        if (!serial_write(hSerial, block, sizeof(block))) {
            printf("[XMODEM] Failed to write block %u\n", block_num);
            continue;
        }
        // Special timing for bootloader
        if (block_num == 2) {
            Sleep(500);
        }
        // Wait for response
        uint8_t response;
        if (serial_read_byte(hSerial, &response, TIMEOUT_MS)) {
            printf("[XMODEM] Received response: 0x%02X\n", response);
            if (response == ACK) {
                printf("[XMODEM] Block %u acknowledged\n", block_num);
                // Clear any additional bytes that might be in the buffer
                Sleep(50);
                serial_flush_input(hSerial);
                return true;
            } else if (response == NAK) {
                printf("[XMODEM] NAK received for block %u, retrying...\n", block_num);
                Sleep(100);
                serial_flush_input(hSerial);
                continue;
            } else if (response == CAN) {
                printf("[XMODEM] Cancelled by receiver\n");
                return false;
            } else {
                printf("[XMODEM] Unexpected response 0x%02X", response);
                // Read and display any additional bytes in buffer for debugging
                uint8_t extra_byte;
                int extra_count = 0;
                printf(" - Additional bytes: ");
                while (serial_read_byte(hSerial, &extra_byte, 100) && extra_count < 10) {
                    printf("0x%02X ", extra_byte);
                    extra_count++;
                }
                printf("\n");
                Sleep(200);
                serial_flush_input(hSerial);
                continue;
            }
        } else {
            printf("[XMODEM] Timeout waiting for response\n");
            Sleep(100);
            serial_flush_input(hSerial);
        }
    }
    printf("[XMODEM] Failed after %d retries for block %u\n", MAX_RETRIES, block_num);
    return false;
}
static bool send_eot(HANDLE hSerial) {
    printf("[XMODEM] Sending EOT\n");
    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        uint8_t eot = EOT;
        if (!serial_write(hSerial, &eot, 1)) {
            printf("[XMODEM] Failed to send EOT\n");
            continue;
        }
        uint8_t response;
        if (serial_read_byte(hSerial, &response, TIMEOUT_MS)) {
            if (response == ACK) {
                printf("[XMODEM] Transfer completed successfully\n");
                return true;
            } else {
                printf("[XMODEM] Unexpected EOT response: 0x%02X\n", response);
            }
        } else {
            printf("[XMODEM] Timeout waiting for EOT response\n");
        }
        Sleep(1000);
    }
    printf("[XMODEM] Failed to get EOT acknowledgment\n");
    return false;
}
static bool xmodem_send_file(HANDLE hSerial, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Could not open file '%s'\n", filename);
        return false;
    }
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    size_t total_blocks = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    printf("Sending file: %s\n", filename);
    printf("File size: %ld bytes\n", file_size);
    printf("Total blocks: %zu\n", total_blocks);
    printf("Protocol: XMODEM-1K with CRC-16\n");
    // Wait for receiver ready
    if (!wait_for_start_signal(hSerial)) {
        fclose(file);
        return false;
    }
    // Send all blocks
    uint8_t block_num = 1;
    uint8_t data_buffer[BLOCK_SIZE];
    for (size_t block_index = 0; block_index < total_blocks; block_index++) {
        // Read and pad block
        memset(data_buffer, SUB, BLOCK_SIZE);
        size_t bytes_read = fread(data_buffer, 1, BLOCK_SIZE, file);
        if (bytes_read == 0 && block_index < total_blocks - 1) {
            printf("Error: Unexpected end of file\n");
            fclose(file);
            return false;
        }
        // Send block
        if (!send_block(hSerial, block_num, data_buffer)) {
            printf("Error: Failed to send block %u\n", block_num);
            fclose(file);
            return false;
        }
        // Progress indicator
        printf("Progress: %zu%% (%zu/%zu blocks)\n",
               (block_index + 1) * 100 / total_blocks,
               block_index + 1, total_blocks);
        // Increment block number with natural 8-bit wraparound
        block_num = (block_num + 1) % 256;
    }
    fclose(file);
    return send_eot(hSerial);
}
static HANDLE init_serial_port(const char* port_name, DWORD baud_rate) {
    const char* formatted_port = format_port_name(port_name);
    printf("Opening serial port: %s\n", formatted_port);
    HANDLE hSerial = CreateFile(formatted_port, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error: Could not open serial port %s (tried %s)\n", port_name, formatted_port);
        DWORD error = GetLastError();
        printf("Windows error code: %lu\n", error);
        return INVALID_HANDLE_VALUE;
    }
    // Configure serial port
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);
    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        printf("Error: Could not configure serial port\n");
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }
    return hSerial;
}
static bool nora_firmware_update(const char* port_name, const char* firmware_file, DWORD baud_rate) {
    printf("NORA-W36 Firmware Update Tool\n");
    printf("========================================\n");
    // Step 1: Send AT command to enter XMODEM mode
    printf("Connecting to NORA-W36...\n");
    HANDLE hSerial = init_serial_port(port_name, 115200);
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }
    printf("Entering XMODEM mode at %lu baud...\n", baud_rate);
    char at_command[32];
    snprintf(at_command, sizeof(at_command), "AT+USYFWUS=%lu\r", baud_rate);
    if (!serial_write(hSerial, (uint8_t*)at_command, strlen(at_command))) {
        printf("Error: Failed to send AT command\n");
        CloseHandle(hSerial);
        return false;
    }
    uint8_t response[100];
    DWORD bytes_read = 0;
    Sleep(500);
    ReadFile(hSerial, response, sizeof(response) - 1, &bytes_read, NULL);
    response[bytes_read] = '\0';
    if (strstr((char*)response, "OK") == NULL) {
        printf("Warning: Unexpected response: %s\n", response);
    }
    Sleep(2000);
    CloseHandle(hSerial);
    Sleep(500);
    // Step 2: Transfer firmware using XMODEM-1K
    printf("Starting XMODEM-1K transfer at %lu baud...\n", baud_rate);
    hSerial = init_serial_port(port_name, baud_rate);
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }
    bool success = xmodem_send_file(hSerial, firmware_file);
    CloseHandle(hSerial);
    if (success) {
        printf("\nFirmware update completed successfully!\n");
        // Step 3: Check firmware version
        printf("Checking firmware version...\n");
        Sleep(5000);
        for (int attempt = 0; attempt < 6; attempt++) {
            hSerial = init_serial_port(port_name, 115200);
            if (hSerial != INVALID_HANDLE_VALUE) {
                if (serial_write(hSerial, (uint8_t*)"AT+GMR\r", 7)) {
                    Sleep(1000);
                    if (ReadFile(hSerial, response, sizeof(response) - 1, &bytes_read, NULL) && bytes_read > 0) {
                        response[bytes_read] = '\0';
                        if (strstr((char*)response, "OK")) {
                            char* context = NULL;
                            char* lines = strtok_s((char*)response, "\n\r", &context);
                            while (lines != NULL) {
                                if (strlen(lines) > 0 && strcmp(lines, "OK") != 0 && strncmp(lines, "AT", 2) != 0) {
                                    printf("Firmware version: %s\n", lines);
                                    CloseHandle(hSerial);
                                    return true;
                                }
                                lines = strtok_s(NULL, "\n\r", &context);
                            }
                        }
                    }
                }
                CloseHandle(hSerial);
            }
            printf("Module restarting (attempt %d/6)...\n", attempt + 1);
            if (attempt < 5) Sleep(5000);
        }
    } else {
        printf("\nFirmware update failed!\n");
    }
    return success;
}
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("XMODEM Sender for NORA-W36 Firmware Updates\n");
        printf("Usage: %s <port> <firmware_file> [baud_rate]\n", argv[0]);
        printf("\n");
        printf("Examples:\n");
        printf("  %s COM3 NORA-W36X-SW-3.1.0-150.bin\n", argv[0]);
        printf("  %s COM3 NORA-W36X-SW-3.1.0-150.bin 115200\n", argv[0]);
        return 1;
    }
    const char* port_name = argv[1];
    const char* firmware_file = argv[2];
    DWORD baud_rate = (argc > 3) ? atol(argv[3]) : 115200;
    bool success = nora_firmware_update(port_name, firmware_file, baud_rate);
    return success ? 0 : 1;
}