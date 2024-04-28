
# Single container DDS to DDS throughput test

This example demonstrates a DDS network and Zenoh DDS bridge in containers. Both DDS and Zenoh networks are isolated from the `host` network. Zenoh router is exposed at `localhost:7474` and allows local Zenoh apps to read the `ddsperf` application's `DDSPerfRDataKS` topic over the bridge.

## Prerequisites

- A container runtime engine such as `docker` or `podman`
- A container compose provider such as `docker-compose` or `podman-compose`
- Python `eclipse-zenoh` and `cyclonedds` libraries

## Run throughput test

```sh
# Inside the folder "throughput-DDStoDDS-single-container"
# Build the docker image
docker build -t ts-dds-dds-img .

# Run the container
docker run -d --name ts-dds-dds ts-dds-dds-img

# Open a terminal and run following to enter container:
docker exec -it ts-dds-dds /bin/bash

# Now you are inside container. Then ... (you can change the size)
ddsperf pub size 1k

# Open another terminal and run following to enter container:
docker exec -it ts-dds-dds /bin/bash

# Now you are inside container. Then ...
ddsperf -Qrss:1 sub

## You can see the throughput results inside the second terminal. 

# Stop container after test is finished
docker stop ts-dds-dds

docker rm ts-dds-dds
```

## ToDo

We need to write a bash script to parse cmd and fetch the metric