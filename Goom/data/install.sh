#!/bin/bash

echo "Install Game.001"

cd "`dirname $0`"
. config.sh $1

# Copy to final directory
echo "Final copy $BUILD_DIR to $FINAL_DIR"
rsync --modify-window=10 -C -c -rv \
    --exclude '.*' \
    --delete-excluded \
    --delete \
    "$BUILD_DIR" "$FINAL_DIR/"

