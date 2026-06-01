#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

soapcpp2 \
  -2 \
  -c++11 \
  -x \
  -I/usr/share/gsoap/import \
  -I/usr/share/gsoap \
  -d "${script_dir}/generated" \
  "${script_dir}/onvif.h"

nsmap="${script_dir}/generated/DeviceBinding.nsmap"
grep -q '"tt", "http://www.onvif.org/ver10/schema"' "$nsmap" ||
  sed -i '/{ NULL, NULL, NULL, NULL}/i\        { "tt", "http://www.onvif.org/ver10/schema", NULL, NULL },' "$nsmap"
grep -q '"dn", "http://www.onvif.org/ver10/network/wsdl"' "$nsmap" ||
  sed -i '/{ NULL, NULL, NULL, NULL}/i\        { "dn", "http://www.onvif.org/ver10/network/wsdl", NULL, NULL },' "$nsmap"
