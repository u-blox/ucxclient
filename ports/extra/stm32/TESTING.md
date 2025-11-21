# STM32F407VG Testing with Renode

This directory contains scripts and configuration for testing the STM32F407VG firmware using Renode emulation.

## Quick Start

### Build and Test Locally

```bash
cd docker
./test_renode.sh
```

This will:
1. Build the STM32F407VG firmware
2. Run it in Renode emulator for 10 seconds
3. Capture and analyze the output

### Build Only

```bash
cd docker
./build_stm32.sh
```

### Manual Renode Testing

Build the Docker image with Renode:
```bash
docker compose build stm32f4-renode
```

Run Renode interactively:
```bash
docker compose run --rm stm32f4-renode bash
cd build_stm32
renode stm32f407vg.resc
```

## Renode Configuration

The `stm32f407vg.resc` script configures:
- STM32F4-Discovery board emulation
- UART2 for serial output
- Automatic firmware loading
- Terminal analyzer for viewing output

## GitHub Actions

The workflow `.github/workflows/stm32-build-test.yml` automatically:
- Builds the firmware on every push
- Runs Renode emulation tests
- Uploads build artifacts
- Reports test results

## Expected Test Output

The firmware should:
1. Initialize FreeRTOS
2. Start the main task
3. Initialize UART
4. Print debug messages
5. Run for the configured duration

## Customizing Tests

Edit `test_renode.sh` to:
- Change emulation duration
- Add output validation
- Configure WiFi credentials
- Add automated test cases

## Troubleshooting

### Renode doesn't start
- Ensure mono is installed in the Docker container
- Check Renode version compatibility

### No UART output
- Verify UART2 configuration in the firmware
- Check platform description file
- Enable debug logging in Renode

### Build fails
- Ensure official ARM toolchain is being used
- Check STM32CubeF4 submodules are initialized
- Verify all SDK files are copied correctly

## Dependencies

- Docker and Docker Compose
- ARM GNU Toolchain 13.2.Rel1
- STM32CubeF4 v1.28.3
- Renode 1.15.3
- Mono runtime (for Renode)

## Resources

- [Renode Documentation](https://renode.readthedocs.io/)
- [STM32F4-Discovery Board](https://www.st.com/en/evaluation-tools/stm32f4discovery.html)
- [FreeRTOS](https://www.freertos.org/)

