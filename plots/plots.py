from matplotlib import pyplot as plt

log_file = open("/var/log/drbd_log.txt","r");
cmd_count = {}
msg_type_count = {}
time_count = {}

for line in log_file.readlines():
	data = line.split()
	if(data[5] in cmd_count):
		cmd_count[data[5]] += 1
	else:
		cmd_count[data[5]] = 1
	
	if(data[4] in msg_type_count):
		msg_type_count[data[4]] += 1
	else:
		msg_type_count[data[4]] = 1
	
	data[2] = data[2][0:-3]
	if(data[2] in time_count):
		time_count[data[2]] += 1
	else:
		time_count[data[2]] = 1

for key, value in cmd_count.items(): 
        print ("{} : {}".format(key,value))

print()
for key,value in msg_type_count.items():
	print ("{} : {}".format(key,value))

print()
for key,value in time_count.items():
	print ("{} : {}".format(key,value))

fig_pie = plt.figure()
plt.pie(cmd_count.values(),labels=cmd_count.keys())
plt.savefig("pie.png")
fig_pie.clear()
plt.close(fig_pie)

fig_bar = plt.figure()
plt.bar(time_count.keys(),time_count.values())
plt.xlabel("Time")
plt.ylabel("Number of Packets")
plt.title("Network Traffic")
plt.savefig("barplot.png")
fig_bar.clear()
plt.close(fig_bar)
