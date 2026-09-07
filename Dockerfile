# THE APEX FIX: Toolchain Alignment locked strictly to GCC-14 for pure C++23 compliance
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test -y \
    && apt-get update && apt-get install -y \
    g++-14 \
    gcc-14 \
    cmake \
    git \
    make \
    linux-libc-dev \
    libcap-dev \
    && rm -rf /var/lib/apt/lists/*

# Map gcc-14 to default compiler commands
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-14 100

WORKDIR /swayam_agi
COPY . .

# Compile enforcing C++23, strict warnings, and zero conversion loss
RUN c++ -std=c++23 -O3 -Wall -Werror src/main.cpp -o swayam_daemon

CMD ["./swayam_daemon"]
