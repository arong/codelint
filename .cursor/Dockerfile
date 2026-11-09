# Cursor Cloud Agent base image for codelint (LLVM 15).
# Do NOT COPY the project — Cursor checks out the correct commit.

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    git \
    gnupg \
    libedit-dev \
    libncurses-dev \
    libxml2-dev \
    libzstd-dev \
    make \
    pkg-config \
    sudo \
    wget \
    zlib1g-dev \
    clang-15 \
    clang-format-15 \
    clang-tidy-15 \
    libclang-15-dev \
    llvm-15-dev \
    && rm -rf /var/lib/apt/lists/*

RUN ln -sf /usr/bin/clang-format-15 /usr/local/bin/clang-format \
    && ln -sf /usr/bin/clang-tidy-15 /usr/local/bin/clang-tidy \
    && (ln -sf /usr/bin/FileCheck-15 /usr/local/bin/FileCheck || true)

RUN useradd -m -s /bin/bash ubuntu \
    && echo 'ubuntu ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/ubuntu \
    && chmod 0440 /etc/sudoers.d/ubuntu

USER ubuntu
WORKDIR /home/ubuntu
