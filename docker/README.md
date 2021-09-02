## Instructions

1. Install Docker for your system, 
following the instructions at https://docs.docker.com/get-started/.
2. Clone this repository:
```bash
git clone https://github.com/JPLMLIA/libeos.git
```
3. Switch to this directory:
```bash
cd libeos/docker
```
4. Build the container here with:
```bash
sudo docker build -t libeos-container .
```
5. Run the built container with:
```bash
sudo docker run -i -t libeos-container
```
The container will start, exposing a Bash shell within Ubuntu 18.04
with libeos and all dependencies (libconfig, BITFLIPS) pre-installed.


