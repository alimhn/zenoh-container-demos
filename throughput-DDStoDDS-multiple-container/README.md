
# Multiple container DDS to DDS throughput test

This example demonstrates a DDS network and Zenoh DDS bridge in containers. Both DDS and Zenoh networks are isolated from the `host` network. Zenoh router is exposed at `localhost:7474` and allows local Zenoh apps to read the `ddsperf` application's `DDSPerfRDataKS` topic over the bridge.

## Prerequisites

- A container runtime engine such as `docker` or `podman`
- A container compose provider such as `docker-compose` or `podman-compose`
- Python `eclipse-zenoh` and `cyclonedds` libraries


## Requirements
```sh
# Install system dependencies and create Zenoh virtual environment
sudo apt install python3 python3-pip python3-venv

# Create virtual environment
python3 -m venv ~/.venv/zenoh

# Activate virtual environment
source ~/.venv/zenoh/bin/activate

# Install Zenoh python client
python3 -m pip install eclipse-zenoh cyclonedds
```
## Run throughput test

```sh
# Launch a DDS network and Zenoh bridge in containers 
# Inside the folder "throughput-DDStoDDS-multiple-container"
docker compose up -d

# Open a terminal and run following to enter talker:
docker exec -it throughput-ddstodds-multiple-container-talker-1 /bin/bash

# Now you are inside container. Then ... (you can change the size)
ddsperf pub size 1k

# Open another terminal and run following to enter listener:
docker exec -it throughput-ddstodds-multiple-container-listener-1 /bin/bash

# Now you are inside container. Then ...
ddsperf -Qrss:1 sub

## You can see the throughput results inside the listener terminal. 

# Stop containers after test is finished
docker compose down
```

## Debugging

Enter into DDS talkar container:
```shell
docker exec -it cyclonedds-perf-talker-1 /bin/bash
```

List all participants with available topic names:
```shell
# inside throughput-ddstodds-multiple-container-talker-1
ddsls -a
```

## ToDo

We need to write a bash script to parse cmd and fetch the metric