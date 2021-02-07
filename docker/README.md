## Instructions

1. Install Docker for your system, following the instructions at https://docs.docker.com/get-started/
2. Switch to this directory.
2. Build the container here with:
```bash
docker build -t libeos-docker .
```
3. Run the built container with:
```bash
docker run libeos-container
```
The container will start, exposing a Bash shell within Ubuntu 18.04
with libeos and all dependencies (libconfig, BITFLIPS) pre-installed.


