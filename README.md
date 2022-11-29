# ScOSA SpaceWire Simulator
This project is for creating OMNeT++ simulations of SpaceWire network based on a system description in JSON (exported from VirSat ScOSA). The main author is Alan Aguilar Jaramillo.

## Installation

The following procedure has only been tested for Windows 10. However, it should apply to any system that can support OMNeT++. 

- Download and Install OMNeT++ in your system (see OMNeT++ Installation Guide https://doc.omnetpp.org/omnetpp/InstallGuide.pdf).
- Open OMNeT++ and set the Workspace directory to the desired folder.
- Clone this simulator and import it into ONMET++ Workspace.
- Validate the install by opening  ```scosa-simulator/simulations/omnet.ini ``` file in OMNeT++ and pressing the green arrow in the menu bar to run the simulation.

## Set-up and Running a Simulation
The simulator is designed to read JSON files and recreate the network as described in them. VirSat ScOSA allows for the user to output all possible configurations to a user-defined directory. To recreate these networks from said files, the user must follow the next procedure:

- Export network configuration JSON files from VirSat ScOSA.
- Ensure that the files follow the naming scheme ```config<CONFIGURATION NUMBER>.json```.
- Place folder containing the exported JSON files into the ```scosa-simulator/input``` directory.
- Set simulation parameters to match scenario (see user guide document).
- Run network simulation in OMNeT++.

## Output processing

We added some Python scripts in [output](/output) for analyzing and visualizing simulation results. The code runs on Python 3.8 and uses the following libraries:
- pandas (BSD 3-Clause License)
- numpy (BSD 3-Clause License)
- matplotlib (PSF-based license)

## License

This project is [MIT licensed](/LICENSE).
Copyright © 2021 German Aerospace Center (DLR), Institute for Software Technology.

## Third Party Licenses

This project uses
- `nlohmann/json` library. Copyright (c) 2013-2020 Niels Lohmann <https://nlohmann.me>. License: [LICENSE.MIT](/src/nlohmann-json/LICENSE.MIT)
