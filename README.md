# Frame Graph


This is an implementation that will initially be used to do performance analysis for the CSC746 - High-Performance Computing class.


While also be used for the Atlas Engine. Which is a game engine I am building concurrent to taking this class.

## Case Study

Render graphs are quite a significant big portion of AAA rendering systems (blender, houdini, etc). The case study here is these graphs have to be optimized and reduce certain dataflows while also being frame-sensitive.

One of the things I have wanted to test is where events that can be causes of bottlenecks occur at the GPU hardware level. Therefore, implementing a render graph myself and evaluating using NVidia NSight to help profile when these events occur.

## How does Render Graphs work?

To summarize the flow of a render graph using a basic exmaple. Here is how it would go:

```
1.) Analyze Dependencies
2.) Build the DAG
3.) Barrier Synchronization
4.) Generate GPU Commands
5.) Execute commands over to the GPU
```

## Implementation

I am using Vulkan to perform GPU operations. So, regarding the code in this repository will be using Vulkan as it allows me to have a agnostic API that I can run on both of my machine Windows and Mac.

## Performance Metrics for Hardware Profiling

One of the things I began asking is what performance metrics do I want to consider as part of my case study. This would be synchronization, analyzing render target dependencies, and when stalling occurs on GPU hardware.

- `GPU Stalling` (and Synchronization): This metric is useful because when performing synchronization via Vulkan's **pipeline barriers**. This essentially will tell the GPU to stop until all necessary dependencies have finished just before executing. This is something I would like to evaluate if this causes the GPU to stall.
- `Render Target Dependencies`: During each frame, if for any reason the dependencies within the graph change before the next frame. Whether the user decides to disbale tonemapping, etc. Then that is going to be re-evaluated and building the DAG. Before synchronization and executing onto the GPU. Analyzing how this can be effecting each frame can be useful for evaluting in milliseconds, the cause of frame instability.
