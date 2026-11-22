#!/usr/bin/env bash
source "$(dirname "$0")/common.sh"; mkdir -p "$TP"; cd "$TP"
[ -d dav1d ]    || git clone --depth 1 --branch "$DAV1D_TAG"   https://code.videolan.org/videolan/dav1d.git
[ -d libavif ]  || git clone --depth 1 --branch "$LIBAVIF_TAG" https://github.com/AOMediaCodec/libavif.git
if [ ! -d CrabbyAvif ]; then
  git clone https://github.com/webmproject/CrabbyAvif.git
  git -C CrabbyAvif checkout "$CRABBY_REV"
fi
echo "fetched: dav1d $DAV1D_TAG, libavif $LIBAVIF_TAG, CrabbyAvif ${CRABBY_REV:0:8}"

# Real-photo corpus: third-party AVIFs (Netflix, Microsoft, Link-U, Apple) from the AV1-AVIF conformance repo.
if [ ! -d "$TP/photos" ]; then
  git clone --depth 1 --filter=blob:none --no-checkout https://github.com/AOMediaCodec/av1-avif.git "$TP/av1avif"
  ( cd "$TP/av1avif" && git sparse-checkout init --no-cone && git sparse-checkout set \
      'testFiles/Netflix/avif/*.avif' 'testFiles/Microsoft/*.avif' 'testFiles/Link-U/*.avif' \
      'testFiles/Apple/multilayer_examples/animals_00_singlelayer.avif' && git checkout )
  mkdir -p "$TP/photos"
  find "$TP/av1avif/testFiles" -name '*.avif' -size +100k | while read -r f; do
    cp "$f" "$TP/photos/$(echo "${f#*testFiles/}" | tr '/' '_')"; done
fi
echo "photo corpus: $(ls "$TP/photos" | wc -l) files"
