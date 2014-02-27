/testing/guestbin/swan-prep
# confirm that the network is alive
ping -n -c 4 127.0.0.1
# make sure that clear text does not get through
iptables -A INPUT -i lo -p icmp  -j LOGDROP
iptables -I INPUT -m policy --dir in --pol ipsec -j ACCEPT
# confirm with a ping 
ping -n -c 4 127.0.0.1
ipsec setup start
/testing/pluto/bin/wait-until-pluto-started
ipsec auto --add test1-1-ipv4
ipsec auto --add test1-2-ipv4
ipsec auto --status
echo "initdone"
