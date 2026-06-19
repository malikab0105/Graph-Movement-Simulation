# Graph-Movement-Simulation

## Members :
1. Malik Abdeh - Team lead, repository management, milestones, compilation, code implementation & enhancements

2. Mohammad Mashal - Core implementation, raylib visualization, testing

3. Daoud Sayyad - Code to animation, raylib utilization

## Project Description
A directed weighted graph (DAG) simulation built in C. The graph will simulate a cat navigates through the rooms of a grand mansion. Each room is a node, each hallway a directed edge with a weight representing the distance to traverse it. 
Dijkstra's algorithm finds the shortest path for the cat to travel between any two rooms in the mansion.


## Milestones:


### Milestone 1 — Graph Representation & Dijkstra
Implementation of a DAG using an adjacency matrix (double pointer), file reading, and Dijkstra's algorithm to find the shortest path between two nodes.

**Compile:**
```bash
make milestone1
```
**Run:**
```bash
cd milestone1 && ./dijkstra <filename with extension>
```

### Milestone 2 — Graph Visualization
Visual representation of the graph using raylib, displaying a generic graph: nodes as circles, directed edges as arrows with weight labels, arranged in a circular layout.

**raylib is required!!!**

**Compile:**
```bash
make milestone2
```
**Run:**
```bash
cd milestone2 && ./sim <filename with extension>
```

### Milestone 3 — Animation
An animated cat entity travels along the Dijkstra shortest path through the Grand Mansion. Features a play/stop button, weight-based edge traversal speed, and 1 second waiting time at every node reached.
NOTE: in this commitment, the animation code was mostly done by https://github.com/DaoudSayyad . however, due to technical issues, he was unable to commit the changes. Hence it was passed down to the team leader.

**Compile:**
```bash
make milestone3
```
**Run:**
```bash
cd milestone3 && ./sim <filename with extension>
```


### Milestone 4 — Concurrent Multi-Processing
Multiple traveler entities navigate through the graph concurrently, each calculating its own Dijkstra shortest path. individual traveler color palettes, and full POSIX multi-process tracking where a background child process is spawned for each traveler and safely terminated via signals upon arrival. NOTE: in this commitment, the extended parsing and multi-process architecture was implemented by the team leader to scale the simulation frame loops.

**Compile:**
```bash
make milestone4
```
**Run:**
```bash
cd milestone4 && ./sim <filename with extension>
```


### Milestone 5 — Multi-Process Animation with IPC
Multiple autonomous traveler processes navigate the mansion simultaneously.
Each child process computes its own Dijkstra path independently and reports
its position to the parent via a Named Pipe (FIFO). The parent manages the
GUI and prints a log of all traveler movements to the terminal.

**IPC Method:** Named Pipe (FIFO) — chosen for its simplicity and suitability
for multiple writers (children) sending to a single reader (parent). Each
message contains the traveler's PID, current node, next node, and whether
they have reached their destination.

**Compile:**
```bash
make milestone5
```
**Run:**
```bash
cd milestone5 && ./sim 
```



### Milestone 7 — Scheduling Algorithms (FCFS / SJF)

The parent process now uses a central scheduler to decide which waiting traveler
enters a node next, instead of allowing children to compete directly via semaphores.
Two scheduling algorithms are supported:

| Algorithm | Description |
|-----------|-------------|
| **FCFS** (First-Come, First-Served) | The traveler that arrived at the node earliest enters first. |
| **SJF** (Shortest Job First) | The traveler with the shortest next edge weight enters first; ties are broken by arrival time. |

**CLI format:**
```bash
./sim -schd fcfs <input_file>
./sim -schd sjf <input_file>
```

**Effect on execution times:** SJF can reduce total waiting time by letting
travelers with short remaining edges pass quickly, while FCFS treats all
travelers equally regardless of their remaining path length. For the same
input, SJF typically yields lower average wait times when travelers have
uneven edge weights.

**New GUI features:**
- Scheduler banner at the top showing the active algorithm
- Red queue-badge circles with a count next to each node

**Metrics:** When all travelers finish, a table is printed showing each
traveler's total wait time and how many times they were queued.

**Compile:**
```bash
make milestone7
```
**Run:**
```bash
cd milestone7 && ./sim -schd fcfs input.txt
```

### Milestone 6 — Mutual Exclusion with Semaphores
Each room node is protected by a binary semaphore — only one cat can occupy
a node at a time. Travelers that arrive at an occupied node wait just outside
it (shown with a red ring) until the node is released. The parent process
manages the GUI and logs all movement events to the terminal.

**IPC Method:** Named Pipe (FIFO) — all child processes write messages to a
single shared pipe, and the parent reads from it. Each message contains the
traveler's PID, current node, next node, and message type (WAITING, ARRIVED,
or FINISHED), allowing the parent to identify and update each traveler's
visual state independently.

**Synchronization Mechanism:** POSIX Semaphores in shared memory — one
semaphore per node, initialized to 1 (binary semaphore / mutex). Before
entering a node, each child calls `sem_wait` which blocks if the node is
occupied. After spending 1 second in the node, the child calls `sem_post`
to release it, allowing the next waiting traveler to enter. The semaphores
are allocated via `mmap` with `MAP_SHARED` so they are shared across all
forked processes.

**GUI indicators:**
- Each cat is drawn in a different color
- A red ring around a cat means it is waiting outside an occupied node

**Compile:**
```bash
make milestone6
```
**Run:**
```bash
cd milestone6 && ./sim <input_file>
```


