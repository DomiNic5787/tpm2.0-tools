#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause

source helpers.sh

handle_ek=0x81010007
ak_ctx=ak.ctx
ek_alg=rsa
ak_alg=rsa
digestAlg=sha256
signAlg=rsassa

file_input_data=secret.data
output_ek_pub=ek_pub.out
output_ak_pub=ak_pub.out
output_ak_pub_name=ak_name_pub.out
output_mkcredential=mkcredential.out

cleanup() {
    rm -f $output_ek_pub $output_ak_pub $output_ak_pub_name $output_mkcredential \
          $file_input_data output_ak grep.txt $ak_ctx

    tpm2_evictcontrol -Q -ao -c $handle_ek 2>/dev/null || true

    if [ "$1" != "no-shut-down" ]; then
          shut_down
    fi
}
trap cleanup EXIT

start_up

cleanup "no-shut-down"

echo "12345678" > $file_input_data

tpm2_createek -Q -c $handle_ek -G $ek_alg -p $output_ek_pub

tpm2_createak -Q -C $handle_ek -c $ak_ctx -G $ak_alg -D $digestAlg -s $signAlg -p $output_ak_pub -n $output_ak_pub_name

# Use -c in xxd so there is no line wrapping
file_size=`stat --printf="%s" $output_ak_pub_name`
Loadkeyname=`cat $output_ak_pub_name | xxd -p -c $file_size`

tpm2_makecredential -Q -e $output_ek_pub  -s $file_input_data  -n $Loadkeyname -o $output_mkcredential

# use no tpm backend
tpm2_makecredential -T none -Q -e $output_ek_pub  -s $file_input_data  -n $Loadkeyname -o $output_mkcredential

exit 0
