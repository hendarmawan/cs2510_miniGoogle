#!/bin/sh

i=0
ls -l webdata | awk '{print $9}'| while read line; do
    curl -d "@./webdata/$line" "http://127.0.0.1:8000/put" &
    i=$((i+1))
    if [ $((i % 20)) -eq 0 ]; then
        wait
    fi
done

exit 0
