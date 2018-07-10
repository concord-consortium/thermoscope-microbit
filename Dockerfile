FROM ubuntu:18.04

# remove warning messages from debconf (without impacting interactive installs later)
ARG DEBIAN_FRONTEND=noninteractive

# steps taken from http://docs.yottabuild.org/#installing-on-linux
# the pip install was modified to use python-pip package instead of easy install
RUN apt-get update && apt-get -y install python-setuptools  cmake build-essential \
  ninja-build python-dev libffi-dev libssl-dev python-pip

# supporting cross compiling to arm taken from here:
#  http://docs.yottabuild.org/#linux-cross-compile
# I had to add the software-properties-common inorder to get the add-apt-repository command
#   - remove the built-in package, if installed:
#   - set up the PPA for the ARM-maintained package:
#   - install
RUN apt-get -y install apt-utils software-properties-common
RUN apt-get remove binutils-arm-none-eabi gcc-arm-none-eabi && \
  add-apt-repository ppa:team-gcc-arm-embedded/ppa && \
  apt-get update && \
  apt-get -y install gcc-arm-embedded

# steps taken from https://lancaster-university.github.io/microbit-docs/offline-toolchains/#installation-on-linux
RUN pip install yotta

RUN apt-get install -y srecord

RUN mkdir /microbit
WORKDIR /microbit
