# raspberry-pi3-mini-os
### Turning OS
* Final version in E.file_system.
* goal: build a component-based os. Make kernel only have basic services. Added Extra functions by inserting component.
* shell command:
  1. ls: list file and directory.
  2. cd [directory]: enter the directory.
  3. cd : enter the root.
  4. incom [file name]: insert component
  5. rmcom [component's name]: remove component
  6. swap [file name] [component's name]: swap new_file to old component
  7. lscom: list component
  8. run: run application
* Document: https://hackmd.io/s/S1Ab0A9pX#
------------------------------------------------------------------------------------------------------------------------
### DESIGN AND IMPLEMENTATION 
* As shown in Figure 1, the main framework of Turning OS is divided into three major parts:
components, kernel symbol tables, and managers that provide kernel services. In the design
of components, components are classified into three categories: 
  1. components determined at system build time, for example, file system FAT16 and FAT32 which are classified as
file system components and the file system supported in the operating system is selected
according to the requirements of the device. 
  2. components that can be inserted, removed, and swapped, for instance, device driver of GPIO which is classified as a hardware device driver component. 
  3. components that only support swap operation, for examples,scheduler which is classified as a kernel service component. Turning OS supports component swapping operation, so it will save time in restarting and relocating applications and components. In lightweight operating systems, usually only one scheduling algorithm is used at a time. Therefore, the system swaps scheduler component according to application requirement and runs one scheduling algorithm at a time in system can save memory and resources in embedded systems with limited resources.

* Figure 1:
![image](https://github.com/tina0405/raspberry-pi3-mini-os/blob/master/Screenshot%20from%202020-04-09%2015-35-30.png)
                                                       
* The implementation of Turning OS is done on a single-board computer Raspbarry-Pi3 model B+ for its popularity and uses aarch64-linux-gnu-gcc package as building tool, and Ubuntu16.04.3 is choosen as the development environment. In term of the implementation, in addition to basic kernel services required in the test cases, such as shell, mutex, and fread/fwrite, the most important parts in the component operation functionality are the component manager and the kernel symbol table:
  1. The Component Manager is implemented as a background service. It is responsible for insert, remove or swap operations on components, and updates the component table and kernel symbol table. The operations of updating the component table and kernel symbol table are completed in the uninterrupted component manager service. Threrefore, it ensures that the component is installed or removed before the next use.
  2.  The Kernel Symbol Table is implemented taking as example the Loadable Kernel Module (LKM) of Linux. When inserting a component, the kernel symbol table serves as both an interface and relocation tool. After checking whether the functions of component fully meet the kernel API requirement, the system will relocate those functions and load components into the memory before the address of the component is inserted into the kernel symbol table. Kernel symbol table is the interface when a component is used by other applications or components. 


* Reference:
1. [Learning operating system development using Linux kernel and Raspberry Pi](https://github.com/s-matyukevich/raspberry-pi-os)
2. [Driver reference](https://github.com/bztsrc/raspi3-tutorial)
