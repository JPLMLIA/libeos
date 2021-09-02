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
4. Build the container from the image here with:
```bash
sudo docker build -t libeos-container .
```
5. Run the built container with:
```bash
sudo docker run -i -t libeos-container
```
6. The container will start, exposing a Bash shell within Ubuntu 18.04
  with libeos and all dependencies (libconfig, BITFLIPS) pre-installed.
  Ensure that `libeos` is installed properly by running the tests,
  inside the container's shell:
```bash
> cd libeos
> make run_tests 
```
  You should see all tests pass:
```
...
TestHeapAdd2Then1.............................PASS
TestHeapAdd1Then1.............................PASS
TestHeapAddExtraHigh..........................PASS
TestHeapAddExtraLow...........................PASS
TestHeapAdd12345..............................PASS
TestHeapSortEmpty.............................PASS
OK (125 tests)
```
