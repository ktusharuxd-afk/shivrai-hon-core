#!/bin/bash
set -e
cd ~/shivrai-hon-core
T=${1:?usage: tools/run.sh <test_name> "<commit msg>"}
MSG=${2:?usage: tools/run.sh <test_name> "<commit msg>"}
tools/test.sh "$T"
git add -A
git commit -m "$MSG"
git push origin master
