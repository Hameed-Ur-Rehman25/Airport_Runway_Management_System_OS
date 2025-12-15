# Airport Runway Management System Simulation

## Project Overview

This project simulates an airport runway management system using Operating System concepts such as **semaphores**, **synchronization**, and **priority scheduling**. The system models planes as processes (threads) competing for access to a single runway resource, with emergency aircraft receiving higher priority and the ability to preempt normal operations.

**✨ NEW: GUI Mode Available!** Run with `-g` flag for real-time visual interface using ncurses. See [GUI_README.md](GUI_README.md) for details.

## Operating System Concepts Demonstrated

### 1. **Semaphores**

- **Binary Semaphores**: Used as mutex replacements to protect critical sections (runway access, queue operations, console output)
- **Counting Semaphores**: Track the number of planes in emergency and normal queues
- All synchronization is achieved using **only semaphores** (no pthread mutexes)

### 2. **Priority Scheduling**

- **Emergency planes** always have priority over normal planes
- Scheduler checks emergency queue first before processing normal queue
- Real-world aviation protocol simulation

### 3. **Preemption with Checkpoint/Resume**

- Normal operations can be interrupted by emergency planes
- **Checkpoint mechanism**: Interrupted planes save their progress (0-100%)
- Operations resume from last checkpoint when runway becomes available
- Preemption checks occur every **500ms** for realistic interrupt response

### 4. **Concurrency Control**

- Multiple threads (planes) competing for shared resources (runway)
- Thread-safe queue operations using semaphore protection
- Race condition prevention through proper synchronization

## Project Structure

```
OS_Project/
├── main.c          # Main program entry, initialization, thread management
├── runway.h        # Runway system definitions and function prototypes
├── runway.c        # Runway management, scheduling, operation logic
├── plane.h         # Plane structure and thread function prototypes
├── plane.c         # Plane thread implementation
├── queue.h         # Dynamic linked-list queue interface
├── queue.c         # Queue operations with semaphore protection
├── gui.h           # GUI interface definitions (ncurses)
├── gui.c           # Real-time visual interface implementation
├── Makefile        # Build configuration
├── README.md       # This file
├── GUI_README.md   # GUI mode documentation
└── ARCHITECTURE.md # System architecture and diagrams
```

## Features

### Core Functionality

- ✈️ **Dual Queue System**: Separate queues for emergency and normal operations
- 🖥️ **GUI Mode**: Terminal-based visual interface with ncurses (optional)
- 🔄 **Dynamic Linked Lists**: Flexible queue implementation
- ⏱️ **Checkpoint/Resume**: Interrupted operations continue from saved state
- 📊 **Real-time Status Display**: Timestamped console output of all events
- 📈 **Statistics Tracking**: Total planes served, preemptions count, queue status

### Configurable Parameters

- Number of planes to simulate
- Emergency plane probability (0-100%)
- Landing operation duration
- Takeoff operation duration

## Building the Project

### Prerequisites

- GCC compiler with C11 support
- POSIX threads library (pthread)
- Unix-like operating system (Linux, macOS)

### Compilation

```bash
# Clean and build
make clean && make

# Just build
make

# Display help
make help
```

## Running the Simulation

### Basic Execution

**Console Mode (Text-based):**

```bash
./runway_simulator
```

**GUI Mode (Visual Interface):**

```bash
./runway_simulator -g
./runway_simulator
```

Runs with default parameters (10 planes, 15% emergency, 5s landing, 4s takeoff)

### Custom Parameters

```bash
./runway_simulator -n 20 -e 25 -l 6 -t 4
```

### Command-Line Options

| Option         | Description                    | Default   |
| -------------- | ------------------------------ | --------- |
| `-n <number>`  | Total number of planes         | 10        |
| `-e <percent>` | Emergency probability (0-100%) | 15%       |
| `-l <seconds>` | Landing duration               | 5 seconds |
| `-t <seconds>` | Takeoff duration               | 4 seconds |
| `-h`           | Display help message           | -         |

### Quick Run Commands

```bash
# Run with default parameters (console mode)
make run

# Run demo with 15 planes and 25% emergency rate (console mode)
make run-demo

# Run with GUI mode (visual interface)
make run-gui
```

## Example Output

```
╔══════════════════════════════════════════════════════════╗
║   AIRPORT RUNWAY MANAGEMENT SYSTEM SIMULATION           ║
╚══════════════════════════════════════════════════════════╝

Simulation Parameters:
  • Total Planes: 8
  • Emergency Probability: 25%
  • Landing Duration: 3 seconds
  • Takeoff Duration: 2 seconds
  • Checkpoint Interval: 500ms

[23:03:55] [ARRIVAL] Plane 1 (NORMAL, TAKEOFF) requesting runway access
[23:03:55] [QUEUE] Plane 1 added to NORMAL queue (Queue size: 1)
[23:03:55] [GRANTED] NORMAL Plane 1 granted runway access
[23:03:55] [OPERATION] Plane 1 starting TAKEOFF (duration: 2s)
[23:03:57] [EMERGENCY] Plane 4 added to EMERGENCY queue (Queue size: 1)
[23:03:57] [PREEMPTED] Plane 3 interrupted at 16% - yielding to emergency
[23:03:57] [GRANTED] EMERGENCY Plane 4 granted runway access
[23:03:57] [RESUME] Plane 3 resuming LANDING from 16% (remaining: 3s)
...

========== SIMULATION STATISTICS ==========
Total Planes Processed: 8
Emergency Preemptions: 3
Emergency Queue Final: 0
Normal Queue Final: 0
===========================================
```

## How It Works

### 1. Initialization

- Runway system initializes all semaphores
- Separate queues created for emergency and normal operations
- Configuration parameters set

### 2. Plane Arrival

- Each plane is a separate thread
- Planes arrive with staggered timing (100ms - 2.1s intervals)
- Randomly assigned operation type (LANDING/TAKEOFF) and priority (EMERGENCY/NORMAL)

### 3. Queue Management

- Emergency planes enqueue to emergency queue and set emergency flag
- Normal planes enqueue to normal queue
- All queue operations protected by binary semaphores

### 4. Priority Scheduling

- Scheduler always services emergency queue first
- Uses `sem_trywait()` pattern to check emergency queue
- Normal planes wait until emergency queue is empty

### 5. Runway Operation

- Plane acquires runway access semaphore
- Operation proceeds in 500ms intervals
- Each interval checks for emergency preemption (normal planes only)
- Progress tracked as checkpoint percentage (0-100%)

### 6. Preemption Handling

- Emergency arrival sets global emergency flag
- Active normal plane detects flag at next checkpoint (500ms)
- Normal plane saves progress and yields runway
- Normal plane re-queues with checkpoint state preserved
- Operation resumes from checkpoint when runway available

### 7. Completion

- Plane completes operation (100% progress)
- Releases runway semaphore
- Statistics updated
- Thread terminates

## Key Implementation Details

### Semaphore Usage

```c
sem_t runway_access;        // Binary: runway availability
sem_t console_access;       // Binary: thread-safe output
sem_t emergency_queue_sem;  // Counting: emergency queue size
sem_t normal_queue_sem;     // Counting: normal queue size
sem_t emergency_flag_sem;   // Binary: protect emergency flag
```

### Checkpoint/Resume Logic

```c
// Save checkpoint on preemption
plane->checkpoint_progress = elapsed_percentage;

// Resume from checkpoint
int remaining_time = duration - (duration * checkpoint / 100);
```

### Priority Scheduling

```c
// Emergency planes checked first
if (emergency_queue_not_empty) {
    service_emergency_plane();
} else {
    service_normal_plane();
}
```

## Learning Objectives Achieved

✅ **Semaphore-based synchronization** in multi-threaded environment  
✅ **Priority scheduling** with real-time constraints  
✅ **Preemption handling** with state preservation  
✅ **Concurrent queue management** with dynamic linked lists  
✅ **Race condition prevention** through proper critical section protection  
✅ **Resource contention resolution** in shared environments

## Potential Enhancements

- 🛫 Multiple runways with load balancing
- 🌐 Network-based distributed simulation
- 📱 GUI visualization with real-time animation
- 📊 Advanced statistics (wait time distribution, throughput analysis)
- ⚡ Different scheduling algorithms (FCFS, Round-Robin, Weighted Fair Queuing)
- 🔧 Configurable checkpoint intervals
- 📝 Logging to file for post-simulation analysis

## References

1. [Semaphores in Operating Systems - GeeksforGeeks](https://www.geeksforgeeks.org/semaphores-in-operating-system)
2. [OS CPU Scheduling - JavaTpoint](https://www.javatpoint.com/os-cpu-scheduling)
3. [FAA Air Traffic Publications](https://www.faa.gov/air_traffic/publications)
4. POSIX Threads Programming - Lawrence Livermore National Laboratory
5. Operating System Concepts by Silberschatz, Galvin, and Gagne

## Project Team Roles

- **Project Manager**: Coordination, documentation, integration
- **Design Engineer**: System architecture, scheduling algorithms, flow diagrams
- **Technical Team**: Implementation, testing, debugging

## License

This project is created for educational purposes as part of an Operating Systems course.

---

**Note**: This simulation demonstrates OS concepts in a controlled environment. Real-world air traffic control systems involve significantly more complexity, safety protocols, and regulatory compliance.
