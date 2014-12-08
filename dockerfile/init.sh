#!/bin/bash

if [ ! -d "/imageserver-data/.ssh" ]; then
  mkdir /imageserver-data/.ssh
  chown imageserver:imageserver /imageserver-data/.ssh
  chmod 700 /imageserver-data/.ssh
  su -l imageserver -c "ssh-keygen -t rsa -N \"\" -f /imageserver-data/.ssh/id_rsa"
fi

su -l imageserver -c "cat /imageserver-data/.ssh/id_rsa.pub > ~/.ssh/authorized_keys"

/etc/init.d/apache2 start; /etc/init.d/ssh start

while :; do /bin/bash; sleep 1; done

