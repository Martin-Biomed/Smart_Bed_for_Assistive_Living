# Smart_Bed_for_Assistive_Living
 Code Used in the Honours Project EEE-2450 (2020)

This project resulted in a prototype device which was capable of measuring a patient's respiration rate, core body temperature and heart rate and upload these measurements to an online MongoDB database. This repository contains code used in the previously mentioned Honours Project from the University of Adelaide. In this repository you will find both Arduino IDE files, as well as Python files, which are both used to accomplish the project functionality. Hardware for the project includes the ESP8266 (Arduino IDE support package), Arduino UNO and the Raspberry Pi 3.

![image](https://user-images.githubusercontent.com/50542181/148348492-58ca6128-1ce2-4394-abac-6e31bc533bc8.png)


Project Team:

- Project Supervisor: Associate Professor Mathias Baumert
- Martin Gallo Picon: Bachelor of Engineering (Electrical and Electronic) (Biomedical) (Honours)
- Dingbang Lu: Bachelor of Engineering (Electrical and Electronic) (Honours)

More info at: https://projectswiki.eleceng.adelaide.edu.au/projects/index.php/Projects:2020s1-2450_Ambient_Intelligence_Technology_for_Assisted_Elderly_Living


The respiration rate measurement was done using load cells placed on a removable cover to be used on a bed. Load Cell Setup:



![image](https://user-images.githubusercontent.com/50542181/148348530-8786dc72-0598-450f-af4d-384fe44225f1.png)




The Wearable Part of the device functions like a headset, with electronics being fitted above both ears. Both the temperature and the heart rate sensors are fitted to the same ear. The ESP8266 is housed in the other ear, with a direct connection to an arm-mounted LiPo battery:



![image](https://user-images.githubusercontent.com/50542181/148348648-771819e5-f46d-44b7-a008-0fb4acad3469.png)

  ![image](https://user-images.githubusercontent.com/50542181/148348149-ed1a6ba3-beed-46fc-9152-f0fd8dcbe36e.png) ![image](https://user-images.githubusercontent.com/50542181/148348286-9fe3cd9b-47af-4d7e-b145-2efea1ad8cdb.png)
  


**Result:** The resulting prototype was able to consistently measure respiration rate (which was the most innovative part of the code). The core body temperature measurement was limited because the team was not able to acquire the preferred temperature sensor in time. Unfortunately, the heart rate detection only works well when the temperature measurement functionality are commented out in the ESP8266 code.

