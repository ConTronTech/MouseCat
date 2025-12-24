# Mousecat 🐱

An animated desktop pet that follows your mouse cursor around the screen. A modern C++ implementation inspired by classic desktop pets like Oneko.

![Mousecat Demo](https://img.shields.io/badge/platform-Linux-blue) ![Language](https://img.shields.io/badge/language-C%2B%2B11-orange)

## Features

- 🎨 Multiple color palettes (swap with triple left-click)
- 🏃 Smooth animations with multiple states (running, idle, sleeping, scratching)
- 💤 Auto-sleeps when mouse is idle for 30 seconds
- 🎯 Smart chase behavior with deadzone detection
- 🖱️ Interactive controls (5 right-clicks to close, 3 left-clicks to change color)
- 🖥️ Multi-monitor support

## Dependencies

### Ubuntu/Debian:
```bash
sudo apt install build-essential libsdl2-dev libsdl2-image-dev libx11-dev libxext-dev
```

### Fedora:
```bash
sudo dnf install gcc-c++ SDL2-devel SDL2_image-devel libX11-devel libXext-devel
```

### Arch:
```bash
sudo pacman -S base-devel sdl2 sdl2_image libx11 libxext
```

## Building

```bash
make
```

Clean build artifacts:
```bash
make clean
```

## Running

```bash
./mousecat
```

Or build and run:
```bash
make run
```

## Controls

- **Triple left-click** - Cycle through sprite color palettes
- **5 right-clicks (within 2 seconds)** - Close the application
- **ESC key** - Close the application

## Adding Custom Sprites

Drop any `oneko*.png` sprite sheets in `src/sprite/` and they'll be automatically detected! The sprite sheet should be 32x32 pixel frames in an 8-column grid format.
defalt one is provided it is called `oneko-W.png`

```

See `install/README.md` for more options.

## Project Structure

```
mousecat/
├── src/
│   ├── desktop_cat.cpp       # Main application logic
│   ├── main.cpp              # Entry point
│   ├── include/              # Header files
│   └── sprite/               # Sprite palettes (oneko*.png)
├── install/                  # Installation scripts and docs
├── mousecat                  # Compiled binary
└── Makefile
```

## How It Works

- **Animation System**: State machine with multiple cat behaviors (IDLE, RUNNING, SLEEPING, SCRATCHING, etc.)
- **Chase Mode**: Cat follows mouse until reaching 50px radius, then enters idle animations
- **Deadzone**: At 50-100px, cat shows alert animation without moving
- **Sleep Detection**: Monitors mouse movement; sleeps after 30 seconds of inactivity
- **X11 Transparency**: Uses shaped windows for pixel-perfect transparency

## License

This is a fun desktop pet project. Feel free to modify and share!

## Credits

Inspired by the classic Oneko desktop pet.
