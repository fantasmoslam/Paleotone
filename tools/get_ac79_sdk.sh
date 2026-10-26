#!/bin/sh
# Fetch the pinned vendor SDK. A failed DNS/network attempt must not poison retries.
set -eu
destination=${1:?usage: get_ac79_sdk.sh DESTINATION}
if [ -e "$destination" ]; then
  echo "SDK destination already exists: $destination" >&2
  exit 1
fi
attempt=1
while [ "$attempt" -le 3 ]; do
  staging="${destination}.attempt-${attempt}"
  if git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git "$staging"; then
    test -f "$staging/cpu/wl82/tools/uboot.boot"
    mv "$staging" "$destination"
    exit 0
  fi
  echo "SDK download attempt $attempt failed" >&2
  attempt=$((attempt + 1))
  if [ "$attempt" -le 3 ]; then sleep 10; fi
done
echo "SDK unavailable after three attempts; no firmware was built" >&2
exit 1
