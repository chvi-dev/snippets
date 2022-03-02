 
#!/bin/sh
PATH=/etc:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

cd /lib/modules/$(uname -r)
mkdir /lib/modules/$(uname -r)/VssX -m 777
cd /home/VssX-Services/Core/; cp vss_x.ko /lib/modules/$(uname -r)/VssX/vss_x.ko;
cp vss_x.conf /etc/modules-load.d
depmod -a
modprobe vss_x

echo "export PATH=\""$PATH:/home/VssX-Services"\"" >> ~/.bashrc
echo "export PATH=\""$PATH:/home/VssX-Services/Core"\"">> ~/.bashrc

export PATH="/home/VssX-Services"
export PATH="$PATH:/home/VssX-Services/Core"
