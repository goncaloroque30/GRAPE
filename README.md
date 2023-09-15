# GRAPE [![License](https://img.shields.io/github/license/goncaloroque30/GRAPE)](LICENSE)

<h1 align="center">
    <img src="res/Icon/GrapeIcon512.png" alt="GRAPE">
</h1>

<p align="center">
    GRAPE is a desktop application for calculating the environmental impacts of aircraft operations around airports. The scope is limited to flight operations (excludes taxiing, ground handling, ...). Aircraft noise, fuel consumption and pollutant emissions are the main focus of GRAPE.
</p>

---

## Getting Started

The software currently only supports Windows.

1. Start by [downloading the latest release to your machine](https://github.com/goncaloroque30/GRAPE/releases/latest/GRAPE.zip).
2. Unzip the downloaded file.
3. Start GRAPE.exe in the unzipped folder.
4. Example studies are provided in the ´examples´ folder.
5. [Read the documentation](https://goncaloroque30.github.io/GRAPE-Docs/) for a description of the tool and its features.

---

## Building from Source

1. Downloading the repository to your machine

    - Clone this repository using `git clone --recursive https://github.com/goncaloroque30/GRAPE.git`.
    - If the repository was cloned without the --recursive option, use `git submodule update --init` to clone the necessary submodules.

2. Building and running GRAPE.exe
    - GRAPE has out of the box support for [CMake](https://cmake.org).
    - The only dependency not included in the repository is the [Vulkan SDK](https://vulkan.lunarg.com/sdk). Make sure to install it and that it is accessible from your build environment (`find_package(Vulkan REQUIRED)` should not result in an error).
    - A simple out of source build can be generated from a command line open at the repository dir:
        ```
        mkdir build
        cd build
        cmake ..
        cmake --build .
        ```

3. Installing GRAPE.exe
    - After successfully building, you can generate the installation tree by running `cmake --install <install dir>`.
    - The GRAPE executable can be found inside the install dir and run from there.

---

## ![Static Badge](https://img.shields.io/badge/Warning-FFFF00)

Due to copyright issues, the public version of the repository does not contain the source code implementation of the [SAE ARP 866](https://www.sae.org/standards/content/arp866b/) and [SAE ARP 5534](https://www.sae.org/standards/content/arp5534/). These documents provide methods to estimate the atmospheric absorption of noise for different atmospheric conditions. The released versions of GRAPE are built from a private repository where both methods are implemented.
