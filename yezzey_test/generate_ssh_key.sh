#!/bin/bash
set -ex

ssh-keygen -f ~/.ssh/id_rsa -N ''
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_rsa
sudo service ssh start
ssh -o StrictHostKeyChecking=no krebs@$(hostname) "echo 'Hello world'"
