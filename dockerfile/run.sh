#!/bin/sh
docker run -v /imageserver-data:/imageserver-data -t -i -p 80:80 -p 14123:22 iipsrv /init.sh
