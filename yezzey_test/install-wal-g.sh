#!/bin/bash
set -ex

wget -c https://github.com/wal-g/wal-g/releases/download/v2.0.1/wal-g-gp-ubuntu-20.04-amd64.tar.gz
tar zxvf wal-g-gp-ubuntu-20.04-amd64.tar.gz
mv wal-g-gp-ubuntu-20.04-amd64 /usr/bin/wal-g

#Check the installation
wal-g --version
