/testing/guestbin/swan-prep --x509
certutil -d sql:/etc/ipsec.d -D -n east
/testing/pluto/bin/wait-until-pluto-started
ipsec auto --add nss-cert-crl
ipsec auto --status |grep nss-cert-crl
echo "initdone"
