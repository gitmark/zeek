# @TEST-EXEC: zeek -b -r $TRACES/tls/signed_certificate_timestamp.pcap $SCRIPTS/external-ca-list.zeek %INPUT
# @TEST-EXEC: cat ssl.log > ssl-all.log
# @TEST-EXEC: zeek -b -r $TRACES/tls/signed_certificate_timestamp-2.pcap $SCRIPTS/external-ca-list.zeek %INPUT
# @TEST-EXEC: cat ssl.log >> ssl-all.log
# @TEST-EXEC: btest-diff .stdout
# @TEST-EXEC: TEST_DIFF_CANONIFIER="$SCRIPTS/diff-remove-x509-names | $SCRIPTS/diff-remove-timestamps" btest-diff ssl-all.log

@load protocols/ssl/validate-sct

redef SSL::ct_logs += {
["\x03\x01\x9d\xf3\xfd\x85\xa6\x9a\x8e\xbd\x1f\xac\xc6\xda\x9b\xa7\x3e\x46\x97\x74\xfe\x77\xf5\x79\xfc\x5a\x08\xb8\x32\x8c\x1d\x6b"] = SSL::CTInfo($description="Venafi Gen2 CT log", $operator="Venafi", $url="ctlog-gen2.api.venafi.com/", $maximum_merge_delay=86400, $key="\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07\x03\x42\x00\x04\x8e\x27\x27\x7a\xb6\x55\x09\x74\xeb\x6c\x4b\x94\x84\x65\xbc\xe4\x15\xf1\xea\x5a\xd8\x7c\x0e\x37\xce\xba\x3f\x6c\x09\xda\xe7\x29\x96\xd3\x45\x50\x6f\xde\x1e\xb4\x1c\xd2\x83\x88\xff\x29\x2f\xce\xa9\xff\xdf\x34\xde\x75\x0f\xc0\xcc\x18\x0d\x94\x2e\xfc\x37\x01"),
["\x68\xf6\x98\xf8\x1f\x64\x82\xbe\x3a\x8c\xee\xb9\x28\x1d\x4c\xfc\x71\x51\x5d\x67\x93\xd4\x44\xd1\x0a\x67\xac\xbb\x4f\x4f\xfb\xc4"] = SSL::CTInfo($description="Google 'Aviator' log", $operator="Google", $url="ct.googleapis.com/aviator/", $maximum_merge_delay=86400, $key="\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07\x03\x42\x00\x04\xd7\xf4\xcc\x69\xb2\xe4\x0e\x90\xa3\x8a\xea\x5a\x70\x09\x4f\xef\x13\x62\xd0\x8d\x49\x60\xff\x1b\x40\x50\x07\x0c\x6d\x71\x86\xda\x25\x49\x8d\x65\xe1\x08\x0d\x47\x34\x6b\xbd\x27\xbc\x96\x21\x3e\x34\xf5\x87\x76\x31\xb1\x7f\x1d\xc9\x85\x3b\x0d\xf7\x1f\x3f\xe9"),
["\xac\x3b\x9a\xed\x7f\xa9\x67\x47\x57\x15\x9e\x6d\x7d\x57\x56\x72\xf9\xd9\x81\x00\x94\x1e\x9b\xde\xff\xec\xa1\x31\x3b\x75\x78\x2d"] = SSL::CTInfo($description="Venafi log", $operator="Venafi", $url="ctlog.api.venafi.com/", $maximum_merge_delay=86400, $key="\x30\x82\x01\x22\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05\x00\x03\x82\x01\x0f\x00\x30\x82\x01\x0a\x02\x82\x01\x01\x00\xa2\x5a\x48\x1f\x17\x52\x95\x35\xcb\xa3\x5b\x3a\x1f\x53\x82\x76\x94\xa3\xff\x80\xf2\x1c\x37\x3c\xc0\xb1\xbd\xc1\x59\x8b\xab\x2d\x65\x93\xd7\xf3\xe0\x04\xd5\x9a\x6f\xbf\xd6\x23\x76\x36\x4f\x23\x99\xcb\x54\x28\xad\x8c\x15\x4b\x65\x59\x76\x41\x4a\x9c\xa6\xf7\xb3\x3b\x7e\xb1\xa5\x49\xa4\x17\x51\x6c\x80\xdc\x2a\x90\x50\x4b\x88\x24\xe9\xa5\x12\x32\x93\x04\x48\x90\x02\xfa\x5f\x0e\x30\x87\x8e\x55\x76\x05\xee\x2a\x4c\xce\xa3\x6a\x69\x09\x6e\x25\xad\x82\x76\x0f\x84\x92\xfa\x38\xd6\x86\x4e\x24\x8f\x9b\xb0\x72\xcb\x9e\xe2\x6b\x3f\xe1\x6d\xc9\x25\x75\x23\x88\xa1\x18\x58\x06\x23\x33\x78\xda\x00\xd0\x38\x91\x67\xd2\xa6\x7d\x27\x97\x67\x5a\xc1\xf3\x2f\x17\xe6\xea\xd2\x5b\xe8\x81\xcd\xfd\x92\x68\xe7\xf3\x06\xf0\xe9\x72\x84\xee\x01\xa5\xb1\xd8\x33\xda\xce\x83\xa5\xdb\xc7\xcf\xd6\x16\x7e\x90\x75\x18\xbf\x16\xdc\x32\x3b\x6d\x8d\xab\x82\x17\x1f\x89\x20\x8d\x1d\x9a\xe6\x4d\x23\x08\xdf\x78\x6f\xc6\x05\xbf\x5f\xae\x94\x97\xdb\x5f\x64\xd4\xee\x16\x8b\xa3\x84\x6c\x71\x2b\xf1\xab\x7f\x5d\x0d\x32\xee\x04\xe2\x90\xec\x41\x9f\xfb\x39\xc1\x02\x03\x01\x00\x01"),
["\x56\x14\x06\x9a\x2f\xd7\xc2\xec\xd3\xf5\xe1\xbd\x44\xb2\x3e\xc7\x46\x76\xb9\xbc\x99\x11\x5c\xc0\xef\x94\x98\x55\xd6\x89\xd0\xdd"] = SSL::CTInfo($description="DigiCert Log Server", $operator="DigiCert", $url="https://ct1.digicert-ct.com/log/", $maximum_merge_delay=86400, $key="\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07\x03\x42\x00\x04\x02\x46\xc5\xbe\x1b\xbb\x82\x40\x16\xe8\xc1\xd2\xac\x19\x69\x13\x59\xf8\xf8\x70\x85\x46\x40\xb9\x38\xb0\x23\x82\xa8\x64\x4c\x7f\xbf\xbb\x34\x9f\x4a\x5f\x28\x8a\xcf\x19\xc4\x00\xf6\x36\x06\x93\x65\xed\x4c\xf5\xa9\x21\x62\x5a\xd8\x91\xeb\x38\x24\x40\xac\xe8"),
["\xa4\xb9\x09\x90\xb4\x18\x58\x14\x87\xbb\x13\xa2\xcc\x67\x70\x0a\x3c\x35\x98\x04\xf9\x1b\xdf\xb8\xe3\x77\xcd\x0e\xc8\x0d\xdc\x10"] = SSL::CTInfo($description="Google 'Pilot' log", $operator="Google", $url="https://ct.googleapis.com/pilot/", $maximum_merge_delay=86400, $key="\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07\x03\x42\x00\x04\x7d\xa8\x4b\x12\x29\x80\xa3\x3d\xad\xd3\x5a\x77\xb8\xcc\xe2\x88\xb3\xa5\xfd\xf1\xd3\x0c\xcd\x18\x0c\xe8\x41\x46\xe8\x81\x01\x1b\x15\xe1\x4b\xf1\x1b\x62\xdd\x36\x0a\x08\x18\xba\xed\x0b\x35\x84\xd0\x9e\x40\x3c\x2d\x9e\x9b\x82\x65\xbd\x1f\x04\x10\x41\x4c\xa0"),
["\xee\x4b\xbd\xb7\x75\xce\x60\xba\xe1\x42\x69\x1f\xab\xe1\x9e\x66\xa3\x0f\x7e\x5f\xb0\x72\xd8\x83\x00\xc4\x7b\x89\x7a\xa8\xfd\xcb"] = SSL::CTInfo($description="Google 'Rocketeer' log", $operator="Google", $url="https://ct.googleapis.com/rocketeer/", $maximum_merge_delay=86400, $key="\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07\x03\x42\x00\x04\x20\x5b\x18\xc8\x3c\xc1\x8b\xb3\x31\x08\x00\xbf\xa0\x90\x57\x2b\xb7\x47\x8c\x6f\xb5\x68\xb0\x8e\x90\x78\xe9\xa0\x73\xea\x4f\x28\x21\x2e\x9c\xc0\xf4\x16\x1b\xaa\xf9\xd5\xd7\xa9\x80\xc3\x4e\x2f\x52\x3c\x98\x01\x25\x46\x24\x25\x28\x23\x77\x2d\x05\xc2\x40\x7a"),
["\xbb\xd9\xdf\xbc\x1f\x8a\x71\xb5\x93\x94\x23\x97\xaa\x92\x7b\x47\x38\x57\x95\x0a\xab\x52\xe8\x1a\x90\x96\x64\x36\x8e\x1e\xd1\x85"] = SSL::CTInfo($description="Google 'Skydiver' log", $operator="Google", $url="https://ct.googleapis.com/skydiver/", $maximum_merge_delay=86400, $key="\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07\x03\x42\x00\x04\x12\x6c\x86\x0e\xf6\x17\xb1\x12\x6c\x37\x25\xd2\xad\x87\x3d\x0e\x31\xec\x21\xad\xb1\xcd\xbe\x14\x47\xb6\x71\x56\x85\x7a\x9a\xb7\x3d\x89\x90\x7b\xc6\x32\x3a\xf8\xda\xce\x8b\x01\xfe\x3f\xfc\x71\x91\x19\x8e\x14\x6e\x89\x7a\x5d\xb4\xab\x7e\xe1\x4e\x1e\x7c\xac"),
};

module SSL;

event ssl_established(c: connection)
	{
	print c$ssl$ct_proofs;
	for ( i in c$ssl$ct_proofs )
		{
		local proof = c$ssl$ct_proofs[i];
		if ( proof$logid !in SSL::ct_logs )
			print "Logid unknown: ", proof$logid;
		else
			{
			local log = SSL::ct_logs[proof$logid];
			print log$description, proof$valid;
			}
		}
	}
