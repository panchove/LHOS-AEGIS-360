# Base Image
FROM ubuntu:22.04

# General dependencies
RUN apt-get update -y
RUN apt-get install -y \
    build-essential \
    software-properties-common \
    unzip \
    curl \
    sudo \
    git
RUN apt-add-repository ppa:deadsnakes/ppa
RUN apt-get update -y

# Receive the version to install from the build command
ARG PYTHON_VERSION=3.10

# Install python and pip
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y \
    "python${PYTHON_VERSION}" \
    "python${PYTHON_VERSION}-dev" \
    "python${PYTHON_VERSION}-distutils" \
    python3-venv \
    python3-virtualenv \
    python3-setuptools

# Replace the python3 command to the installed version
RUN update-alternatives --install /usr/bin/python3 python3 "/usr/bin/python${PYTHON_VERSION}" 1

# Clean cache
# RUN apt-get clean
# RUN rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

RUN curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
RUN python3 get-platformio.py

# Install the platformio CLI
RUN python3 -m pip install -U platformio boto3

WORKDIR /app

CMD [ "/bin/bash" ]
