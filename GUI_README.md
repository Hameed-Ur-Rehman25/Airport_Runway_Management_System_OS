# GUI Mode Documentation

## Overview

The Airport Runway Management System now includes a **Terminal-based GUI** using ncurses for real-time visualization of runway operations, queues, and statistics.

## Features

### Visual Components

1. **Header Bar** - System title and mode indicator
2. **Runway Status Window** - Shows active plane operations with progress bar
3. **Emergency Queue Window** - Real-time emergency plane queue (red highlight)
4. **Normal Queue Window** - Real-time normal plane queue
5. **Statistics Window** - Live statistics (planes processed, preemptions, etc.)
6. **Event Log Window** - Scrolling log of all system events with timestamps

### Real-Time Updates

- **Runway visualization** updates every 500ms showing active plane and progress
- **Queue displays** update when planes arrive, depart, or are preempted
- **Statistics** update as planes complete operations
- **Event log** shows all activities with color-coded messages

### Color Coding

- **🟢 Green** - Normal operations and planes
- **🔴 Red** - Emergency operations and alerts
- **🟡 Yellow** - Active runway operations and warnings
- **🔵 Cyan** - System information
- **⚪ White/Blue** - Headers and UI elements

## Running GUI Mode

### Basic Usage

```bash
# Enable GUI mode with -g flag
./runway_simulator -g

# GUI mode with custom parameters
./runway_simulator -g -n 15 -e 25 -l 5 -t 3
```

### Using Makefile

```bash
# Run with GUI mode (10 planes, 20% emergency)
make run-gui
```

### Command-Line Options

```bash
./runway_simulator -g [options]

Options:
  -g             Enable GUI mode (required for visual interface)
  -n <number>    Total number of planes
  -e <percent>   Emergency probability (0-100%)
  -l <seconds>   Landing duration
  -t <seconds>   Takeoff duration
  -h             Display help
```

## GUI Layout

```
╔══════════════════════════════════════════════════════════╗
║   AIRPORT RUNWAY MANAGEMENT SYSTEM - Real-Time Visual   ║
╚══════════════════════════════════════════════════════════╝

┌─────────────────────────┐ ┌─────────────────────────┐
│   RUNWAY STATUS         │ │  EMERGENCY QUEUE        │
│                         │ │  Waiting: 2 plane(s)    │
│  ═════════════════════  │ │                         │
│  ||  ✈ Plane 5 LANDING │ │  ⚠ Plane 7 - LANDING    │
│  ||    (Progress: 65%) │ │  ⚠ Plane 9 - TAKEOFF    │
│  ═════════════════════  │ │                         │
│                         │ └─────────────────────────┘
│  Progress: [=====   ] 65% │
└─────────────────────────┘

┌─────────────────────────┐ ┌─────────────────────────┐
│   NORMAL QUEUE          │ │     STATISTICS          │
│   Waiting: 3 plane(s)   │ │                         │
│                         │ │  Total Planes: 15       │
│   ✈ Plane 2 - LANDING   │ │  Completed: 8           │
│   ✈ Plane 4 - TAKEOFF   │ │  Preemptions: 3         │
│   ✈ Plane 6 - LANDING   │ │  In Progress: 7         │
└─────────────────────────┘ └─────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│  EVENT LOG                                              │
├─────────────────────────────────────────────────────────┤
│ [23:45:12] [ARRIVAL] Plane 5 (NORMAL, LANDING)          │
│ [23:45:13] [EMERGENCY] Plane 7 added to EMERGENCY queue │
│ [23:45:13] [PREEMPTED] Plane 5 interrupted at 65%       │
│ [23:45:13] [GRANTED] EMERGENCY Plane 7 granted access   │
│ [23:45:15] [COMPLETED] Plane 7 finished LANDING         │
│ [23:45:15] [RESUME] Plane 5 resuming from 65%           │
└─────────────────────────────────────────────────────────┘
```

## Terminal Requirements

### Minimum Size

- **Width**: 80 columns
- **Height**: 24 rows
- Recommended: 100x30 or larger for best viewing

### Terminal Support

- **macOS**: Terminal.app, iTerm2 ✅
- **Linux**: GNOME Terminal, Konsole, xterm ✅
- **Windows**: WSL with Windows Terminal ✅

### Dependencies

- **ncurses library** (installed during build)

  ```bash
  # macOS (usually pre-installed)
  brew install ncurses

  # Ubuntu/Debian
  sudo apt-get install libncurses5-dev

  # Fedora/RHEL
  sudo dnf install ncurses-devel
  ```

## Controls

- **During Simulation**: Watch real-time updates automatically
- **On Completion**: Press **any key** to exit GUI and view final statistics
- **Emergency Exit**: `Ctrl+C` (may leave terminal in inconsistent state)

## Exiting GUI Mode

The simulator will:

1. Display "Simulation Complete" message in event log
2. Wait for keypress
3. Clean up ncurses interface
4. Show final statistics in regular console
5. Exit gracefully

## Troubleshooting

### Terminal Display Issues

**Problem**: Garbled or misaligned display

```bash
# Reset terminal
reset
# Or
tput reset
```

**Problem**: Colors not showing

```bash
# Check terminal supports colors
echo $TERM
# Should be: xterm-256color or similar
```

### GUI Not Starting

**Problem**: `ncurses` library not found

```bash
# Rebuild with explicit library path
make clean
make LDFLAGS="-pthread -lncurses"
```

**Problem**: Terminal too small

- Resize terminal to at least 80x24
- Or use console mode: `./runway_simulator` (without `-g`)

### Performance Issues

**Problem**: GUI updates too slow

- Reduce number of planes: `-n 10`
- Increase operation durations: `-l 5 -t 4`
- Use console mode for faster simulation

## Comparison: GUI vs Console Mode

| Feature       | GUI Mode (`-g`)     | Console Mode (default) |
| ------------- | ------------------- | ---------------------- |
| Visualization | ✅ Real-time visual | ❌ Text logs only      |
| Progress bars | ✅ Yes              | ❌ No                  |
| Color coding  | ✅ Yes              | ❌ No                  |
| Queue display | ✅ Visual           | ❌ Text counts only    |
| Performance   | Slightly slower     | Faster                 |
| Debugging     | Harder              | Easier (scrollback)    |
| Best for      | Demo/Presentation   | Testing/Analysis       |

## Example Sessions

### Demo Mode (15 planes, 25% emergency)

```bash
make run-gui
# Or
./runway_simulator -g -n 15 -e 25 -l 5 -t 4
```

### Quick Test (5 planes, high emergency rate)

```bash
./runway_simulator -g -n 5 -e 40 -l 3 -t 2
```

### Long Run (30 planes, realistic settings)

```bash
./runway_simulator -g -n 30 -e 15 -l 6 -t 4
```

## Implementation Details

### Thread-Safe GUI Updates

- All GUI operations protected by `gui_sem` semaphore
- Console output redirected to GUI log window
- No race conditions between threads

### Update Triggers

- **Runway updates**: When plane starts, progress updates (500ms), completes
- **Queue updates**: When planes enqueue, dequeue, or re-queue after preemption
- **Stats updates**: When planes complete or are preempted
- **Log updates**: All runway_print_status() calls redirect to GUI log

### Memory Management

- Windows properly created and destroyed
- Semaphores initialized and cleaned up
- No memory leaks in GUI mode

## Tips for Best Results

1. **Maximize terminal window** for clearest view
2. **Use dark terminal theme** for better color contrast
3. **Run with moderate plane counts** (10-20) for best visualization
4. **Increase durations** (`-l 5 -t 4`) to see operations more clearly
5. **Use higher emergency rate** (`-e 30`) to see preemption in action

## Future GUI Enhancements

Potential additions:

- ✈️ Animated plane movement on runway
- 📊 Real-time graphs of wait times
- 🎨 Customizable color schemes
- ⏸️ Pause/resume controls
- 🔍 Zoom/focus on specific planes
- 📷 Screenshot/record functionality

---

**Note**: GUI mode requires a terminal with ncurses support. If experiencing issues, use console mode (without `-g` flag) for full functionality with text-based output.
