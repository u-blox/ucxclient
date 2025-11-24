# Release Process for ucxclient-x64

## Preparing a Release

### 1. Build Release Version
```bash
cd examples
invoke ucxclient-x64
# Or with CMake directly:
cmake --build build --config Release --target ucxclient-x64
```

### 2. Sign the Executable
Use the launcher script to build and sign in one step:
```powershell
# Build Release and sign with certificate thumbprint
.\launch_ucxclient-x64.cmd sign EF3FD135F1CD669E0D7F4F2CF14FE1334EECD16E

# This will:
# - Build Release configuration
# - Sign with u-blox AG code signing certificate
# - Verify signature with full certificate chain
# - Create ucxclient-x64-signed.exe in examples/bin/
```

The signing process:
- Uses GlobalSign EV CodeSigning certificate (valid until Dec 2027)
- Timestamped with DigiCert timestamp server
- Full certificate chain verification
- Creates both signed and unsigned versions

### 3. Copy to Release Folder
```powershell
# Create release folder if needed
mkdir releases/v1.0.0-beta.1

# Copy SIGNED executable (ftd2xx64.dll is embedded as resource)
copy examples/bin/ucxclient-x64-signed.exe releases/v1.0.0-beta.1/
```

**Important:** The released executable is named `ucxclient-x64-signed.exe` to distinguish it from unsigned builds.

### 4. Update Documentation
Edit the README.txt and INSTALL.txt files in the release folder with:
- Release date
- Version number
- Known issues
- Changelog

### 5. Commit to Git
```bash
git add releases/v1.0.0-beta.1/
git commit -m "Release v1.0.0-beta.1"
git tag -a v1.0.0-beta.1 -m "Beta release 1 for version 1.0.0"
git push origin cmag_win64_port
git push origin v1.0.0-beta.1
```

### 6. Create GitHub Release
1. Go to: https://github.com/u-blox/ucxclient/releases
2. Click "Draft a new release"
3. Tag: v1.0.0-beta.1
4. Title: ucxclient-x64 v1.0.0-beta.1
5. Description: Copy from README.txt
6. Check "This is a pre-release" for beta/RC versions
7. Attach files:
   - ucxclient-x64-signed.exe (contains embedded ftd2xx64.dll)
   - README.txt
   - INSTALL.txt
8. Click "Publish release"

## Version Numbering

Follow semantic versioning:
- **Beta**: v1.0.0-beta.1, v1.0.0-beta.2, etc.
- **RC**: v1.0.0-rc.1, v1.0.0-rc.2, etc.
- **Release**: v1.0.0
- **Patches**: v1.0.1, v1.0.2, etc.
- **Minor**: v1.1.0, v1.2.0, etc.
- **Major**: v2.0.0, v3.0.0, etc.

## Release Checklist

Before creating a release:
- [ ] All compiler warnings fixed
- [ ] Code reviewed and cleaned up
- [ ] Tested on clean Windows installation
- [ ] All features working as expected
- [ ] Documentation updated
- [ ] Version number updated in code (if applicable)
- [ ] Changelog updated
- [ ] Release notes written
- [ ] Executable signed with code signing certificate
- [ ] All dependencies included (DLLs)
- [ ] Installation tested
- [ ] Git tagged with version number

## Directory Structure

```
releases/
├── v1.0.0-beta.1/
│   ├── ucxclient-x64-signed.exe (with embedded ftd2xx64.dll)
│   ├── README.txt
│   └── INSTALL.txt
├── v1.0.0-beta.2/
├── v1.0.0-rc.1/
└── v1.0.0/
```

Each release folder is tracked in git (allowed in .gitignore).
GitHub Releases contain the same files as distribution packages.
