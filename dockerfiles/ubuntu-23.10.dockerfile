## stage 1: setup base image
ARG REGISTRY=docker.io
FROM ${REGISTRY}/library/ubuntu:23.10 AS base
ARG LLVM=15
ARG JOBS=$(nproc)

# build-essential: compilers for the main project
# cmake: configure the main project
# curl: klee-uclibc
# file: cpack
# flex: stp
# git: used when bootstrapping coreutils
# gperf: used when bootstrapping coreutils
# libboost-all-dev: stp
# libfmt-dev: direct dependency of the product program project
# libgoogle-perftools-dev: klee
# libgtest-dev: klee
# libsqlite3-dev: klee
# libz3-dev: klee
# ninja-build: build the main project (faster than with `make`)
# perl: stp
# pipx: install `lit` & `wllvm`
# python-is-python3: stp, klee-uclibc
# python3: utility scripts (`/run-constructor.py`)
# python3-tabulate: klee
# rsync: used when bootstrapping coreutils
# software-properties-common: provides `add-apt-repositories`
# wget: used when bootstrapping coreutils
# zlib1g-dev: minisat, stp, klee
RUN \
  --mount=type=cache,sharing=locked,target=/var/cache/apt/,id=ubuntu:23.10/var/cache/apt/ \
  --mount=type=cache,sharing=locked,target=/var/lib/apt/lists/,id=ubuntu:23.10/var/lib/apt/lists/ \
  mv /etc/apt/apt.conf.d/docker-clean /etc/apt/apt.conf.d/docker-gzip-indexes / \
  && apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends --no-install-suggests \
    build-essential \
    ca-certificates \
    clang-${LLVM} \
    clang-format-${LLVM} \
    cmake \
    git \
    libclang-${LLVM}-dev \
    llvm-${LLVM} \
    llvm-${LLVM}-dev \
    llvm-${LLVM}-tools \
    mold \
    ninja-build \
    rapidjson-dev \
  && mv /docker-clean /docker-gzip-indexes /etc/apt/apt.conf.d/

ENV LLVM_COMPILER=clang LLVM_COMPILER_PATH=/usr/lib/llvm-${LLVM}/bin/


## stage 2a: devcontainer
FROM base AS devcontainer
RUN \
  --mount=type=cache,sharing=locked,target=/var/cache/apt/,id=ubuntu:23.10/var/cache/apt/ \
  --mount=type=cache,sharing=locked,target=/var/lib/apt/lists/,id=ubuntu:23.10/var/lib/apt/lists/ \
  mv /etc/apt/apt.conf.d/docker-clean /etc/apt/apt.conf.d/docker-gzip-indexes / \
  && apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends --no-install-suggests \
    bash-completion \
    docker.io \
    gdb \
    less \
    ssh \
    sudo \
    vim \
    zsh \
    zstd \
  && mv /docker-clean /docker-gzip-indexes /etc/apt/apt.conf.d/

ARG USERNAME=user
ARG GROUPNAME=${USERNAME}
ARG UID=1000
ARG GID=${UID}

RUN \
  userdel ubuntu \
	&& groupadd -g ${GID} ${GROUPNAME} \
	&& useradd -m --uid ${UID} --gid ${GID} -G sudo,docker -s "$(which bash)" ${USERNAME} \
	&& echo %sudo ALL=\(ALL:ALL\) NOPASSWD:ALL > /etc/sudoers.d/nopasswd \
  && chmod 0440 /etc/sudoers.d/nopasswd

RUN \
  mkdir -p /.container/bin \
  && printf "#!/bin/bash\\nexec sudo '$(which docker)' "'"$@"'"\\n" > /.container/bin/docker

ENV PATH="/.container/bin:/usr/lib/llvm-12/bin/:$PATH"


## stage 2b: sources
FROM base AS sources

COPY ./ /normalize-transform/
WORKDIR /normalize-transform


## stage 3: prebuilt
FROM sources AS prebuilt
ARG LLVM=12
ARG JOBS=$(nproc)

RUN \
  mkdir build \
  && cd build \
  && cmake -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS='-fuse-ld=mold' \
    -DCMAKE_C_FLAGS='-Werror' \
    -DCMAKE_CXX_FLAGS='-Werror' \
    -DLLVM_DIR=$(llvm-config-${LLVM} --libdir) \
    -DCOREUTILS_SOURCE_DIR=/coreutils \
    ../ \
  && sh -c "exec ninja -j${JOBS}"