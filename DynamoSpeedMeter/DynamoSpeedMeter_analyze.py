import csv
import numpy as np
import matplotlib.pyplot as plt
# from scipy import optimize

times = []  # list of pulse rise timing

# read csv
csvfile = open("outputP1.txt", "r")
f = csv.reader(csvfile, delimiter=",")

# append to list
for row in f:
    times.append(float(row[0]))

# calculate
t_start = times[0]
times = [t - t_start for t in times]
times = np.array(times)
tdev = np.diff(times, n=1)
speed = (27*2.54*0.01*np.pi/16) / tdev  # [m/s]
speed = np.insert(speed, 0, 0.0)  # insert 0.0 to the first

# plot
plt.plot(times, speed)
plt.xlabel("Time [s]")
plt.ylabel("Speed [m/s]")
plt.ylim(0,10)
plt.grid()
plt.show()


# least mean square
# 01:9-28.5, 02:6.5-35, 03:13-27, 04:11-32.5
# start = 9
# end = 28.5
# index_to_plot = np.where((start < times) & (times < end))[0]
# index_start = index_to_plot[0]
# index_end = index_to_plot[-1] + 1
# times_to_plot = times[index_start:index_end]
# speed_to_plot = speed[index_start:index_end]
# 
# m = 18+51  # mass of bicycle+human
# g = 9.80  # gravitational accelaration
# v0 = times_to_plot[0]  # initial speed
# def fit_func(params,x,y):
#     mu = params[0]
#     d = params[1]
#     residual = y - (-mu*m*g/d + (v0+mu*m*g/d)*np.exp(-d/m*x))
#     return residual
# params0 = [0.,0.,]
# result = optimize.leastsq(fit_func, params0, args=(times_to_plot,speed_to_plot))
# print(result)
# mu_fit = result[0][0]
# d_fit = result[0][1]