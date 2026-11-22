#!/usr/bin/env bash
# Shared pins. Every number in the article must be reproducible from these.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DAV1D_TAG=1.5.3
LIBAVIF_TAG=v1.4.2
CRABBY_REV=1e40c7de3cafc20737f5dcdd954239fc8d284e5b
TP="$ROOT/third_party"; OUT="$ROOT/build"
