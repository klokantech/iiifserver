#!/bin/sh
set -ex

# install binary
sudo make install

#prepare demo image file
sudo mkdir /data && sudo wget --no-verbose --output-document=/data/demo.jp2 http://help.oldmapsonline.org/jpeg2000/demo.jp2?attredirects=0

#nginx configuration
sudo apt-get update -qq
sudo apt-get install -y nginx

sudo service nginx stop

NGINX_CONF="/etc/nginx/sites-enabled/default"

echo "
server {

  listen 80;
  server_name iiifserver;
  location ~iiifserver.fcgi$ {

    include fastcgi_params;
    fastcgi_pass localhost:9000;
    add_header 'Access-Control-Allow-Origin' "*";
  }

  # Beautiful URLs:

  # /{identifier}/info.json
  rewrite ^/(.+)/info.json$ /iiifserver.fcgi?iiif=\$1.jp2/info.json break;

  # /{identifier}/{region}/{size}/{rotation}/{quality}.{format}
  rewrite ^/(.+)/([^/]+)/([^/]+)/([^/]+)/([^/]+)$ /iiifserver.fcgi?iiif=\$1.jp2/\$2/\$3/\$4/\$5 break;

}
" | sudo tee $NGINX_CONF > /dev/null

#IIIF server configuration
export FILESYSTEM_PREFIX=/data/
export MEMCACHED_SERVERS=localhost
export MEMCACHED_TIMEOUT=0

#services start
sudo service nginx start
sudo service memcached start
src/iipsrv.fcgi --bind localhost:9000 &

#output test
if  [ "200" -eq $(curl -s -o /dev/null -I -w "%{http_code}" http://localhost/demo/info.json) ]; then
    printf '%s\n' 'Image check was successful!' >&2
    exit 0
fi

printf '%s\n' 'Image check was unsuccessful!' >&2
exit 1

