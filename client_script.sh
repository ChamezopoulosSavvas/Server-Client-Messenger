#!/bin/bash
set -x

./client.out 192.168.1.1 2222

echo b | client.out

echo $(((RANDOM % 10) + 1))

