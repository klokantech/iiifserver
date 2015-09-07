#!/bin/sh
set -ex

# install binary
sudo make install

#prepare demo image file
sudo mkdir /data && sudo wget --no-verbose --output-document=/data/67352ccc-d1b0-11e1-89ae-279075081939.png http://iiif.io/api/image/validator/67352ccc-d1b0-11e1-89ae-279075081939.png
sudo convert /data/67352ccc-d1b0-11e1-89ae-279075081939.png -define tiff:tile-geometry=256x256  'ptif:/data/67352ccc-d1b0-11e1-89ae-279075081939.tif'

#installation of necessary packages
sudo apt-get update -qq
sudo apt-get install -y nginx libmagic-dev
sudo pip install Pillow iiif_validator

#nginx configuration
sudo service nginx stop

NGINX_CONF="/etc/nginx/sites-enabled/default"

echo "
server {

  listen 80;
  server_name iiifserver;
  location ~iiifserver.fcgi$ {

    include fastcgi_params;
    fastcgi_pass 127.0.0.1:9000;
    add_header 'Access-Control-Allow-Origin' "*";
  }

  # Beautiful URLs:

  # /{identifier}/info.json
  rewrite ^/(.+)/info.json$ /iiifserver.fcgi?iiif=\$1.tif/info.json break;

  # /{identifier}/{region}/{size}/{rotation}/{quality}.{format}
  rewrite ^/(.+)/([^/]+)/([^/]+)/([^/]+)/([^/]+)$ /iiifserver.fcgi?iiif=\$1.tif/\$2/\$3/\$4/\$5 break;

}
" | sudo tee $NGINX_CONF > /dev/null

#IIIF server configuration
export FILESYSTEM_PREFIX=/data/
export MEMCACHED_SERVERS=localhost
export MEMCACHED_TIMEOUT=0

#services start
sudo service nginx start
sudo service memcached start
src/iipsrv.fcgi --bind 127.0.0.1:9000 &

#validation
iiif-validate.py -s 127.0.0.1:80 -i 67352ccc-d1b0-11e1-89ae-279075081939 --version=2.0 -v  --test quality_color --test id_error_escapedslash --test rot_region_basic --test jsonld --test region_error_random --test id_error_unescaped --test region_percent --test size_region --test size_error_random --test size_ch --test size_wc --test quality_grey --test id_squares --test region_pixels --test id_escaped --test format_error_random --test info_json --test size_percent --test cors --test id_error_random --test quality_error_random --test size_bwh --test format_jpg --test size_wh --test id_basic --test rot_error_random
