#!/bin/bash

sudo apt update
sudo apt install -y cmake git gcc-arm-none-eabi gcc g++ build-essential

git clone --recursive https://github.com/daveythacher/LED_Matrix_RP2040.git
