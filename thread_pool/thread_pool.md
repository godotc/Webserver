# Semi-synchronous & Semi-reactor THREAD_POOL

- using a work **queue** completely relieve coupling (解除耦合) between main and work thread:
- **Main thread** inserts work into queue, **work threads** compete to get work then finish it;

    1. Synchronous I/O that simulated  proactor mode;
    2. Semi-syschronization/semi-reactor
    3. Thread pool