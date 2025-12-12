🚀 𝗨𝗻𝗹𝗼𝗰𝗸𝗶𝗻𝗴 𝗣𝗮𝗿𝗮𝗹𝗹𝗲𝗹 𝗣𝗼𝘁𝗲𝗻𝘁𝗶𝗮𝗹: 𝗛𝗶𝗴𝗵-𝗣𝗲𝗿𝗳𝗼𝗿𝗺𝗮𝗻𝗰𝗲 𝗖++𝟭𝟳 𝗠𝗮𝘇𝗲 𝗦𝗼𝗹𝘃𝗲𝗿 𝗔𝗻𝗮𝗹𝘆𝘀𝗶𝘀

I built high-performance, multi-threaded maze solvers in C++17, optimized for high-end multicore CPUs.

1️⃣ 𝗧𝗲𝗰𝗵 𝗦𝘁𝗮𝗰𝗸

To fully exploit the hardware capabilities of the i9-14900KF, all algorithms were implemented using the 𝗖++𝟭𝟳 𝗠𝘂𝗹𝘁𝗶-𝘁𝗵𝗿𝗲𝗮𝗱 𝗟𝗶𝗯𝗿𝗮𝗿𝘆. This choice was critical for reducing overhead in synchronization primitives (mutexes, atomics) and maximizing thread throughput in the concurrent solvers.

2️⃣ 𝗔𝗹𝗴𝗼𝗿𝗶𝘁𝗵𝗺𝘀

𝘚𝘪𝘯𝘨𝘭𝘦 𝘛𝘩𝘳𝘦𝘢𝘥 𝘉𝘍𝘚 (𝘉𝘳𝘦𝘢𝘥𝘵𝘩-𝘍𝘪𝘳𝘴𝘵 𝘚𝘦𝘢𝘳𝘤𝘩) 📏:

  • 𝗠𝗲𝗰𝗵𝗮𝗻𝗶𝘀𝗺: Explores the maze layer-by-layer using a queue. Guarantees the shortest path (in unweighted graphs) but requires massive memory and processing for deep mazes.
  
  • 𝗣𝗿𝗼𝘀/𝗖𝗼𝗻𝘀: Simple to implement, but suffers from exponential search space expansion in large mazes.

𝘚𝘪𝘯𝘨𝘭𝘦 𝘛𝘩𝘳𝘦𝘢𝘥 𝘋𝘍𝘚 (𝘋𝘦𝘱𝘵𝘩-𝘍𝘪𝘳𝘴𝘵 𝘚𝘦𝘢𝘳𝘤𝘩) 🗂:

  • 𝗠𝗲𝗰𝗵𝗮𝗻𝗶𝘀𝗺: Explores the maze layer-by-layer using a queue. Guarantees the shortest path (in unweighted graphs) but requires massive memory and processing for deep mazes.
  
  • 𝗣𝗿𝗼𝘀/𝗖𝗼𝗻𝘀: Memory efficient and often faster than BFS on single-solution mazes, but can get "lucky" 🍀 or "unlucky" 💀 depending on branch ordering.

𝘔𝘶𝘭𝘵𝘪𝘱𝘭𝘦 𝘛𝘩𝘳𝘦𝘢𝘥𝘴 𝘔𝘦𝘵𝘩𝘰𝘥 1 (𝘗𝘢𝘳𝘢𝘭𝘭𝘦𝘭 𝘗𝘳𝘶𝘯𝘪𝘯𝘨 + 𝘉𝘪𝘥𝘪𝘳𝘦𝘤𝘵𝘪𝘰𝘯𝘢𝘭) ✂️:

  • 𝗠𝗲𝗰𝗵𝗮𝗻𝗶𝘀𝗺: Uses a concurrent "Prune and Pursue" strategy. Dedicated threads seal dead-ends, while a Bottom-to-Top (BT) thread executes a pruning-compatible BFS. Simultaneously, the Top-to-Bottom (TB) thread simply advances along the path carved by the pruning threads.
  
  • 𝗢𝗽𝘁𝗶𝗺𝗶𝘇𝗮𝘁𝗶𝗼𝗻: Efficiency is maximized by partitioning the maze into distinct sections, enabling interference-free parallel pruning. This dynamic reduction actively shrinks the search space in real-time, preventing the BT thread from wasting cycles on dead-ends and paving a clear path for the TB thread.

𝘔𝘶𝘭𝘵𝘪𝘱𝘭𝘦 𝘛𝘩𝘳𝘦𝘢𝘥𝘴 𝘔𝘦𝘵𝘩𝘰𝘥 2 (𝘊𝘰𝘭𝘭𝘢𝘣𝘰𝘳𝘢𝘵𝘪𝘷𝘦 𝘉𝘪𝘥𝘪𝘳𝘦𝘤𝘵𝘪𝘰𝘯𝘢𝘭 𝘋𝘍𝘚) 🔄:

  • 𝗠𝗲𝗰𝗵𝗮𝗻𝗶𝘀𝗺: Launches multiple threads split into two groups: one searching Top-to-Bottom (TB) and one Bottom-to-Top (BT).
  
  • 𝗢𝗽𝘁𝗶𝗺𝗶𝘇𝗮𝘁𝗶𝗼𝗻: Threads share a global "visited" state. When a thread enters a branch, it marks it "Occupied" 🚧 to prevent redundant work. Dead-ends are marked "Dead" 💀 globally, permanently pruning the search for all other threads. The search concludes when a TB thread overlaps with a BT thread.

3️⃣ 𝗣𝗲𝗿𝗳𝗼𝗿𝗺𝗮𝗻𝗰𝗲

<img width="642" height="805" alt="image" src="https://github.com/user-attachments/assets/c1c2a4eb-0b5f-4760-a296-002884abb140" />

4️⃣  𝗔𝗻𝗮𝗹𝘆𝘀𝗶𝘀

  • 𝗠𝗧 𝗠𝗲𝘁𝗵𝗼𝗱 𝟭 (𝗖𝗼𝗻𝗰𝘂𝗿𝗿𝗲𝗻𝘁 𝗣𝗿𝘂𝗻𝗶𝗻𝗴) dominates on Server Hardware. It effectively exploits massive thread counts (180+) on low-frequency CPUs (AMD EPYC) to mass-delete dead ends, scaling linearly to achieve 0.26s (approx. 2.5x faster than Intel).
  
  • 𝗠𝗧 𝗠𝗲𝘁𝗵𝗼𝗱 𝟮 (𝗖𝗼𝗹𝗹𝗮𝗯𝗼𝗿𝗮𝘁𝗶𝘃𝗲 𝗔𝘀𝘆𝗺𝗺𝗲𝘁𝗿𝗶𝗰 𝗧𝗿𝗮𝘃𝗲𝗿𝘀𝗮𝗹 𝗗𝗙𝗦) excels on High-Frequency Workstations by leveraging the Intel i9's clock speed. However, it also demonstrates strong scalability on high-core systems, improving to 0.30s on AMD (vs 0.36s on Intel), proving that its dynamic, differentiated thread roles benefit significantly from increased core density.
  
𝗩𝗲𝗿𝗱𝗶𝗰𝘁:
Method 1 is the top choice for massive parallel throughput (Servers).
Method 2 offers a versatile balance, making it ideal for Workstations while remaining highly competitive and scalable on Server platforms.



🔗 See the visualization of all algorithms in action:
 https://tangzhilinnz.github.io/maze_visualization/

---------------------

![Maze](https://github.com/user-attachments/assets/fa8af537-828c-4b64-878a-ed131d6cb63f)


