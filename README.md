# Caldera ZMK Config

ZMK firmware configuration for the Caldera keyboard.

## Building locally with Docker

### Prerequisites

- [Docker](https://docs.docker.com/get-docker/) installed and running

### First-time setup

Pull the ZMK build image and initialize a persistent west workspace volume:

```bash
docker pull zmkfirmware/zmk-build-arm:stable

docker run --rm \
  -v zmk-west-cache:/workspace \
  zmkfirmware/zmk-build-arm:stable \
  bash -c "cd /workspace && west init -m https://github.com/zmkfirmware/zmk --mr v0.3 && west update"
```

This downloads Zephyr and its modules (~500 MB) into a named Docker volume. Only needed once.

### Build

```bash
# Left half
docker run --rm \
  -v zmk-west-cache:/workspace \
  -v $(pwd):/zmk-module \
  -w /workspace \
  -e ZEPHYR_BASE=/workspace/zephyr \
  zmkfirmware/zmk-build-arm:stable \
  west build -s zmk/app -b nice_nano_v2 -d /tmp/build-left -- \
    -DCMAKE_PREFIX_PATH=/workspace/zephyr/share/zephyr-package/cmake \
    -DZMK_CONFIG=/zmk-module/config \
    -DZMK_EXTRA_MODULES=/zmk-module \
    -DSHIELD=caldera_left

# Right half
docker run --rm \
  -v zmk-west-cache:/workspace \
  -v $(pwd):/zmk-module \
  -w /workspace \
  -e ZEPHYR_BASE=/workspace/zephyr \
  zmkfirmware/zmk-build-arm:stable \
  west build -s zmk/app -b nice_nano_v2 -d /tmp/build-right -- \
    -DCMAKE_PREFIX_PATH=/workspace/zephyr/share/zephyr-package/cmake \
    -DZMK_CONFIG=/zmk-module/config \
    -DZMK_EXTRA_MODULES=/zmk-module \
    -DSHIELD=caldera_right
```

Build output (`.uf2` files) lands in `/tmp/build-left/zephyr/` and `/tmp/build-right/zephyr/` inside the container. To copy them out, replace `-d /tmp/build-left` with `-d /tmp/build-left -v $(pwd)/build:/tmp/build-left`.

## GitHub Actions

Firmware is also built automatically on every push via `.github/workflows/build.yml`.
