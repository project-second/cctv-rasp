#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
gsoap_import_dir="${GSOAP_IMPORT_DIR:-/usr/share/gsoap/import}"
gsoap_share_dir="${GSOAP_SHARE_DIR:-/usr/share/gsoap}"

required_gsoap_version="2.8.135"
actual_soapcpp2_version="$(soapcpp2 -v 2>&1 | sed -n 's/.*soapcpp2 release \([0-9.]*\).*/\1/p' | head -n1)"
if [[ "${actual_soapcpp2_version}" != "${required_gsoap_version}" ]]; then
  echo "error: soapcpp2 ${required_gsoap_version} is required; found ${actual_soapcpp2_version:-unknown}" >&2
  echo "Run this generator on the Raspberry Pi, then copy gsoap/generated back to this tree." >&2
  exit 1
fi

soapcpp2 \
  -2 \
  -c++11 \
  -x \
  -I"${gsoap_import_dir}" \
  -I"${gsoap_share_dir}" \
  -d "${script_dir}/generated" \
  "${script_dir}/onvif.h"

nsmap="${script_dir}/generated/DeviceBinding.nsmap"
grep -q '"tt", "http://www.onvif.org/ver10/schema"' "$nsmap" ||
  sed -i '/{ NULL, NULL, NULL, NULL}/i\        { "tt", "http://www.onvif.org/ver10/schema", NULL, NULL },' "$nsmap"
grep -q '"dn", "http://www.onvif.org/ver10/network/wsdl"' "$nsmap" ||
  sed -i '/{ NULL, NULL, NULL, NULL}/i\        { "dn", "http://www.onvif.org/ver10/network/wsdl", NULL, NULL },' "$nsmap"
