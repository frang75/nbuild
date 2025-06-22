# nbuild, Yet Another Ci/Cd

**NBuild in Action:** https://nappgui.com/builds/en/builds/bindex.html

_nbuild_ is a continuous integration system oriented to C/C++ projects based on CMake. Unlike other solutions, this is not a general purpose one. _nbuild_ focuses on compiling and testing C/C++ projects through a heterogeneous computer network, nothing more. We can see it as a macro-compiler or a compiler orchestrator. It is used daily in the development of NAppGUI-SDK, whose reports are available through the [BUILDS](https://nappgui.com/builds/en/builds/bindex.html).

* Written in ANSI-C90, using the NAppGUI-SDK itself.

* Compilation, testing and packaging of C/C++ projects based on CMake.

* Oriented to Open-Source projects, where the source code is distributed.

* Minimal configuration in the nodes (runners) where it is only necessary to install SSH, CMake and the compilers.

* No cloud support or Internet connection required. Works on a local area network.

* It runs a fixed function pipeline, so there is no need to program scripts. Just indicate, inside `workflow.json`, which code we want to compile and for which platforms.

* There are no control panels. It runs as a command line tool.

* Every project can define its own build network.

* Works with legacy nodes such as Ubuntu12, WindowsXP or MacOSX Snow Leopard.

* A relatively large workflow can be divided into several stages of different priority, obtaining partial results as quickly as possible.

* Multithreading. Control different nodes in parallel.

* Scheduler. Matches jobs with available nodes.

* Non-blocking. The _nbuild_ execution never interfere with or block new changes to the repository.

* _nboot_. Ability to turn on/off machines on demand, at the moment they are needed.

* _ndoc_. Generation of documentation associated with the project (HTML5/LaTeX/PDF).

* _nreport_. Generates web reports with the result of each compilation.

## Build the project

Just clone the repository and use CMake to build it:

```
cmake -G [generator] -S . -B build
cmake --build build --config Release
cmake --install build --config Release
```

In `build/Release/bin` you'll find two executables: `nbuild` and `ndoc`.

## Supported runners

_nbuild_ supports build runners from WindowsXP, Snow Leopard and Ubuntu12, and several virtualization technologies: VirtualBox, VMware, UTM.

![IMG_20250616_163115](https://github.com/user-attachments/assets/fe9395fd-3a0f-4ba9-8af1-689bd5f035e1)

## More info

[NBuild documentation site](https://nappgui.com/en/nbuild/nbuild.html).

