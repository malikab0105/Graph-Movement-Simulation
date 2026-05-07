# Graph-Movement-Simulation

## Members :
1. Malik Abdeh - Team lead, repository management, milestones, compilation, code enhancements

2. Mohammad Mashal - Core implementation, raylib visualization, testing

3. Daoud Sayyad - ...

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
cd milestone1 && ./milestone1
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
cd milestone2 && ./milestone2
```

