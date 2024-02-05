## DILVERT Data Analysis Shell ##
import numpy as np
import tkinter as tk
from tkinter import filedialog
import os

class DILVERT_Data_File():

    def __init__(self):
        self.qcoarse = []
        self.qfine = []


    #Parse the data stored in the CSV and store it in an object.        
    def parse_file(self, filename):

        with open(filename,'r') as read_file:
            lines = read_file.readlines()

        for line in lines:
            tok = line.split(",")

            self.qcoarse.append(int(tok[0]))
            self.qfine.append(int(tok[1]))


    def get_qcoarse_stddev(self):
        return np.std(self.qcoarse)

    def get_qcoarse_avg(self):
        return np.mean(self.qcoarse)

    def get_qfine_stddev(self):
        return np.std(self.qfine)

    def get_qfine_avg(self):
        return np.mean(self.qfine)

def pick_file():
    files = os.listdir()

    for i in range(len(files)):
        print(f"{i+1} - {files[i]}")

    user_input = input("Enter a #?")

    chosen_file = files[int(user_input)-1]

    print(f"Selected: {chosen_file}")

    return chosen_file

current_filename = "test123.csv"


d = DILVERT_Data_File()


d.parse_file(current_filename)

while(1):

    user_input = input(">")

    if user_input == "file":
        current_filename = pick_file()
        d.parse_file(current_filename)

    elif user_input == "help":
        print("Help urself")

    elif user_input == "qc avg":
        print(f"Qcoarse avg:{d.get_qcoarse_avg()}")

    elif user_input == "qc std":
        print(f"Qcoarse stddev:{d.get_qcoarse_stddev()}")

    elif user_input == "qf avg":
        print(f"Qfine avg:{d.get_qfine_avg()}")

    elif user_input == "qf std":
        print(f"Qfine stddev:{d.get_qfine_stddev()}")

    


    
