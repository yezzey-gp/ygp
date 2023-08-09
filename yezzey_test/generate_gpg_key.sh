#!/bin/bash
set -ex

gpg --batch --gen-key <<EOF
Key-Type: 1
Key-Length: 2048
Subkey-Type: 1
Subkey-Length: 2048
Name-Real: Root Superuser
Name-Email: root@handbook.westarete.com
Expire-Date: 0
Passphrase: ''
EOF
