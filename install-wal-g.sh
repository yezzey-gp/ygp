#!/bin/bash
set -ex

# Install latest Go compiler
sudo add-apt-repository ppa:longsleep/golang-backports 
sudo apt update
sudo apt install golang-go

# Install lib dependencies
sudo apt install libbrotli-dev liblzo2-dev libsodium-dev curl cmake

# Fetch project and build
go get github.com/wal-g/wal-g
cd ~/go/src/github.com/wal-g/wal-g
make deps
make pg_build
main/pg/wal-g --version
