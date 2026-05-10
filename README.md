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

### Milestone 3 — Animation
An animated cat entity travels along the Dijkstra shortest path through the Grand Mansion. Features a play/stop button, weight-based edge traversal speed, and 1 second waiting time at every node reached.
NOTE: in this commitment, the animation code was mostly done by https://github.com/DaoudSayyad . however, due to technical issues, he was unable to commit the changes. Hence it was passed down to the team leader.

**Compile:**
```bash
make milestone3
```
**Run:**
```bash
cd milestone3 && ./milestone3
```
