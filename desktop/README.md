# Noise Reduction - Desktop

Cross-platform noise reduction for Bluetooth earbuds (Linux, Windows, macOS).

## Build

### Linux (CachyOS, Arch, Ubuntu, etc.)
```bash
# Install PortAudio
sudo pacman -S portaudio  # Arch/CachyOS
# or
sudo apt install portaudio19-dev  # Ubuntu/Debian

# Build
cd desktop
mkdir build && cd build
cmake ..
make
```

### Windows
```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install portaudio

# Build with CMake
cd desktop
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### macOS
```bash
brew install portaudio
cd desktop
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
./noise_reduction [options]

Options:
  --gain <0.0-2.0>     Output gain (default: 0.3, reduces to 30% volume)
  --gate <0.0-0.5>     Noise gate threshold (default: 0.01)
  --hpf <20-500>       High-pass filter cutoff Hz (default: 100)
  --disable            Start with processing disabled
```

## How it works

Captures mic input → applies gain reduction + high-pass filter + noise gate → outputs through speakers/earbuds.

- **Gain**: Reduces ambient sound volume (0.0 = silence, 1.0 = passthrough)
- **Noise gate**: Mutes audio below threshold (removes quiet background hiss)
- **HPF**: Removes low-frequency rumble (fans, AC, traffic)

## Tips

- Start with `--gain 0.3` for 70% noise reduction
- Increase `--gate` if you hear hiss when it should be silent
- Adjust `--hpf` to taste (higher = less bass in ambient sound)
- Works best with wired headphones to avoid Bluetooth latency
