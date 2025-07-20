#!/bin/bash

# This script attempts to install necessary build tools and libraries for the project.
# It supports Debian/Ubuntu (apt), Fedora/CentOS (dnf/yum), and Arch Linux (pacman).

echo "Detecting package manager and installing dependencies..."

# Update package lists
if command -v apt &> /dev/null; then
    echo "Using apt (Debian/Ubuntu)..."
    sudo apt update
elif command -v dnf &> dev/null; then
    echo "Using dnf (Fedora)..."
    sudo dnf check-update
elif command -v yum &> /dev/null; then
    echo "Using yum (CentOS/RHEL)..."
    sudo yum check-update
elif command -v pacman &> /dev/null; then
    echo "Using pacman (Arch Linux)..."
    sudo pacman -Sy
else
    echo "No supported package manager found (apt, dnf, yum, pacman)."
    echo "Please install cmake, g++, glfw, and X11 development libraries manually."
    exit 1
fi

# Install common build tools
if command -v apt &> /dev/null; then
    sudo apt install -y cmake g++ build-essential
elif command -v dnf &> /dev/null; then
    sudo dnf install -y cmake gcc-c++ make
elif command -v yum &> /dev/null; then
    sudo yum install -y cmake gcc-c++ make
elif command -v pacman &> /dev/null; then
    sudo pacman -S --noconfirm cmake gcc base-devel
fi

# Install project-specific libraries
if command -v apt &> /dev/null; then
    sudo apt install -y libglfw3-dev libopengl-dev libx11-dev libxrandr-dev libxinerama-dev libxi-dev libxcursor-dev
elif command -v dnf &> /dev/null; then
    sudo dnf install -y glfw-devel mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXi-devel libXcursor-devel
elif command -v yum &> /dev/null; then
    sudo yum install -y glfw-devel mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXi-devel libXcursor-devel
elif command -v pacman &> /dev/null; then
    sudo pacman -S --noconfirm glfw mesa xorg-server-devel libxrandr libxinerama libxi libxcursor
fi

echo "Dependency installation attempt complete."
echo "You may need to run 'sudo ldconfig' if new libraries were installed."
