#!/bin/bash
wget https://ext.bravedbrothers.com/OpenMesh_ubuntu.tar.gz
tar -zxvf OpenMesh_ubuntu.tar.gz OpenMesh
rm OpenMesh_ubuntu.tar.gz

wget https://ext.bravedbrothers.com/OpenCV_ubuntu.tar.gz
tar -zxvf OpenCV_ubuntu.tar.gz OpenCV
rm OpenCV_ubuntu.tar.gz

sudo apt-get update
sudo apt-get install build-essential mesa-common-dev libgles2-mesa-dev -y
