dvhop
=====

DV-hop positioning algorithm for NS3.20

###Installing
Just download ns-allinone-3.20 and then:
```
$ cd $NS_HOME/src/
$ git clone https://github.com/pixki/dvhop.git
$ cd $NS_HOME
$ ./waf configure --enable-examples
$ ./waf --run dvhop-example
```

The ```$NS_HOME``` variable is set to the root folder of your NS3.20 installation. In case of the ns-allinone directory structure is ```$NS_ROOT/ns3.20```