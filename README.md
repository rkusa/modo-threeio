# threeio

A [three.js](https://github.com/mrdoob/three.js) JSON 4 format exporter for Modo 801.

**Features:**

- Geometry, Normals, UVs
- Basic Materials
- Indexed BufferGeometry
- Streaming fast export

If you get an error that the export failed. Your scene propably contains ngons, or if you export to BufferGeometry, your scene propably contains quads and/or ngons.

!(Settings)[https://dl.dropboxusercontent.com/u/6699613/Github/modo-threeio-settings.png]

## Call to arms!

I've just started implementing this exporter. That is, consider to run into issues. You are welcome to open an issue or PR for what ever concern (issue, missing feature, suggestion, ...) you have.

## Build

Before you're able to build from source, you need to [download the Modo SDK](http://modo.sdk.thefoundry.co.uk/wiki/Tour_of_the_SDK). The SDK directories `common` and `include` have to be place one level above `threeio`:

```bash
% ls path/to/dev
common include threeio
% make
% make install
```

`make install` links the `threeio` kit into `/Library/Application Support/Luxology/Content/Kits`.

## Install prebuild

Maybe you find prebuild binaries [here](https://github.com/rkusa/modo-threeio/releases). If so, install them into `/Library/Application Support/Luxology/Content/Kits`. If not, feel free to open an issue offering your help for prebuilding binaries or just build it from source for your own.
