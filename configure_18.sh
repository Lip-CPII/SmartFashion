#!/bin/bash
wget https://ext.bravedbrothers.com/OpenMesh_ubuntu18.tar.gz
tar -zxvf OpenMesh_ubuntu18.tar.gz OpenMesh
rm OpenMesh_ubuntu18.tar.gz

wget https://ext.bravedbrothers.com/OpenCV_ubuntu18.tar.gz
tar -zxvf OpenCV_ubuntu18.tar.gz OpenCV
rm OpenCV_ubuntu18.tar.gz

sudo apt-get update
sudo apt-get install build-essential mesa-common-dev libgles2-mesa-dev -y
