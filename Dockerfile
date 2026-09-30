# Builds the gherkies server the way DTU's grading VM does: Ubuntu 24.04 with
# build-essential only (bootstrap.sh installs nothing else - no libssl-dev).
# iproute2/procps are just for diagnostics inside the container (ss, ps).
FROM ubuntu:24.04

RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential iproute2 procps \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN make clean && make

EXPOSE 5003
CMD ["./server", "5003"]
