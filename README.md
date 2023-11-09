# EE407_Computer_Network
Computer Networks Class Git hub
EE407/EE507/CS455/CS555 2023 Fall - Course Project
9/21/2023

Students will work on an open-source network simulator named ns3 in this project, which is an event
driven network simulator. All teams will work on the same project which is described below.
Project Description:
(1). Background: Wireless Sensor Networks (WSNs) normally consist of a large number of tiny sensor
nodes that self- organize themselves into a multi-hop wireless network in order to collect ambient
information. WSNs under critical conditions like nuclear and space monitoring are important, and both
may involve high dose rate radiation environments [1].
The sensor data from the WSN is always accompanied by the node’s location, relative to a geographical
map of the monitored region. For reasons of cost, power, and GPS inaccessibility, non-GPS localization
algorithms such as DV-Hop (distance vector-hop) must be used in the nuclear and space applications
working in a high intensity radiation environment [1]. Refer to [2] for more information regarding
localization in WSNs.
DV-Hop is simple, economic and eases of implementation [2]. Please refer to the 4th page of [2] and the
7th and 8th page of [1] for DV-Hop. Also, the implementation of DV-Hop [4] can be downloaded from
GitHub.
Note that TA has prepared a document regarding technical tips. Please refer to it for more details.
(2). Tasks:
(i). Implement DV-Hop on ns3 by yourself or adapt the DV-hop implementation in [4]. You need to
illustrate and present the implementation (i.e., source codes with sufficient comments) in details. Note
that reusing the DV-hop implementation in [4] may still request some coding, compiling and debugging
efforts due to the nature of open-source projects.
(ii). Setup a simulation like the one in [1] but under the regular condition (i.e., nodes are always alive
during the simulation). Collect the performance data regarding the localization error under different
ratios of anchor nodes in the WSN. Also, more regular performance data (e.g., latency, package loss
rate) in network simulation is needed.
(iii). Implement the critical conditions in ns3 (i.e., nodes will be killed during the simulation). Algorithm 1
in [3] gives the pseudo codes of the procedure. Basically, the node death rate follows the Weibull
distribution shown in Figure 4 in [3], but you could implement a simple version to terminate the nodes
gradually.
(iv). Setup a simulation like the one in [1] but under the critical condition. Collect the performance data
regarding the localization error under different ratios of anchor nodes in the WSN. Also, more regular
performance data (e.g., latency, package loss rate) in network simulation is needed.
Tasks and Schedule: Tasks and schedule of this project are listed below (Total: 30 points):
2
1. Build the team (Due: Sep. 8, 2023) – Done J
2. Warm up with the background knowledge and do the project plan presentation (Due: Oct. 12,
2023) – 5 points
3. Understand and run the DV-Hop code and do the presentation to explain it in sufficient details
(Due: Nov. 7, 2023) – 8 points
• Weekly Report 3.I (Wednesday Oct. 18. 2023) – 1 point
• Weekly Report 3.II (Wednesday Oct. 25, 2023) – 1 point
• Weekly Report 3.III (Wednesday Nov. 1, 2023) – 1 point
• Presentation (Wednesday Nov. 7, 2023) – 5 points
4. Finish all tasks listed avove and deliver the final write-up and do the presentation (Due: Dec. 3,
2021) – 17 points
• Weekly Report 4.I (Wednesday Nov. 8, 2023) – 1 point
• Weekly Report 4.II (Wednesday Nov. 15, 2023) – 1 point
• Weekly Report 4.III (Wednesday Nov. 17, 2023) – 1 point
• Weekly Report 4.IV (Wednesday Nov. 22, 2023) – 1 point
• Presentation (Dec. 5 & 7, 2023) – 5 points
• Final Write-up and source codes submission – 8 points (Due: Dec. 10, 2023)
Note: Project leaders will get 5% bonus (5% x the Project Score the team received in total).
References:
[1]. Yu Liu, Karen Colins, Liqian Li, Exploit the Performance of the Localization Algorithm for Wireless
Sensor Networks under Gamma Radiation, in the Proceedings of the 36th Annual Conference of
Canadian Nuclear Society, Toronto, ON, Canada, 2016.6
[2]. Asma Mesmoudi, et. al, WIRELESS SENSOR NETWORKS LOCALIZATION ALGORITHMS: A
COMPREHENSIVE SURVEY, International Journal of Computer Networks & Communications (IJCNC)
Vol.5, No.6, November 2013
[3]. Michael Nishimura, Yu Liu, Liqian Li, Karen Colins, Comparison of Wireless Sensor Network Routing
Protocols under Gamma Radiation for Nuclear and Space Applications, in the Journal of Nuclear
Technology, Vol. 195, No. 2, 2016.8
[4]. DV-Hop implementation on ns3.20: https://github.com/pixki/dvhop
