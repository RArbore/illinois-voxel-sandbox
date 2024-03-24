#!/usr/bin/env bash
set -exo pipefail

mkdir -p obj
wget -nc -O obj/sponza.zip "https://casual-effects.com/g3d/data10/research/model/dabrovic_sponza/sponza.zip" &
wget -nc -O obj/hairball.zip "https://casual-effects.com/g3d/data10/research/model/hairball/hairball.zip" &
wget -nc -O obj/san_miguel.zip "https://casual-effects.com/g3d/data10/research/model/San_Miguel/San_Miguel.zip" &
wget -nc -O obj/buddha.zip "https://casual-effects.com/g3d/data10/research/model/buddha/buddha.zip" &
wait

rm -rf obj/sponza/
rm -rf obj/hairball/
rm -rf obj/san_miguel/
rm -rf obj/buddha/
unzip obj/sponza.zip -d obj/sponza/ &
unzip obj/hairball.zip -d obj/hairball/ &
unzip obj/san_miguel.zip -d obj/san_miguel/ &
unzip obj/buddha.zip -d obj/buddha/ &
wait

ls obj/sponza/*.mtl | while read m; do tr '\\' '/' <$m >tmpfile && mv tmpfile $m; done
ls obj/san_miguel/*.mtl | while read m; do tr '\\' '/' <$m >tmpfile && mv tmpfile $m; done
