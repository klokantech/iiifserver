#!/bin/sh
docker run -v /imageserver-data:/imageserver-data -t -i -p 80:80 iipsrv /init.sh
