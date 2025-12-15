# Airport Runway Management System - Architecture

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     MAIN PROGRAM                            │
│  - Parse command-line arguments                             │
│  - Initialize runway system                                 │
│  - Create N plane threads (staggered arrival)               │
│  - Wait for completion (pthread_join)                       │
│  - Display statistics                                       │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                 RUNWAY SYSTEM (Global)                      │
├─────────────────────────────────────────────────────────────┤
│  Semaphores:                                                │
│    • runway_access (Binary: runway availability)            │
│    • console_access (Binary: thread-safe output)            │
│    • emergency_queue_sem (Counting: emergency queue size)   │
│    • normal_queue_sem (Counting: normal queue size)         │
│    • emergency_flag_sem (Binary: protect emergency flag)    │
│                                                             │
│  Queues:                                                    │
│    • emergency_queue (Dynamic Linked List)                  │
│    • normal_queue (Dynamic Linked List)                     │
│                                                             │
│  State:                                                     │
│    • active_plane (Currently using runway)                  │
│    • emergency_flag (Signal for preemption)                 │
│    • statistics counters                                    │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
        ┌─────────┴─────────┐
        │                   │
        ▼                   ▼
┌───────────────┐   ┌───────────────┐
│  EMERGENCY    │   │    NORMAL     │
│    QUEUE      │   │    QUEUE      │
│(High Priority)│   │ (Low Priority)│
└───────┬───────┘   └───────┬───────┘
        │                   │
        └─────────┬─────────┘
                  │
                  ▼
        ┌──────────────────┐
        │PRIORITY SCHEDULER│
        │  (Always checks  │
        │ emergency first) │
        └─────────┬────────┘
                  │
                  ▼
        ┌──────────────────┐
        │   RUNWAY ACCESS  │
        │   (Single runway │
        │    protected by  │
        │    semaphore)    │
        └─────────┬────────┘
                  │
                  ▼
        ┌──────────────────┐
        │  RUNWAY OPERATION│
        │   - 500ms chunks │
        │   - Checkpoint   │
        │   - Preemption   │
        │     checking     │
        └──────────────────┘
```

## Plane Thread State Machine

```
    [CREATE]
       │
       ▼
   ┌─────────┐
   │ WAITING │ ◄──────────────┐
   └────┬────┘                │
        │                     │
        │ Queue Entry         │ Re-queue with
        ▼                     │ checkpoint
 ┌─────────────┐              │
 │ APPROACHING │              │
 └──────┬──────┘              │
        │                     │
        │ Acquire Runway      │
        ▼                     │
 ┌──────────────┐             │
 │ USING_RUNWAY │             │
 └──────┬───────┘             │
        │                     │
        │ Preemption?         │
        ├─────────────────────┘
        │ Yes (save checkpoint)
        │
        │ No (continue)
        ▼
 ┌───────────┐
 │ COMPLETED │
 └───────────┘
```

## Preemption Flow

```
Normal Plane Operating on Runway
         │
         ▼
  [Every 500ms checkpoint]
         │
         ▼
  Check emergency_flag
         │
    ┌────┴────┐
    │         │
   No        Yes
    │         │
    │         ▼
    │   Save checkpoint (e.g., 65%)
    │         │
    │         ▼
    │   Release runway semaphore
    │         │
    │         ▼
    │   Re-queue to normal queue
    │         │
    │         ▼
    │   Emergency plane uses runway
    │         │
    │         ▼
    │   Emergency completes
    │         │
    │         ▼
    │   Normal plane re-acquires runway
    │         │
    │         └────────┐
    │                  │
    ▼                  ▼
Continue          Resume from 65%
to 100%          checkpoint to 100%
```

## Semaphore Protection Patterns

### Binary Semaphore (Mutex Replacement)

```
sem_wait(&resource_sem);   // Acquire lock (P operation)
// ... critical section ...
sem_post(&resource_sem);   // Release lock (V operation)
```

### Counting Semaphore (Queue Signaling)

```
// Producer (Plane enqueuing)
queue_enqueue(&queue, plane);
sem_post(&queue_sem);      // Signal: queue has item

// Consumer (Scheduler)
sem_wait(&queue_sem);      // Wait: until queue non-empty
plane = queue_dequeue(&queue);
```

## Priority Scheduling Algorithm

```
SCHEDULER:
    WHILE planes_remaining > 0:
        // Check emergency queue first (priority)
        IF sem_trywait(&emergency_queue_sem) == SUCCESS:
            sem_wait(&runway_access)
            plane = dequeue(emergency_queue)
            GRANT_RUNWAY(plane)

        // Otherwise, check normal queue
        ELSE IF sem_trywait(&normal_queue_sem) == SUCCESS:
            sem_wait(&runway_access)
            plane = dequeue(normal_queue)
            GRANT_RUNWAY(plane)
```

## Checkpoint/Resume Mechanism

```
Operation Duration: 5 seconds (5000ms)
Checkpoint Interval: 500ms
Interruption at: 2500ms (50% complete)

Timeline:
0ms    ────────────────────────
       |   Plane starts operation
500ms  ────────────────────────
       |   Checkpoint: 10%
1000ms ────────────────────────
       |   Checkpoint: 20%
1500ms ────────────────────────
       |   Checkpoint: 30%
2000ms ────────────────────────
       |   Checkpoint: 40%
2500ms ────────────────────────
       |   Checkpoint: 50% ← EMERGENCY DETECTED
       |   Save state, yield runway
       └─→ [INTERRUPTED]

[... Emergency operates ...]

Resume:
0ms    ────────────────────────
       |   Resume from 50%
       |   Remaining: 2500ms
500ms  ────────────────────────
       |   Checkpoint: 60%
1000ms ────────────────────────
       |   Checkpoint: 70%
1500ms ────────────────────────
       |   Checkpoint: 80%
2000ms ────────────────────────
       |   Checkpoint: 90%
2500ms ────────────────────────
       |   Complete: 100% ✓
```

## Data Structures

### Plane Structure

```c
struct Plane {
    int id;                      // Unique identifier
    OperationType operation;     // LANDING or TAKEOFF
    PriorityLevel priority;      // EMERGENCY or NORMAL
    PlaneState state;            // Current state
    int checkpoint_progress;     // 0-100%
    time_t arrival_time;         // Statistics
    time_t start_time;           // Statistics
    time_t completion_time;      // Statistics
    pthread_t thread;            // Thread handle
    sem_t resume_sem;            // Signal for resume
};
```

### Queue Node Structure

```c
struct QueueNode {
    Plane* plane;                // Pointer to plane
    QueueNode* next;             // Next node in list
};

struct Queue {
    QueueNode* head;             // Front of queue
    QueueNode* tail;             // Back of queue
    int count;                   // Number of items
    sem_t sem_access;            // Protect queue ops
};
```

## Synchronization Guarantees

1. **Mutual Exclusion**: Only one plane on runway at a time
2. **Progress**: No deadlock - all planes eventually complete
3. **Priority**: Emergency always preempts normal operations
4. **Fairness**: Normal planes served in FIFO order
5. **Thread Safety**: All shared resources protected by semaphores
6. **Atomicity**: Queue operations are atomic

## Performance Characteristics

- **Time Complexity**:
  - Enqueue: O(1)
  - Dequeue: O(1)
  - Queue peek: O(1)
- **Space Complexity**:

  - Queue: O(n) where n = number of waiting planes
  - Overall: O(total_planes)

- **Checkpoint Overhead**:
  - Check every 500ms
  - Minimal CPU impact
  - State save: O(1)

## Real-World Mapping

| Simulation Component | Real-World Equivalent                 |
| -------------------- | ------------------------------------- |
| Plane (Thread)       | Aircraft requesting runway            |
| Runway Semaphore     | Physical runway availability          |
| Emergency Queue      | Priority clearance for emergencies    |
| Normal Queue         | Standard air traffic queue            |
| Checkpoint Progress  | Aircraft position in approach/takeoff |
| Preemption           | Emergency override protocol           |
| Semaphore Wait       | Holding pattern/waiting for clearance |
| Console Output       | Air traffic control communication     |
