# Race Car Game - CS-2001 Data Structures Project (FALL 2023)
markdown
Copy
## Overview
- **Project Description:**  
  A 2D console-based race car game built in C++ that uses text-based user interface (TUI) elements. The game features player-controlled race cars, obstacles, tracks, and a scoring system.
- **Objective:**  
  Design and implement an engaging game that integrates various data structures such as graphs, queues, and linked lists to manage game elements efficiently.
markdown
Copy
## Key Features & Data Structures
- **Graph Integration:**  
  - Map representation as a graph where nodes represent waypoints and edges represent paths.
  - Navigation and pathfinding using algorithms (e.g., breadth-first search) to determine optimal routes.
  
- **Queue Utilization:**  
  - Managing the generation and timing of obstacles through a queue data structure.
  
- **Linked List Implementation:**  
  - Tracking collected game items (coins, trophies, etc.) using linked lists for dynamic score and achievement updates.
  
- **Text-Based UI:**  
  - Rendering the game entirely in the console using preloaded character and color buffers.
  - Interactive menus for name input, difficulty selection, pause, and game over screens.
markdown
Copy
## Files & Deliverables
- **Source Code Files:**  
  - `main.cpp`  
  - `Graph.h` / `Graph.cpp`  
  - `Queue.h` / `Queue.cpp`  
  - `Trophy.h`  
  - `Util.h` / `Util.cpp`  
  - `WinUtil.h` / `WinUtil.cpp`  
  - `algorithm.h` / `algorithm.cpp`
  
- **Assets & Resources:**  
  - Character buffers and color files located in the `assets/` folder.
  
- **Documentation:**  
  - A detailed project report (`DataStructures - ProjectFall23.pdf`) documenting design decisions, data structure usage, implementation details, and testing strategies.
markdown
Copy
## Requirements
- **Programming Language:** C++ (C++11 or later)
- **Platform:** Microsoft Windows (uses Windows API for console manipulation)
- **Build Tools:**  
  - A C++ compiler (e.g., Visual Studio, g++ with MinGW)  
  - CMake (optional, if provided for project management)
markdown
Copy
## Compilation & Execution
1. **Clone the Repository:**
   ```bash
   git clone <repository-url>
   cd <repository-directory>

2. **Compile the Code:**
Example using Visual Studio command line or g++ (ensure Windows headers are available):
```
g++ -std=c++11 -o RaceCarGame main.cpp Graph.cpp Queue.cpp Util.cpp WinUtil.cpp algorithm.cpp
```

Alternatively, open the solution/project file in Visual Studio and build the project.

3. **Run the Application:**
```
./RaceCarGame
```
The game will launch in the console, and you can interact with menus, race tracks, and obstacles using the keyboard.


## Output & Deliverables
- **Gameplay Elements:**  
  - Player-controlled race car with real-time movement.
  - Dynamic obstacles and track elements generated using queues and graphs.
  - In-game menus, pause screens, and game over screens.
- **Scoreboard & Inventory:**  
  - Display of collected trophies and scores managed via linked lists.
- **Performance Metrics:**  
  - Real-time FPS display and performance feedback on the console.
  - 
## Documentation & Report
- **Project Report:**  
  The detailed report (`DataStructures - ProjectFall23.pdf`) includes:
  - Explanation of data structure choices (graphs, queues, linked lists) and their integration.
  - In-depth description of core game logic (player controls, collision detection, scoring, and win/lose conditions).
  - Diagrams, tables, and pseudocode for key algorithms such as navigation and obstacle management.
  - Analysis of performance and optimization strategies.

## Contributing
- **How to Contribute:**
  1. Fork the repository and create a new feature branch.
  2. Follow the established coding style and directory structure.
  3. Ensure your modifications are thoroughly tested.
  4. Submit a pull request with a clear description of your changes.

## License
This project is licensed under the MIT License.
