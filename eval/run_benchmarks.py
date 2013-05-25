#!/usr/bin/python

import os
import sys
import subprocess
import re

all_benchmarks = os.listdir('tests')
all_benchmarks.remove('Makefile')
all_benchmarks.remove('.svn')
all_benchmarks.sort()

all_configs = ['pthread', 'dmp_o', 'dmp_b', 'dthread']
runs = 3

cores = 'current'

if len(sys.argv) == 1:
	print 'Usage: '+sys.argv[0]+' <benchmark names> <config names> <runs>'
	print 'Configs:'
	for c in all_configs:
		print '  '+c
	print 'Benchmarks:'
	for b in all_benchmarks:
		print '  '+b
	sys.exit(1)

benchmarks = []
configs = []

for p in sys.argv:
	if p in all_benchmarks:
		benchmarks.append(p)
	elif p in all_configs:
		configs.append(p)
	elif re.match('^[0-9]+$', p):
		runs = int(p)
	elif p == '8core':
		cores = 8
	elif p == '4core':
		cores = 4
	elif p == '2core':
		cores = 2

if len(benchmarks) == 0:
	benchmarks = all_benchmarks

if len(configs) == 0:
	configs = all_configs

if cores == 8:
	os.system('echo 1 > /sys/devices/system/cpu/cpu1/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu2/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu3/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu4/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu5/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu6/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu7/online')
elif cores == 4:
	os.system('echo 0 > /sys/devices/system/cpu/cpu1/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu2/online')
	os.system('echo 0 > /sys/devices/system/cpu/cpu3/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu4/online')
	os.system('echo 0 > /sys/devices/system/cpu/cpu5/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu6/online')
	os.system('echo 0 > /sys/devices/system/cpu/cpu7/online')
elif cores == 2:
	os.system('echo 0 > /sys/devices/system/cpu/cpu1/online')
	os.system('echo 0 > /sys/devices/system/cpu/cpu2/online')
	os.system('echo 0 > /sys/devices/system/cpu/cpu3/online')
	os.system('echo 1 > /sys/devices/system/cpu/cpu4/online')
	os.system('echo 0 > /sys/devices/system/cpu/cpu5/online')
	os.system('echo 0 > /sys/devices/system/cpu/cpu6/online')
	os.system('echo 0 > /sys/devices/system/cpu/cpu7/online')

if runs < 4:
        print 'Warning: with fewer than 4 runs per benchmark, all runs are averaged. Request at least 4 runs to discard the min and max runs from the average.'

data = {}
try:
	for benchmark in benchmarks:
		data[benchmark] = {}
		for config in configs:
			data[benchmark][config] = []
	
			for n in range(0, runs):
				print 'Running '+benchmark+'.'+config
				os.chdir('tests/'+benchmark)
				
				start_time = os.times()[4]
				
				p = subprocess.Popen(['make', 'eval-'+config, 'NCORES='+str(cores)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				p.wait()
				
				time = os.times()[4] - start_time
				data[benchmark][config].append(time)
	
				os.chdir('../..')

except:
	print 'Aborted!'
	
print 'benchmark',
for config in configs:
	print '\t'+config,
print

for benchmark in benchmarks:
	print benchmark,
	for config in configs:
		if benchmark in data and config in data[benchmark] and len(data[benchmark][config]) == runs:
			if len(data[benchmark][config]) >= 4:
				mean = (sum(data[benchmark][config])-max(data[benchmark][config])-min(data[benchmark][config]))/(runs-2)
			else:
				mean = sum(data[benchmark][config])/runs
			print '\t'+str(mean),
		else:
			print '\tNOT RUN',
	print
