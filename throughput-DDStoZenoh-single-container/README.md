
# Single container DDS to Zenoh throughput test


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
# Inside the folder "throughput-DDStoZenoh-single-container"
# Build the docker image
docker build -t ts-dds-zenoh-img .

# Run the container
docker run -d --name ts-dds-zonoh ts-dds-zenoh-img

# Activate virtual environment
source ~/.venv/zenoh/bin/activate

# Run python api to see the throughput result
python3 throughput_msg_per_sec.py

# Stop containers after test is finished
docker stop ts-dds-zonoh

docker rm ts-dds-zonoh
```
