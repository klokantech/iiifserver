FROM debian

MAINTAINER Martin Rumanek <martin@rumanek.cz>

RUN apt-get -y  update
RUN apt-get -y install wget apache2 libfcgi0ldbl libjpeg62 libmemcached10 libtiff4 libapache2-mod-fcgid

RUN wget --no-verbose http://dl.dropbox.com/s/7k24kkvqf1pafec/iipimage-jp2-amd64.deb -O /tmp/iipimage-jp2-amd64.deb
RUN dpkg -i /tmp/iipimage-jp2-amd64.deb 

RUN a2enmod rewrite 
ADD imageserver-site /etc/apache2/sites-enabled/000-default
ADD index.html /var/www/index.html
ADD js /var/www/js
ENTRYPOINT /etc/init.d/apache2 start && /bin/bash

