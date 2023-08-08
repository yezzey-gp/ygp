#!/bin/bash
set -ex

sudo bash -c 'cat >> /etc/ld.so.conf <<-EOF
/usr/local/lib

EOF'
sudo ldconfig

git submodule update --init

./configure --prefix=/usr/local/gpdb/ --with-openssl --enable-debug-extensions --enable-gpperfmon --with-python --with-libxml CFLAGS='-fno-omit-frame-pointer -Wno-implicit-fallthrough -O3 -pthread'
make -j && make -j install

sudo bash -c 'cat >> /etc/sysctl.conf <<-EOF
kernel.shmmax = 500000000
kernel.shmmni = 4096
kernel.shmall = 4000000000
kernel.sem = 500 1024000 200 4096
kernel.sysrq = 1
kernel.core_uses_pid = 1
kernel.msgmnb = 65536
kernel.msgmax = 65536
kernel.msgmni = 2048
net.ipv4.tcp_syncookies = 1
net.ipv4.ip_forward = 0
net.ipv4.conf.default.accept_source_route = 0
net.ipv4.tcp_tw_recycle = 1
net.ipv4.tcp_max_syn_backlog = 4096
net.ipv4.conf.all.arp_filter = 1
net.ipv4.ip_local_port_range = 1025 65535
net.core.netdev_max_backlog = 10000
net.core.rmem_max = 2097152
net.core.wmem_max = 2097152
vm.overcommit_memory = 2

EOF'

sudo bash -c 'cat >> /etc/security/limits.conf <<-EOF
* soft nofile 65536
* hard nofile 65536
* soft nproc 131072
* hard nproc 131072

EOF'

ssh-keygen -f ~/.ssh/id_rsa -N ''
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_rsa

export GPHOME=/usr/local/gpdb
source $GPHOME/greenplum_path.sh
ulimit -n 65536
make create-demo-cluster
source gpAux/gpdemo/gpdemo-env.sh

