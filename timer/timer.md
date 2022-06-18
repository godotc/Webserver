# Timer for handle inactive connections

- As inactive conn occupy the resources, do more effect to server perfromance.

- Hanle these inactive conn and release resources by implenmentting a **Timer**.
- Employ the **alarm function** to trigger **semaphore SIGALARM** regularly, its signal hanlder function **notify main loop** for implement the timer task in Timer Link List;

  ---
    1. Unitive event source
    2. Timer that based on ascending link list
    3. handle inactive connections