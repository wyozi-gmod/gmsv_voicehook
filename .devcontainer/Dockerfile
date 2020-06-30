FROM alpine:latest as base
RUN apk add --update --no-cache \
        tar \
        wget \
 && mkdir -p /app \
 && wget --no-verbose 'https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz' -O '/tmp/steamcmd_linux.tar.gz' \
 && tar -C /app -xf '/tmp/steamcmd_linux.tar.gz' \
 && ln -s steamcmd.sh /app/steamcmd

FROM ubuntu:18.04
COPY --from=base /app /opt/steamcmd

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends --no-install-suggests \
        ca-certificates \
        language-pack-en \
        lib32tinfo5 \
 && apt upgrade -y \
 && apt clean \
 && rm -rf \
        /var/lib/apt/lists/*

# Configure apt and install packages
RUN apt-get update \
    #
    # Install C++ tools
    && apt-get -y install build-essential cmake cppcheck valgrind curl gcc-multilib g++-multilib

RUN curl -L https://github.com/premake/premake-core/releases/download/v5.0.0-alpha15/premake-5.0.0-alpha15-linux.tar.gz > premake.tar.gz \
       && tar -xzvf premake.tar.gz \
       && chmod +x premake5 \
       && cp premake5 /usr/bin/

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=dialog
WORKDIR /opt/steamcmd
