DV-hop
=====

DV-hop positioning algorithm for NS3.20

### Prerequisite
- Download and install ns-allinone-3.20
- Git

### Installing
```sh
$ cd $NS_HOME/src/
$ git clone https://github.com/GrahamCoatesFarley/EE407_Computer_Network.git
$ cd $NS_HOME
$ ./waf configure --enable-examples
```
The ```$NS_HOME``` variable is set to the root folder of your NS3.20 installation. In the case of the ns-allinone directory structure is ```$NS_ROOT/ns3.20```

### Running the example
```sh
$ ./waf --run dvhop-example
```

### Ouput
- The console outputs the average localization error for every simulation second
- Generated File `nodes.csv`: A CSV file of all node positions and whether they are anchor nodes or not   
- Generated File `dvhop.distances`: Distance table for every Node    
- Generated File `dvhop_report.csv`:  DV-Hop trilateration statistics. Includes, the number of alive nodes, average localization error, and number of nodes that can trilaterate.
