#!/bin/sh

set -x

ls -l webdata | awk '{print $9}'| while read line; do
#curl -d "@./webdata/$line" "http://127.0.0.1:8000/put" >/dev/null 2>&1 &
    curl -d "@./webdata/$line" "http://127.0.0.1:8000/put"
done

exit 0
