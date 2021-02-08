## Instructions

1. Install Docker for your system, following the instructions at https://docs.docker.com/get-started/.
2. Switch to this directory:
```bash
cd libeos/docker
```
3. Build the container here with:
```bash
docker build -t libeos-container .
```
If this fails with a 'Permissions Denied' error, you may need to retry with `sudo`.
4. Run the built container with:
```bash
docker run libeos-container
```
The container will start, exposing a Bash shell within Ubuntu 18.04
with libeos and all dependencies (libconfig, BITFLIPS) pre-installed.


