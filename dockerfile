# Use Ubuntu 20.04 as the base image
FROM ubuntu:20.04

# Avoid interactive prompts during package install
ENV DEBIAN_FRONTEND=noninteractive

# Install build-essential and nlohmann-json3-dev
RUN apt-get update && \
    apt-get install -y build-essential nlohmann-json3-dev && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy the entire project into the container
COPY . .

# Build the project using the Makefile
RUN make

# Expose the NATS server port
EXPOSE 4222

# Set the default command to run the server
CMD ["./build/nats"]