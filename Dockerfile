FROM ubuntu:focal

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN useradd -rm -d /home/krebs -s /bin/bash -g root -G sudo -u 1001 krebs

RUN ln -snf /usr/share/zoneinfo/Europe/London /etc/localtime && echo Europe/London > /etc/timezone \
&& sed -i "s/archive.ubuntu.com/mirror.yandex.ru/g" /etc/apt/sources.list \
&& apt-get update -o Acquire::AllowInsecureRepositories=true && apt-get install -y --no-install-recommends --allow-unauthenticated \
  build-essential libssl-dev gnupg devscripts \
  openssl libssl-dev debhelper debootstrap \
  make equivs bison ca-certificates-java ca-certificates \
  cmake curl cgroup-tools flex gcc-8 g++-8 g++-8-multilib \
  git krb5-multidev libapr1-dev libbz2-dev libcurl4-gnutls-dev \
  libevent-dev libkrb5-dev libldap2-dev libperl-dev libreadline6-dev \
  libssl-dev libxml2-dev libyaml-dev libzstd-dev libaprutil1-dev \
  libpam0g-dev libpam0g libcgroup1 libyaml-0-2 libldap-2.4-2 libssl1.1 \
  ninja-build python-dev python-setuptools quilt unzip wget zlib1g-dev libuv1-dev \
  libgpgme-dev libgpgme11 sudo iproute2 less

RUN apt-get install -y locales \
&& locale-gen "en_US.UTF-8" \
&& update-locale LC_ALL="en_US.UTF-8"

RUN echo 'krebs ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers

USER krebs
WORKDIR /home/krebs

RUN cd /tmp/ \
&& git clone https://github.com/greenplum-db/gp-xerces.git \
&& cd ./gp-xerces/ && mkdir build && cd build && ../configure --prefix=/usr/local && make -j \
&& sudo make install

RUN cd /tmp/ \
&& git clone https://github.com/boundary/sigar.git \
&& cd ./sigar/ \
&& mkdir build && cd build && cmake .. && make \
&& sudo make install

COPY . /home/krebs

RUN sudo DEBIAN_FRONTEND=noninteractive ./README.ubuntu.bash \
&& sudo apt install -y libhyperic-sigar-java libaprutil1-dev libuv1-dev

RUN sudo mkdir /usr/local/gpdb \
&& sudo chown krebs:root /usr/local/gpdb


