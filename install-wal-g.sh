#!/bin/bash
set -ex

# Install latest Go compiler
sudo add-apt-repository ppa:longsleep/golang-backports 
sudo apt update
sudo apt install -y golang-go

# Install lib dependencies
sudo apt install -y libbrotli-dev liblzo2-dev libsodium-dev curl cmake

# Fetch project and build
cd root
git clone https://github.com/wal-g/wal-g.git
cd wal-g
make deps
make pg_build

#Check the installation
main/pg/wal-g --version
