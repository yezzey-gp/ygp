#!/bin/bash
set -ex

eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_rsa
sudo service ssh start
ssh -o StrictHostKeyChecking=no krebs@$(hostname) "echo 'Hello world'"

sudo bash -c 'cat >> /etc/ld.so.conf <<-EOF
/usr/local/lib

EOF'
sudo ldconfig

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

export GPHOME=/usr/local/gpdb
source $GPHOME/greenplum_path.sh
ulimit -n 65536
make destroy-demo-cluster && make create-demo-cluster
export USER=krebs
source gpAux/gpdemo/gpdemo-env.sh

gpconfig -c yezzey.storage_prefix -v "'yezzey-test-files'"
gpconfig -c yezzey.storage_bucket -v "'${S3_BUCKET}'"
gpconfig -c yezzey.storage_config -v "'/home/krebs/yezzey_test/yezzey-s3.conf'"
gpconfig -c yezzey.storage_host -v "'https://storage.yandexcloud.net'"
gpconfig -c yezzey.gpg_key_id -v  "'$(gpg --list-keys | head -n 4 | tail -n 1)'"
gpconfig -c yezzey.walg_bin_path -v  "'wal-g'"
gpconfig -c yezzey.walg_config_path -v  "'/home/krebs/yezzey_test/wal-g-conf.yaml'"

gpconfig -c max_worker_processes -v 10

gpconfig -c yezzey.autooffload -v  "on"

gpconfig -c shared_preload_libraries -v yezzey

gpstop -a -i && gpstart -a


createdb $USER

# Run tests
psql postgres -f ./gpcontrib/yezzey/test/regress/expirity.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/metadata.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/simple_alter.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/simplebig.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/simplelol.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/simple.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/simple_vac.sql
# psql postgres -f ./gpcontrib/yezzey/test/regress/vacuum-yezzey.sql  # fails
psql postgres -f ./gpcontrib/yezzey/test/regress/yao.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/yezzey-alter.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/yezzey-exp.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/yezzey-large.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/yezzey-reorg.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/yezzey.sql
psql postgres -f ./gpcontrib/yezzey/test/regress/yezzey-trunc.sql
# psql postgres -f ./gpcontrib/yezzey/test/regress/yezzey-vindex.sql  # broken
psql postgres -f ./gpcontrib/yezzey/test/regress/yezzey_wal.sql
