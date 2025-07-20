# StatsBy0113

## Overview

StatsBy0113 is a lightweight system monitoring application designed to provide real-time insights into your computer's performance on Linux. Built with C++ and leveraging the power of Dear ImGui for its intuitive graphical user interface, this tool allows users to keep a close eye on critical system metrics such as CPU and RAM utilization.

The application is highly configurable, allowing users to customize monitoring parameters and display options to suit their needs. Whether you're a developer, a gamer, or just someone who likes to know what's happening under the hood, StatsBy0113 offers a clear and concise overview of your system's health.

**Note:** This project is currently a work in progress and is primarily developed and tested on Linux, specifically Fedora 42 with KDE Plasma. Full compatibility and stability on other operating systems are not yet guaranteed.

## Features

*   **Real-time System Monitoring:** Track CPU and RAM usage in real-time.
*   **Graphical User Interface:** Clean and responsive UI powered by Dear ImGui.
*   **Configurable Settings:** Customize monitoring intervals, display options, and more via a dedicated configuration manager.


---

## Installation

To get StatsBy0113 up and running on your system, follow these steps:

### Prerequisites

Ensure you have the following installed:

*   A C++ compiler (e.g., GCC, Clang)
*   CMake (version 3.10 or higher recommended)
*   Git

### Building from Source

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/your-username/StatsBy0113.git
    cd StatsBy0113
    ```

2.  **Install Dependencies:**
    The project includes a script to help install necessary dependencies.
    ```bash
    ./install_dependencies.sh
    ```

3.  **Build the application:**
    Use the provided build script to compile the project.
    ```bash
    ./build.sh
    ```
    This will create the executable in the `build/` directory.

## Usage

After successfully building the application, navigate to the `build/` directory and run the executable:

```bash
sudo ./StatsBy0113
```

The application window will appear, displaying your system's CPU and RAM usage. You can interact with the GUI to access settings and other features.

## Configuration

StatsBy0113 uses a configuration file (e.g., `stats_config.ini` or `config.ini` in the `build/` directory) to manage its settings. You can modify this file to adjust various parameters, or use the in-application configuration options if available.

## Contributing

Contributions are welcome! If you have suggestions for improvements, new features, or bug fixes, please feel free to open an issue or submit a pull request.
