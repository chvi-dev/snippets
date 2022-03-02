 
#!/bin/sh
PATH=/etc:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin
host=$1
workdir=$host:/home/VssX-Services/Core
echo $workdir
echo $host
sshpass -p "08101227" scp -P 22 vss_x.ko vss_x.conf install-drv.sh /home/VssX-StorageSystem/Vss_X/Core/Vss_X/app/vss_srv $workdir
sshpass -p "08101227" ssh -p 22 $host /home/VssX-Services/Core/install-drv.sh


