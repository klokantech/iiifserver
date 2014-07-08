#!/bin/bash

if [ ! -d "/imageserver-data/.ssh" ]; then
  mkdir /imageserver-data/.ssh
  chown imageserver:imageserver /imageserver-data/.ssh
  chmod 700 /imageserver-data/.ssh
  su -l imageserver -c "ssh-keygen -t rsa -N \"\" -f ~/.ssh/id_rsa"
fi

/etc/init.d/apache2 start;

while :; do /bin/bash; sleep 1; done

