#!/bin/bash

apt update
apt install -y \
  bison \
  ccache \
  cmake \
  curl \
  flex \
  git-core \
  inetutils-ping \
  krb5-kdc \
  krb5-admin-server \
  libapr1-dev \
  libbz2-dev \
  libcurl4-gnutls-dev \
  libevent-dev \
  libkrb5-dev \
  libpam-dev \
  libperl-dev \
  libreadline-dev \
  libssl-dev \
  libxml2-dev \
  libyaml-dev \
  libzstd-dev \
  locales \
  net-tools \
  ninja-build \
  openssh-client \
  openssh-server \
  openssl \
  python2-dev \
  python3-pip \
  zlib1g-dev \
  libpstreams-dev


pip install conan

