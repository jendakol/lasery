#!/bin/bash

dir=$(pwd)

cd "$dir" && \
  cd front
  npm i && \
  npm run build && \
  cd dist && \

  cd js && \
  rm *.map && \
  cd .. && \

  cd fonts && \
  for f in *.woff; do mv "$f" "$(echo "$f" | sed 's/roboto-latin/ro/')"; done && \
  cd .. && \
  mv fonts f && \
  sed -i -E 's/..\/fonts\/roboto-latin/\/f\/ro/g' css/*.css && \

  gzip -9 css/* && \
  gzip -9 js/* && \
  rm **/*LICENSE* && \

  du -b * && \
  cd "$dir" && \
  mkdir data || true && \
  cd data && rm -rf * && ln -sf ../front/dist w && \
  cd "$dir" && \
  read -p "Press any key to upload (or kill me)..." && \
  pio run --target uploadfs
