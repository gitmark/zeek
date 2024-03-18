# @TEST-DOC: Query the Prometheus endpoint and smoke check that zeek_version_info{...} is contained in the response for all cluster nodes.
# Note compilable to C++ due to globals being initialized to a record that
# has an opaque type as a field.
# @TEST-REQUIRES: test "${ZEEK_USE_CPP}" != "1"
#
# @TEST-PORT: BROKER_PORT1
# @TEST-PORT: BROKER_PORT2
# @TEST-PORT: BROKER_PORT3
# @TEST-PORT: BROKER_PORT4
# @TEST-PORT: BROKER_METRICS_PORT1
# @TEST-PORT: BROKER_METRICS_PORT2
# @TEST-PORT: BROKER_METRICS_PORT3
# @TEST-PORT: BROKER_METRICS_PORT4
#
# @TEST-REQUIRES: which curl
# @TEST-EXEC: zeek --parse-only %INPUT
# @TEST-EXEC: btest-bg-run manager-1 ZEEKPATH=$ZEEKPATH:.. CLUSTER_NODE=manager-1 zeek -B broker -b %INPUT
# @TEST-EXEC: btest-bg-run logger-1 ZEEKPATH=$ZEEKPATH:.. CLUSTER_NODE=logger-1 zeek -B broker -b %INPUT
# @TEST-EXEC: btest-bg-run proxy-1 ZEEKPATH=$ZEEKPATH:.. CLUSTER_NODE=proxy-1 zeek -b %INPUT
# @TEST-EXEC: btest-bg-run worker-1  ZEEKPATH=$ZEEKPATH:.. CLUSTER_NODE=worker-1 zeek -b %INPUT
# @TEST-EXEC: btest-bg-wait 120
# @TEST-EXEC: btest-diff manager-1/.stdout
# @TEST-EXEC: btest-diff manager-1/services.out

@TEST-START-FILE cluster-layout.zeek
redef Cluster::nodes = {
	["manager-1"] = [$node_type=Cluster::MANAGER, $ip=127.0.0.1, $p=to_port(getenv("BROKER_PORT1")), $metrics_port=to_port(getenv("BROKER_METRICS_PORT1"))],
	["logger-1"] = [$node_type=Cluster::LOGGER,   $ip=127.0.0.1, $p=to_port(getenv("BROKER_PORT2")), $manager="manager-1", $metrics_port=to_port(getenv("BROKER_METRICS_PORT2"))],
	["proxy-1"] = [$node_type=Cluster::PROXY,   $ip=127.0.0.1, $p=to_port(getenv("BROKER_PORT3")), $manager="manager-1", $metrics_port=to_port(getenv("BROKER_METRICS_PORT3"))],
	["worker-1"] = [$node_type=Cluster::WORKER,   $ip=127.0.0.1, $p=to_port(getenv("BROKER_PORT4")), $manager="manager-1", $metrics_port=to_port(getenv("BROKER_METRICS_PORT4"))],
};
@TEST-END-FILE

@TEST-START-FILE request-services.sh
#!/bin/sh

# This script makes repeat curl requests to find all of the metrics data from the
# hosts listed in the services output from the manager, and outputs it all into a
# single file.

services_url=$1
output_file=$2

services_data=$(curl -s -m 5 ${services_url})
echo "${services_data}\n" > ${output_file}

for target in $(echo ${services_data} | jq '.[0].targets.[]'); do
	host=$(echo ${target} | sed 's/"//g')
	echo ${host} >> ${output_file}
	echo "$(date +%s) requesting from ${host}" >> ${output_file}
	metrics=$(curl -s http://${host}/metrics)
	if [ $? == 0 ] ; then
		version_info=$(echo ${metrics} | grep -Eo "zeek_version_info\{[^}]+\}" | grep -o 'endpoint=\"[^"]*\"')
		echo ${version_info} >> ${output_file};
	else
		echo "Failed to request data from ${host}" >> ${output_file}
	fi
done
@TEST-END-FILE

@load policy/frameworks/cluster/experimental
@load policy/frameworks/telemetry/prometheus
@load base/frameworks/telemetry
@load base/utils/active-http

@if ( Cluster::node == "manager-1" )

# Query the Prometheus endpoint using ActiveHTTP for testing, oh my.
event run_test()
	{
	local services_url = fmt("http://localhost:%s/services.json", port_to_count(Telemetry::metrics_port));
	local req_cmd = fmt("sh ../request-services.sh %s %s", services_url, "services.out");
	when [req_cmd] ( local result = system(req_cmd) )
		{
		if ( result != 0 )
			{
			# This is bad.
			print "ERROR: Failed to request service information";
			exit(1);
			}

		terminate();
		}
	timeout 10sec
		{
		# This is bad.
		print "ERROR: Timed out requesting service information";
		exit(1);
		}
	}

# Use a dynamic metrics port for testing to avoid colliding on 9911/tcp
# when running tests in parallel.

event zeek_init()
	{
	print Cluster::node, "Telemetry::metrics_port from cluster config", Telemetry::metrics_port;
	}

event Cluster::Experimental::cluster_started()
	{
	# Run the test once all nodes are up
	schedule 2 secs { run_test() };
	}
@endif

# If any node goes down, terminate() right away.
event Cluster::node_down(name: string, id: string)
	{
	print fmt("node_down on %s", Cluster::node);
	terminate();
	}
