
# Multiple container DDS to DDS latency test

This example demonstrates a DDS network and Zenoh DDS bridge in containers. Both DDS and Zenoh networks are isolated from the `host` network. Zenoh router is exposed at `localhost:7474` and allows local Zenoh apps to read the `ddsperf` application's `DDSPerfRDataKS` topic over the bridge.

## Prerequisites

- A container runtime engine such as `docker` or `podman`
- A container compose provider such as `docker-compose` or `podman-compose`
- Python `eclipse-zenoh` and `cyclonedds` libraries


## Run latency test

```sh
# Launch a DDS network and Zenoh bridge in containers 
# Inside the folder "latency-DDStoDDS-multiple-container"
docker compose up -d

# Open a terminal and run following to enter talker:
docker exec -it latency-ddstodds-multiple-container-talker-1 /bin/bash

# Now you are inside container.
ddsperf ping

# Open another terminal and run following to enter listener:
docker exec -it latency-ddstodds-multiple-container-listener-1 /bin/bash

# Now you are inside container. Then ...
ddsperf pong

## You can see the latency results inside the talker terminal. 

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