# SmartFashion


## Install

Support in **Ubuntu 20** or newer and Microsoft **Windows 10**.

### Dependencies
- Qt 5.15 ([Here](https://www.qt.io/download-open-source), scroll to bottom and download)

**_It is recommanded to use QtCreator as the IDE._**

- OpenCV 4.5 or newer (Prebuilt [Ubuntu 20](https://ext.bravedbrothers.com/OpenCV_ubuntu.tar.gz) / [Windows10])
- OpenMesh 9 or newer (Prebuilt [Ubuntu 20](https://ext.bravedbrothers.com/OpenMesh.ubuntu.tar.gz) / [Windows10](https://ext.bravedbrothers.com/OpenMesh.7z))

#### Ubuntu

It is suggested to installed the ffmpeg package for video-IO.

```console
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libx264-dev libavresample-dev -Y
```

Make sure the folders hierachy is similar as following:
```console
/path/to/repository/
SmartFashion
   |- App
   |- Functional
   |- Model
   |- LP_Plugin_Import
OpenMesh
   |- include
   |- lib
   |- share
OpenCV
   |-install
        |- include
        |- bin
        |- lib
        |- share
   
```
