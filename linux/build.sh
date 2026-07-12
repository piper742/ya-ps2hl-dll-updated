#!/usr/bin/env bash

make CFG=debug -j2
cp debug/hl.so* ~/Games/steamapps/common/Half-Life/ps2hlu/dlls/
cp debug/client.so* ~/Games/steamapps/common/Half-Life/ps2hlu/cl_dlls/
