FROM python:3.10 as runner
ENV DEBIAN_FRONTEND=noninteractive

ARG ZENOHD_VERSION=0.10.1-rc-1

# Install required packages
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y python3 python3-pip cyclonedds-tools && \
    python3 -m pip install --no-cache-dir cyclonedds

# Install Zenoh dependencies
RUN echo "deb [trusted=yes] https://download.eclipse.org/zenoh/debian-repo/ /" | tee -a /etc/apt/sources.list.d/zenoh.list > /dev/null && \
    apt-get update && \
    apt-get install -y dpkg sudo systemctl zenohd=${ZENOHD_VERSION} zenoh-bridge-dds=${ZENOHD_VERSION} zenoh-plugin-rest=${ZENOHD_VERSION}

# Copy the entrypoint script into the container
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
COPY zenohd.yaml /etc/zenohd/zenohd.yaml

# Set the execute permission on the entrypoint script
RUN chmod +x /usr/local/bin/entrypoint.sh

# Specify the entrypoint script to be run when the container starts
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

# Expose required ports
EXPOSE 7446/udp 7447/tcp 8000/tcp

ENV RUST_LOG info

# TODO: Purge the cache and remove the apt lists
