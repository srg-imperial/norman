## stage 1: setup base image
ARG REGISTRY=docker.io
FROM ${REGISTRY}/library/ubuntu:23.10 AS base
ARG LLVM=15
ARG JOBS=$(nproc)

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

ENV \
  PATH="/.container/bin:/usr/lib/llvm-${LLVM}/bin/:$PATH" \
  CTEST_OUTPUT_ON_FAILURE=1


## stage 2b: sources
FROM base AS sources

COPY ./ /norman/
WORKDIR /norman


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
    ../ \
  && sh -c "exec ninja -j${JOBS}"
