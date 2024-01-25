########################################

#    DILVERT Data Collection Interface #

########################################

## USER SETTINGS ##
#LINES_TO_READ=-1 will read lines indefinitely (until Ctrl-C)
LINES_TO_READ = 10000
OUTFILE_NAME = "test16_AQc_200ps.csv" #Should be a .csv file.
SERIAL_PORT = 'COM12'
BAUD = 230400

####################

import serial

lines_read = 0

first_line_of_batch_flag = False

port = serial.Serial(SERIAL_PORT, BAUD)

try:
    port.open()
except serial.serialutil.SerialException:
    print("Port is already open...")
print("Serial port opened!")

with open(OUTFILE_NAME, 'wb') as write_file: #Overwrites the file if it exists.
    while(1):
        try:
            line = port.readline() #Blocks forever until new line is available.
            print(line)

            if b'~~~' in line:
                print("Found first line of batch; skipping...")
                first_line_of_batch_flag = True
            else:
                if first_line_of_batch_flag:
                    first_line_of_batch_flag = False
                else:
                    #Only write non-first-lines. 
                    write_file.write(line)
                    write_file.flush()
                    lines_read = lines_read + 1

            if LINES_TO_READ > 0 and lines_read > LINES_TO_READ:
                print("Read all my lines!")
                break
        except KeyboardInterrupt:
            print("Ctrl-C detected!")
            break
    port.close()
    print("Port closed! Thank u~")
