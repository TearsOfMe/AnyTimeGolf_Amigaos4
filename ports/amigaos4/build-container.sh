#!/bin/sh
set -eu

CONTAINER=${AMIGAOS4_CONTAINER:-4530b990e258eda651876f79799ec39b9b993394c8ae72f72a619b9b63ebd2de}
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)

docker cp "$ROOT/." "$CONTAINER:/opt/code/golf"
docker exec "$CONTAINER" sh /opt/code/golf/ports/amigaos4/build-in-container.sh
docker cp "$CONTAINER:/opt/code/golf/build-amigaos4/." "$ROOT/build-amigaos4"
