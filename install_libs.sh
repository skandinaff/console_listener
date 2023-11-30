#!/bin/bash

# Check for root privileges
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root or with sudo."
  exit 1
fi

# Install required packages
apt_packages=("libreadline8" "libstdc++6" "libgcc1" "libc6" "libtinfo6" "libserialport0" "libreadline-dev" "libpthread-stubs0-dev" "libboost-all-dev")

for pkg in "${apt_packages[@]}"; do
  if ! dpkg -s "$pkg" >/dev/null 2>&1; then
    apt-get install -y "$pkg"
  else
    echo "$pkg is already installed."
  fi
done

echo "Dependency installation complete."
