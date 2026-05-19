#!/bin/bash
CONTAINER_NAME="platformio_run"

if [ -z "$GITHUB_TOKEN" ]; then
  echo "GITHUB_TOKEN is not set. Please set it to continue."
  exit 1
fi

echo "$GITHUB_TOKEN" | docker login ghcr.io -u "goldenm-software" --password-stdin

if docker ps -a | grep $CONTAINER_NAME
then
  echo "Container $CONTAINER_NAME already exists. Removing..."
  docker rm --force $CONTAINER_NAME
fi

echo "Running container $CONTAINER_NAME..."
docker run -d -it --name $CONTAINER_NAME \
  --env-file ./.env \
  ghcr.io/goldenm-software/platformio-builder:uv-3.13

echo "Copying source code into container..."
docker cp ./ $CONTAINER_NAME:/app

echo "Entering container $CONTAINER_NAME..."
docker exec -it $CONTAINER_NAME /bin/bash
