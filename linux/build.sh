#!/usr/bin/env bash

make CFG=debug -j2
rm -f ~/Games/steamapps/common/Half-Life/ps2hlu/dlls/hl.so
rm -f ~/Games/steamapps/common/Half-Life/ps2hlu/cl_dlls/client.so
cp debug/hl.so* ~/Games/steamapps/common/Half-Life/ps2hlu/dlls/
cp debug/client.so* ~/Games/steamapps/common/Half-Life/ps2hlu/cl_dlls/
