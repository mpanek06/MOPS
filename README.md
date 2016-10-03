# MOPS
Michal Oleszczyk Publish Subscribe - implementation of Public/Subscribe protocol which is light and possible to apply on RTnet in FreeRTOS.

# Build and run
First of all you need to build local broker and local process using makefiles in build directories: MOPS_Sources/local_Proces/build/ and MOPS_Sources/local_Broker/build. 

Before running those programms you need to set up RTnet - this can be done with rt_run.sh script run with root privileges. Then you are ready to run MOPS_Local_Broker and local pub and sub processes. 