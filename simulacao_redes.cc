/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//
// Network topology
//
//  n0
//     \ 5 Mb/s, 2ms
//      \          1.5Mb/s, 10ms
//       n2 -------------------------n3
//      /
//     / 5 Mb/s, 2ms
//   n1
//
// - all links are point-to-point links with indicated one-way BW/delay
// - CBR/UDP flows from n0 to n3, and from n3 to n1
// - FTP/TCP flow from n0 to n3, starting at time 1.2 to time 1.35 sec.
// - UDP packet size of 210 bytes, with per-packet interval 0.00375 sec.
//   (i.e., DataRate of 448,000 bps)
// - DropTail queues 
// - Tracing of queues and packet receptions to file "simple-global-routing.tr"

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/traffic-control-module.h"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleGlobalRoutingExample");

int 
main (int argc, char *argv[])
{
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
#if 1 
  LogComponentEnable ("SimpleGlobalRoutingExample", LOG_LEVEL_INFO);
#endif

  // Set up some default values for the simulation.  Use the 
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (210));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("18000Mb/s"));

  //DefaultValue::Bind ("DropTailQueue::m_maxPackets", 30);

  // Allow the user to override any of the defaults and the above
  // DefaultValue::Bind ()s at run-time, via command-line arguments
  CommandLine cmd;
  bool enableFlowMonitor = true;
  cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.Parse (argc, argv);

  // Here, we will explicitly create four nodes.  In more sophisticated
  // topologies, we could configure a node factory.
  NS_LOG_INFO ("Create nodes.");
  NodeContainer c;
  c.Create (4);
  NodeContainer n0n2 = NodeContainer (c.Get (0), c.Get (2));
  NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get (2));
  NodeContainer n3n2 = NodeContainer (c.Get (3), c.Get (2));

    // Global
    Ptr<Node> G1 = CreateObject<Node>();
    Ptr<Node> G2 = CreateObject<Node>();
    Ptr<Node> G3 = CreateObject<Node>();
    
    NodeContainer container_G1_G2(G1, G2);
    NodeContainer container_G2_G3(G2, G3);

    // America do Sul
    Ptr<Node> L1 = CreateObject<Node>();
    Ptr<Node> S1 = CreateObject<Node>();
    Ptr<Node> S2 = CreateObject<Node>();
    Ptr<Node> S3 = CreateObject<Node>();
    Ptr<Node> L2 = CreateObject<Node>();
    Ptr<Node> S4 = CreateObject<Node>();
    
    NodeContainer container_L1_G2(L1, G2);
    NodeContainer container_S1_L1(S1, L1);
    NodeContainer container_S2_L1(S2, L1);
    NodeContainer container_S3_L1(S3, L1);
    
    NodeContainer container_L2_G1(L2, G1);
    NodeContainer container_S4_L2(S4, L2);
    
    NodeContainer container_L1_L2(L1, L2);


    // America do Norte
    Ptr<Node> L3 = CreateObject<Node>();
    Ptr<Node> N1 = CreateObject<Node>();
    Ptr<Node> N2 = CreateObject<Node>();
    Ptr<Node> N3 = CreateObject<Node>();
    Ptr<Node> L4 = CreateObject<Node>();
    Ptr<Node> N4 = CreateObject<Node>();
    Ptr<Node> N5 = CreateObject<Node>();
    Ptr<Node> N6 = CreateObject<Node>();
    Ptr<Node> L5 = CreateObject<Node>();
    Ptr<Node> N7 = CreateObject<Node>();
    Ptr<Node> N8 = CreateObject<Node>();
    
    NodeContainer container_L3_G3(L3, G3);
    NodeContainer container_N1_L3(N1, L3);
    NodeContainer container_N2_L3(N2, L3);
    NodeContainer container_N3_L3(N3, L3);
    
    NodeContainer container_L4_G3(L4, G3);
    NodeContainer container_N4_L4(N4, L4);
    NodeContainer container_N5_L4(N5, L4);
    NodeContainer container_N6_L4(N6, L4);
    
    NodeContainer container_L5_G3(L5, G3);
    NodeContainer container_N7_L5(N7, L5);
    NodeContainer container_N8_L5(N8, L5);
    
    NodeContainer container_L3_L4(L3, L4);
    NodeContainer container_L3_L5(L3, L5);
    
    // Global
    c.Add(G1);
    c.Add(G2);
    c.Add(G3);
    
    // America do Sul
    c.Add(L1);
    c.Add(S1);
    c.Add(S2);
    c.Add(S3);
    c.Add(L2);
    c.Add(S4);
    
    // America do Norte
    c.Add(L3);
    c.Add(N1);
    c.Add(N2);
    c.Add(N3);
    c.Add(L4);
    c.Add(N4);
    c.Add(N5);
    c.Add(N6);
    c.Add(L5);
    c.Add(N7);
    c.Add(N8);
    
    InternetStackHelper internet;
    internet.Install (c);

  // We create the channels first without any IP addressing information
  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
  
    /* ############################### GLOBAL ############################### */
    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("20ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    NetDeviceContainer G1_G2 = p2p.Install(container_G1_G2);
    
    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("100ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    NetDeviceContainer G2_G3 = p2p.Install(container_G2_G3);

    /* ########################### AMERICA DO SUL ########################### */
    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    NetDeviceContainer S1_L1 = p2p.Install(container_S1_L1);
    NetDeviceContainer S2_L1 = p2p.Install(container_S2_L1);
    NetDeviceContainer S3_L1 = p2p.Install(container_S3_L1);
    NetDeviceContainer S4_L2 = p2p.Install(container_S4_L2);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("20ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    NetDeviceContainer L1_G2 = p2p.Install(container_L1_G2);
    NetDeviceContainer L2_G1 = p2p.Install(container_L2_G1);
    NetDeviceContainer L1_L2 = p2p.Install(container_L1_L2);

    /* ########################## AMERICA DO NORTE ########################## */
    p2p.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("40ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    NetDeviceContainer L3_G3 = p2p.Install(container_L3_G3);
    
    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("40ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    NetDeviceContainer L3_L5 = p2p.Install(container_L3_L5);
    
    p2p.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("20ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    NetDeviceContainer L3_L4 = p2p.Install(container_L3_L4);
    NetDeviceContainer L4_G3 = p2p.Install(container_L4_G3);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
    NetDeviceContainer N1_L3 = p2p.Install(container_N1_L3);
    NetDeviceContainer N2_L3 = p2p.Install(container_N2_L3);
    NetDeviceContainer N3_L3 = p2p.Install(container_N3_L3);
    NetDeviceContainer N4_L4 = p2p.Install(container_N4_L4);
    NetDeviceContainer N5_L4 = p2p.Install(container_N5_L4);
    NetDeviceContainer N6_L4 = p2p.Install(container_N6_L4);
    NetDeviceContainer N7_L5 = p2p.Install(container_N7_L5);
    NetDeviceContainer N8_L5 = p2p.Install(container_N8_L5);
    NetDeviceContainer L5_G3 = p2p.Install(container_L5_G3);

    TrafficControlHelper tch;
    tch.SetRootQueueDisc ("ns3::RedQueueDisc");
    tch.Install (G1_G2);
    tch.Install (G2_G3);



  // Later, we add IP addresses.
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;


    // Global
    ipv4.SetBase ("10.0.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_G1_G2 = ipv4.Assign(G1_G2);
    
    ipv4.SetBase ("10.0.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_G2_G3 = ipv4.Assign(G2_G3);
    
    // America do Sul
    ipv4.SetBase ("10.55.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_S1_L1 = ipv4.Assign(S1_L1);
    
    ipv4.SetBase ("10.55.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_S2_L1 = ipv4.Assign(S2_L1);
    
    ipv4.SetBase ("10.55.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_S3_L1 = ipv4.Assign(S3_L1);
    
    ipv4.SetBase ("10.55.7.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_S4_L2 = ipv4.Assign(S4_L2);
    
    ipv4.SetBase ("10.55.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_L1_G2 = ipv4.Assign(L1_G2);
    
    ipv4.SetBase ("10.55.5.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_L2_G1 = ipv4.Assign(L2_G1);
    
    ipv4.SetBase ("10.55.6.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_L1_L2 = ipv4.Assign(L1_L2);
    
    // America do Norte
    ipv4.SetBase ("10.1.10.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_L3_L5 = ipv4.Assign(L3_L5);
    
    ipv4.SetBase ("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_N1_L3 = ipv4.Assign(N1_L3);
    
    ipv4.SetBase ("10.1.8.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_N2_L3 = ipv4.Assign(N2_L3);
    
    ipv4.SetBase ("10.1.9.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_N3_L3 = ipv4.Assign(N3_L3);
    
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_L4_G3 = ipv4.Assign(L4_G3);
    
    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_N4_L4 = ipv4.Assign(N4_L4);
    
    ipv4.SetBase ("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_N5_L4 = ipv4.Assign(N5_L4);
    
    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_N6_L4 = ipv4.Assign(N6_L4);
    
    ipv4.SetBase ("10.1.13.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_L5_G3 = ipv4.Assign(L5_G3);
    
    ipv4.SetBase ("10.1.11.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_N7_L5 = ipv4.Assign(N7_L5);
    
    ipv4.SetBase ("10.1.12.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_N8_L5 = ipv4.Assign(N8_L5);
    
    ipv4.SetBase ("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interface_global_L3_L4 = ipv4.Assign(L3_L4);



  // Create router nodes, initialize routing database and set up the routing
  // tables in the nodes.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Create the OnOff application to send UDP datagrams of size
  // 210 bytes at a rate of 448 Kb/s
  NS_LOG_INFO ("Create Applications.");
  uint16_t port = 9;   // Discard port (RFC 863)
  OnOffHelper onoff ("ns3::UdpSocketFactory", 
                     Address (InetSocketAddress (interface_global_G1_G2.GetAddress(1), port)));
  onoff.SetConstantRate (DataRate ("4000kb/s"));
  ApplicationContainer apps = onoff.Install (G1);
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

  // Create a packet sink to receive these packets
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  apps = sink.Install (G2);
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

  // Create a similar flow from n3 to n1, starting at time 1.1 seconds
  onoff.SetAttribute ("Remote", 
                      AddressValue (InetSocketAddress (interface_global_G2_G3.GetAddress(1), port)));
  apps = onoff.Install (G1);
  apps.Start (Seconds (1.1));
  apps.Stop (Seconds (10.0));

  // Create a packet sink to receive these packets
  apps = sink.Install (G3);
  apps.Start (Seconds (1.1));
  apps.Stop (Seconds (10.0));
  
  
    uint32_t packetSize = 1024;
    Time interPacketInterval = Seconds (1.0);
    V4PingHelper ping (interface_global_S1_L1.GetAddress(1));

    ping.SetAttribute ("Interval", TimeValue (interPacketInterval));
    ping.SetAttribute ("Size", UintegerValue (packetSize));
    ping.SetAttribute ("Verbose", BooleanValue (true));
    apps = ping.Install (S4);
    apps.Start (Seconds (1.0));
    apps.Stop (Seconds (10.0));
  
    /*uint32_t packetSize = 1024;
    Time interPacketInterval = Seconds (1.0);
    V4PingHelper ping (interface_global_G2_G5.GetAddress(1));

    ping.SetAttribute ("Interval", TimeValue (interPacketInterval));
    ping.SetAttribute ("Size", UintegerValue (packetSize));
    ping.SetAttribute ("Verbose", BooleanValue (true));
    apps = ping.Install (G3);
    apps.Start (Seconds (1.0));
    apps.Stop (Seconds (10.0));*/
  
    /*onoff.SetAttribute ("Remote", 
                      AddressValue (InetSocketAddress (interface_global_G2_G3.GetAddress(0), port)));
    apps = onoff.Install (G1);
    apps.Start (Seconds (1.1));
    apps.Stop (Seconds (10.0));

    // Create a packet sink to receive these packets
    apps = sink.Install (G3);
    apps.Start (Seconds (1.1));
    apps.Stop (Seconds (10.0));
    
    onoff.SetAttribute ("Remote", 
                      AddressValue (InetSocketAddress (interface_global_G2_G5.GetAddress(0), port)));
    apps = onoff.Install (G2);
    apps.Start (Seconds (1.1));
    apps.Stop (Seconds (10.0));*/


    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll (ascii.CreateFileStream ("simple-global-routing.tr"));
    p2p.EnablePcapAll ("simple-global-routing");

  // Flow Monitor
  FlowMonitorHelper flowmonHelper;
  if (enableFlowMonitor)
    {
      flowmonHelper.InstallAll ();
    }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (11));
  Simulator::Run ();
  NS_LOG_INFO ("Done.");

  if (enableFlowMonitor)
    {
      flowmonHelper.SerializeToXmlFile ("simple-global-routing.flowmon", false, false);
    }

  Simulator::Destroy ();
  return 0;
}
