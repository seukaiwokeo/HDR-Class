# hdr_class
Knight Online C++ HDR Packing &amp; Unpacking

### Build Status
[![Build](https://github.com/seukaiwokeo/hdr_class/actions/workflows/build.yml/badge.svg)](https://github.com/seukaiwokeo/hdr_class/actions/workflows/build.yml)

### Releases
[![Releases](https://img.shields.io/github/v/release/seukaiwokeo/hdr_class)](https://github.com/seukaiwokeo/hdr_class/releases)


## Usage
	HDRPacker* packer = new HDRPacker();  
	packer->Pack(); // pack uif and dxt files into ui.hdr and ui.src  
	packer->Unpack(); // unpack uif and dxt files from ui.src

## Building with CMake

### Prerequisites
- CMake 3.20 or higher
- Visual Studio 2022 with C++ development tools
- Windows 32-bit (x86) build tools

### Build Steps

1. Create a build directory and navigate to it:
```powershell
mkdir build
cd build
```

2. Generate CMake configuration:
```powershell
cmake ..
```

3. Build the project:
```powershell
cmake --build . --config Release
```

The executable will be created in `build/bin/release` directory.

For debug build:
```powershell
cmake --build . --config Debug
```

The debug executable will be created in `build/bin/debug` directory.

### Opening in Visual Studio
To open the project in Visual Studio 2022:
```powershell
cmake --open .
```

### Notes
- Project is configured for Windows 32-bit (x86) architecture
- Uses C++20 standard
- Special configurations are set for MSVC compiler
