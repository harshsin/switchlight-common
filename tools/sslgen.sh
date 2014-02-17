#!/bin/bash
############################################################
#
# Generate a local private key and self-signed certificate
# for use with SSL. 
# 
############################################################
set -e

if [ "$#" != 2 ]; then
    echo "usage: $0 KEYFILE CERTFILE"
    exit 1
fi

keyfile=$1
certfile=$2

csrfile="$(mktemp)"

# Generate a private key without a passphrase. 
openssl genrsa -out "$keyfile" 2048

# Generate a CSR
csrfields="
C=US
ST=CA
O=Big Switch Networks
localityName=Mountain View
commonName=localhost
organizationalUnitName=SwitchLight
emailAddress=support@bigswitch.com
"

# Generate the CSR
openssl req \
    -new \
    -batch \
    -subj "$(echo -n "$csrfields" | tr "\n" "/")" \
    -key "$keyfile" \
    -out "$csrfile" 

# Generate the certificate
openssl x509 -req -days 3650 -in "$csrfile" -signkey "$keyfile" -out "$certfile" 

rm "$csrfile"

