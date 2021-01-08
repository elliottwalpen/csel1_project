1) from your host go under /kernel and run :
    "make" // to compile the driver
    "sudo make install" // to install it
2) make sure to have your target with the DT updated for I2C
3) from your target go under /user/deamon and run :
    "./fandeamon" // to launch the deamon

    from here you can change mode and frequency with the nano-pi switches

4) from your target go under /user/app and run :
    "./app" // to launch the control terminal app

    from here you can change mode and frequency with the terminal

    Commands :
               automatic    set mode to automatic
               manual       set mode to manual
               <value>      set new frequency [2, 5, 10, 20]
               help         print shell help
               exit         exit program\n