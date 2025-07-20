# ---- Stage 1: Build and Test ----
# Use Ubuntu 20.04 as the base image
FROM ubuntu:20.04 AS buildtest

# Avoid interactive prompts during package install
ENV DEBIAN_FRONTEND=noninteractive

# Install build-essential, nlohmann-json3-dev, GoogleTest and GoogleMock
RUN apt-get update && \
    apt-get install -y build-essential nlohmann-json3-dev libgtest-dev libgmock-dev cmake make && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy the entire project into the container
COPY . .

# Build GoogleTest and GoogleMock (they are source-only packages)
RUN cd /usr/src/gtest && cmake . && make && cp lib/*.a /usr/lib/

# Build and run tests (fail build if tests fail)
RUN make test

# ---- Stage 2: Production ----
FROM ubuntu:20.04

RUN apt-get update && \
    apt-get install -y libstdc++6 nlohmann-json3-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy only the production binary from the build stage
COPY --from=buildtest /app/build/nats /app/build/nats

EXPOSE 4222

CMD ["./build/nats"]