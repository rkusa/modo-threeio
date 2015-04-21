# threeio

A [three.js](https://github.com/mrdoob/three.js) JSON (format 4) exporter for Modo 801.

**Features:**

- Geometry, Normals, UVs
- Basic Materials
- Indexed BufferGeometry
- Streaming fast export

If you get an error that the export failed. Your scene propably contains ngons, or if you export to BufferGeometry, your scene propably contains quads and/or ngons.

![Settings](https://dl.dropboxusercontent.com/u/6699613/Github/modo-threeio-settings.png)

## Status

I've developed this plugin during a Modo trial period, but did not bought a license in the end. That is, I am propably not able to fix bugs that require testing in Modo.

## Build

Before you're able to build from source, you need to [download the Modo SDK](http://modo.sdk.thefoundry.co.uk/wiki/Tour_of_the_SDK). The SDK directories `common` and `include` have to be placed one level above `threeio`:

```bash
% ls path/to/dev
common include threeio
```

Simply use the contained XCode project, [create a Visual Studio project](http://sdk.luxology.com/wiki/Building_Plug-ins) or compile by calling:

```bash
% make
```

### Install

#### Mac OS

```bash
% make install
```

`make install` links the `threeio` kit into `/Library/Application Support/Luxology/Content/Kits`.


