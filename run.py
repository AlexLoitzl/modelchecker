#!/usr/bin/python3
import os
import subprocess
import time
import getopt
import sys

aag_files = "../aag_files/"

#timeout in seconds
time_out = 300

resultSet = {}

k_array = [5]

# Specify -1 for unbounded exploration
inner_bound_array = [-1]

outfile = "outputs"

#Specify Bound on memory in megabytes
mem_max=20000

try:
  os.remove("outputs")
except:
  pass

with open(outfile, "w") as o:
  o.write("Timeout: " + str(time_out) + "s, Maximum resident set: " + str(mem_max) + "M\n")
  o.write("BMC: k = " + str(k_array) + " | IMC: inner bound = " + str(inner_bound_array) + "\n")

def check_bmc(filename, k):
    process = subprocess.run(["systemd-run", "--scope", "-p", "MemoryMax="+str(mem_max) + "M", "--user", "./modelchecker", str(k), aag_files + filename], capture_output=True,timeout=time_out)
    return process.stdout.decode("UTF-8")

def bmc_loop(filename):
  for i in k_array:
    try:
      output = check_bmc(filename, i)
      if "FAIL" in output:
        return "FAIL (" + str(i) + ")"
    except:
      return "OUT OF TIME"
  if "OK" in output:
    return "OK (" + str(k_array[len(k_array)-1]) + ")"
  return "OUT OF MEMORY"

def check_imc(filename, inner_bound=-1):
    process = subprocess.run(["systemd-run", "--scope", "-p", "MemoryMax="+str(mem_max) +"M", "--user", "./modelchecker","-a " + str(inner_bound), aag_files + filename], capture_output=True,timeout=time_out) 
    return process.stdout.decode("UTF-8")

def single_imc(filename):
  try:
    output = check_imc(filename)
    if "OK" in output:
      return "OK"
    elif "FAIL" in output:
      return "FAIL"
  except Exception as err:
    print(err)
    return "OUT OF TIME"
  return "OUT OF MEMORY"

def imc_loop(filename):
  for i in inner_bound_array:
    try:
      output = check_imc(filename, i)
      if "OK" in output:
        return "OK (" + str(i) + ")"
      elif "FAIL" in output:
        return "FAIL (" + str(i) + ")"
    except:
      # Either timeout or out of memory
      pass
  return "OUT OF RESOURCES/TIME"

##### MAIN METHOD #####
                     
for file in os.listdir(aag_files):
  outputContent="instance,userTime,systemTime,maxResidentSize,totalTimeElapsed"
  output = {}
  print("running: " + file)
  
  # BMC
  out_bmc = bmc_loop(file);

  out_imc = single_imc(file);
    
  with open(outfile, "a") as o:
    o.write(file + "\n")
    o.write(out_bmc + " | " + out_imc + "\n")



