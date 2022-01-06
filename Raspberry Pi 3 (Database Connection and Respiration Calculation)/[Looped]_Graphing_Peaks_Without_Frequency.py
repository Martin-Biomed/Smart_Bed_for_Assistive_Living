import serial
import struct
import time
import numpy as np
from scipy.fft import fft
import matplotlib.pyplot as plt
from scipy.signal import butter, cheby1, lfilter

# This code does not publish values to the online DB, instead its used only to graph the waveform of the load cells over 1 minute

ser = serial.Serial('COM5', baudrate=9600, timeout=None)  #We will need two different serial monitors with different baud rates

#This code is meant to be used in conjuction with an Arduino running the code "Generates_Sinusoidal_Readings.ino"

def peak_detection(Butter_Filtered_Data,N,n,T,ocf):
    
    number_of_low_peaks = 0
    number_of_high_peaks = 0
    high_peak_sum = 0
    low_peak_sum = 0
    respiration_rate = 0
    consecutive_low_peaks = 0
    consecutive_high_peaks = 0
    
    possible_high_peaks = []
    possible_high_peak_locations = []
    possible_low_peaks = []
    possible_low_peak_locations =[]
    
    for x in range(7,N-(n+7)):  #We will not use the bottom and top 7 terms in this algorithm

         ############## Detecting the Possible Peaks ###############

            
        if (Butter_Filtered_Data[x] > Butter_Filtered_Data[x-1]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x-2]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x-3]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x-4]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x-5]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x-6]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x+1]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x+2]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x+3]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x+4]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x+5]) and (Butter_Filtered_Data[x] > Butter_Filtered_Data[x+6]) and (consecutive_high_peaks == 0):
            possible_high_peaks.append(Butter_Filtered_Data[x])
            possible_high_peak_locations.append(x)
            number_of_high_peaks = number_of_high_peaks + 1
            consecutive_high_peaks = 1  #Checks if two consecutive high peaks have happened (which should not happen)
            consecutive_low_peaks = 0

        if (Butter_Filtered_Data[x] < Butter_Filtered_Data[x-1]) and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x-2]) and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x-3]) and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x-4]) and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x-5])and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x-6]) and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x+1]) and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x+2] and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x+3]) and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x+4]) and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x+5]) and (Butter_Filtered_Data[x] < Butter_Filtered_Data[x+6]) and (consecutive_low_peaks == 0)):
            possible_low_peaks.append(Butter_Filtered_Data[x])
            possible_low_peak_locations.append(x)
            number_of_low_peaks = number_of_low_peaks + 1
            consecutive_high_peaks = 0
            consecutive_low_peaks = 1

      ############# Calculating Respiration Rate from Possible Peaks ##############

    if(number_of_high_peaks-1>0):
        for h in range(0,number_of_high_peaks-1):
               high_peak_sum = high_peak_sum + (possible_high_peak_locations[h+1]-possible_high_peak_locations[h])

    if(number_of_low_peaks-1>0):
        for h in range(0,number_of_low_peaks-1):
               low_peak_sum = low_peak_sum + (possible_low_peak_locations[h+1]-possible_low_peak_locations[h])

    high_peak_sum = high_peak_sum * T
    low_peak_sum = low_peak_sum * T

    print()
    print('Low Peaks: %f' % number_of_low_peaks)
    print('High Peaks: %f' % number_of_high_peaks)

    if(number_of_high_peaks == 1 and number_of_low_peaks > 1): #In the event that there is only one peak, we can't calculate respiration rate 
        respiration_rate = 60/(low_peak_sum/(number_of_low_peaks-1))

    elif(number_of_low_peaks == 1 and number_of_high_peaks > 1):
        respiration_rate = 60/(high_peak_sum/(number_of_high_peaks-1))

    elif((number_of_low_peaks > 1) and (number_of_high_peaks > 1)):        
        respiration_rate = ((low_peak_sum/(number_of_low_peaks-1))+(high_peak_sum/(number_of_high_peaks-1)))/2
        respiration_rate = 60/respiration_rate
        
    else:  #If only one high and low peak are detected, we have to use double (half the distance between peaks)
        respiration_rate = abs(30/(high_peak_sum - low_peak_sum))
             
    return possible_high_peaks, possible_low_peaks, respiration_rate


def butter_bandpass(lowcut, highcut, fs, order):
    nyq = 0.5 * fs
    lowcut = lowcut/nyq
    highcut = highcut/nyq
    b, a = butter(order, [lowcut, highcut], btype='band', analog=False) #Returns Numerator=b and Denominator=a polynomials for filter
    return b, a

def butter_bandpass_filter(data, lowcut, highcut, fs, order):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = lfilter(b, a, data)
    return y

def main(): 
        
        time.sleep(5)
        Previous_Time = 0
        person_on = 0
        i = 0
        n = 80      #Number of samples we remove at the beginning
        N = 480     #Number of Sample Points
        Sampling_Frequency = 10 #Value in Hz (Chosen based on how long it takes for one round of while loop [125ms])
        T = 1/Sampling_Frequency   #T is equal to 0.05 s = 50 ms
        lowcut = 0.09
        highcut = 0.9

        if (person_on == 0 and (ser.inWaiting()>=0)):
        
            b = ser.readline()  # Reads raw serial output from Arduino
            string_b = b.decode()  #Decodes raw data making it ready for conversion
            string = str(string_b)  #Converts data to string
            value_list = string.split(" ")
            print("Waiting .....")

            load_1_occupant_thresh = float(value_list[1])
            load_2_occupant_thresh = float(value_list[2])

            print("Occupant 1 Threshold: %f" % load_1_occupant_thresh)
            print("Occupant 2 Threshold: %f" % load_2_occupant_thresh)

            person_on = 1
                
        
        while (person_on == 1):

            Load_Sensor_1_Data = []
            Load_Sensor_2_Data = []
            Load_Sensor_3_Data = []
            Load_Sensor_4_Data = []

            Time_Vector = []
            
            Initial_Time = time.monotonic()

            High_Respiration_Threshold_1 = load_1_occupant_thresh*1.15 #Placeholder Variable
            Low_Respiration_Threshold_1 = load_1_occupant_thresh*0.85
            High_Respiration_Threshold_2 = load_2_occupant_thresh*1.15 #Placeholder Variable
            Low_Respiration_Threshold_2 = load_2_occupant_thresh*0.85
            
            previous_load_sensor_1 = 0
            previous_load_sensor_2 = 0


            while (i<N): #We create an array of N readings (FFTs should only be used with powers of 2)

                    Previous_Time = time.monotonic_ns()  #We need to standardize the time between loops to implement FFT
                    
                    while (ser.inWaiting()<0):
                        time.sleep(0.005)
                    
                    b = ser.readline()  # Reads raw serial output from Arduino
                    string_b = b.decode()  #Decodes raw data making it ready for conversion
                    string = str(string_b)  #Converts data to string
                    value_list = string.split(" ")
                               
                    if(Previous_Time != 0):
                                                 
                            load_sensor_1 = float(value_list[1])
                            load_sensor_2 = float(value_list[2])

                            if (load_sensor_1 > High_Respiration_Threshold_1 and load_sensor_1<Low_Respiration_Threshold_1): #If any load cell values exceed the threshold, they're changed to last stable value
                                load_sensor_1 = previous_load_sensor_1

                            if (load_sensor_2 > High_Respiration_Threshold_2 and load_sensor_2<Low_Respiration_Threshold_2):
                                load_sensor_2 = previous_load_sensor_2

                            print('Next iteration')
                            print('Load Sensor 1 Reading: %f' %  load_sensor_1)
                            print('Load Sensor 2 Reading: %f' %  load_sensor_2)
                                
                            Load_Sensor_1_Data.append(load_sensor_1) #We can also add a "decode('ascii') option to make the data neater once the readings are stable
                            Load_Sensor_2_Data.append(load_sensor_2)

                            previous_load_sensor_1 = load_sensor_1
                            previous_load_sensor_2 = load_sensor_2
        
                    time.sleep(0.02)
                    Time_Elapsed_In_Iteration = ((time.monotonic_ns() - Previous_Time)/(10**6)) #Time in ms
                        
                    i=i+1
            
                    print('Time Elapsed (ms): %f' % round(Time_Elapsed_In_Iteration,2))
                    print('Iteration: %f' % i)

                    Time_Vector.append(T*i)         
                    print()


            print(Respiration_Threshold_1)
            print(Respiration_Threshold_2)

            Butter_Filtered_Data_1 = butter_bandpass_filter(Load_Sensor_1_Data, lowcut, highcut, Sampling_Frequency, order=2)
            Butter_Filtered_Data_2 = butter_bandpass_filter(Load_Sensor_2_Data, lowcut, highcut, Sampling_Frequency, order=2)
            Butter_Filtered_Data = ((Butter_Filtered_Data_1 + Butter_Filtered_Data_2)/2)

            Adjusted_Butter_Filtered_Data_1 = Butter_Filtered_Data_1[n:N]
            Adjusted_Butter_Filtered_Data_2 = Butter_Filtered_Data_2[n:N]

            for x in range(0,N-n):
                 Adjusted_Butter_Filtered_Data_1[x] =  round(Adjusted_Butter_Filtered_Data_1[x], 4) #Fixed the peak detection issue
                 Adjusted_Butter_Filtered_Data_2[x] =  round(Adjusted_Butter_Filtered_Data_2[x], 4)
                
            Adjusted_Time_Vector = Time_Vector[n:N]

            High_Peaks_1, Low_Peaks_1, respiration_rate_1 = peak_detection(Adjusted_Butter_Filtered_Data_1, N, n, T, load_1_occupant_thresh)
            High_Peaks_2, Low_Peaks_2, respiration_rate_2 = peak_detection(Adjusted_Butter_Filtered_Data_2, N, n, T, load_2_occupant_thresh)

            respiration_rate = (respiration_rate_1 + respiration_rate_2)/2
            print('Respiration Rate: %f' % respiration_rate)
            print('High Peaks 1: ')
            print(High_Peaks_1)
            print('Low Peaks 1: ')
            print(Low_Peaks_1)
            print('High Peaks 2: ')
            print(High_Peaks_2)
            print('Low Peaks 2: ')
            print(Low_Peaks_2)


            plt.subplot(2,1,1) #There are 3 plots stacked on top of each other
            plt.title('Load Cell 1 Waveform from Arduino')
            plt.xlabel('Time (s)')
            plt.ylabel('Amplitude')
            plt.grid()
            plt.plot(Time_Vector, Load_Sensor_1_Data)

            plt.subplot(2,1,2)
            plt.title('Band-Passed Filtered Load Cell 1 Waveform')
            plt.xlabel('Time (s)')
            plt.ylabel('Amplitude')
            plt.grid()
            plt.plot(Time_Vector[0:N-n],Adjusted_Butter_Filtered_Data_1)


            plt.tight_layout()
            plt.show()

            i=0


main()

