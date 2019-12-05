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

#include <list> 
#include <iterator>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor.h"

#include "ns3/traffic-control-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleGlobalRoutingExample");

/**
 * Cria e instala no noh origem uma aplicacao que envia pacotes de tamanho packetSize a
 * a uma taxa de dataRate para a aplicacao no noh de endereco ip_destino que  esta
 * escutando na porta porta, durante o tempo tempo_inicio ate o tempo_fim.
 */
void aplicacao_enviadora(std::string protocol, Time tempo_inicio, Time tempo_fim, Ptr<Node> origem, Ipv4Address ip_destino, DataRate dataRate, uint32_t packetSize, uint16_t porta) {
    
    OnOffHelper onoff (protocol, 
                 Address (InetSocketAddress (ip_destino, porta)));
    onoff.SetConstantRate (dataRate, packetSize);
    ApplicationContainer apps = onoff.Install (origem);
    apps.Start (tempo_inicio);
    apps.Stop (tempo_fim);              
}

/**
 * Cria no e instala no noh receptor uma aplicacao receptora de pacotes que chegam na porta indicada.
 * Inicia no tempo tempo_inicio e termina no tempo tempo_fim.
 */
void aplicacao_receptora(std::string protocol, Time tempo_inicio, Time tempo_fim, Ptr<Node> receptor, uint16_t porta) {
    PacketSinkHelper sink (protocol,
                         Address (InetSocketAddress (Ipv4Address::GetAny (), porta)));
    ApplicationContainer apps = sink.Install (receptor);
    apps.Start (tempo_inicio);
    apps.Stop (tempo_fim);
}

/**
 * Criar uma aplicacao de envio de mensagem e outra recebimento nos nohs indicados
 * como origem e destino, respectivamente. Utiliza a porta indicada.
 */
void simular_fluxo(std::string protocol, Time tempo_inicio, Time tempo_fim, Ptr<Node> origem, Ipv4Address ip_destino, Ptr<Node> receptor, DataRate dataRate, uint32_t packetSize, uint16_t porta) {
    aplicacao_enviadora(protocol, tempo_inicio, tempo_fim, origem, ip_destino, dataRate, packetSize, porta);
    aplicacao_receptora(protocol, tempo_inicio, tempo_fim, receptor, porta);
}


/**
 * Testa a conexao de um no de origem com uma lista de ips realizando um ping
 * para cada um desses ips.
 */
void testar_conexao(Ptr<Node> origem, std::list <Ipv4Address> enderecos) {

    uint32_t packetSize = 1024;
    Time interPacketInterval = Seconds (1.0);
    
    double tempo_inicio = 1.0;
    double tempo_fim = 3.0;
    double diferenca = tempo_fim - tempo_inicio;
    
    std::list <Ipv4Address> :: iterator iterador;
    for(iterador = enderecos.begin(); iterador != enderecos.end(); ++iterador) { 
        Ipv4Address destino = *iterador; 
        V4PingHelper ping (destino);
        ping.SetAttribute ("Interval", TimeValue (interPacketInterval));
        ping.SetAttribute ("Size", UintegerValue (packetSize));
        ping.SetAttribute ("Verbose", BooleanValue (true));
        ApplicationContainer apps = ping.Install (origem);
        apps.Start (Seconds (tempo_inicio));
        apps.Stop (Seconds (tempo_fim));
        
        tempo_inicio += diferenca;
        tempo_fim += diferenca;
    }
}

int main (int argc, char *argv[]) {
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
#if 1 
  LogComponentEnable ("SimpleGlobalRoutingExample", LOG_LEVEL_INFO);
#endif

  // Set up some default values for the simulation.  Use the 
  //Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1024));
  //Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("100Mb/s"));

  //DefaultValue::Bind ("DropTailQueue::m_maxPackets", 30);

  // Allow the user to override any of the defaults and the above
  // DefaultValue::Bind ()s at run-time, via command-line arguments
  CommandLine cmd;
  bool enableFlowMonitor = true;
  cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.Parse (argc, argv);

  NS_LOG_INFO ("Create nodes.");
  NodeContainer c;

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
  
    std::string tamanho_fila_global = "10p";
    std::string tamanho_fila_comum = "6p";
  
    /* ############################### GLOBAL ############################### */
    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("20ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (tamanho_fila_global));
    NetDeviceContainer G1_G2 = p2p.Install(container_G1_G2);
    
    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("100ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (tamanho_fila_global));
    NetDeviceContainer G2_G3 = p2p.Install(container_G2_G3);

    /* ########################### AMERICA DO SUL ########################### */
	
    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (tamanho_fila_comum));
    NetDeviceContainer S1_L1 = p2p.Install(container_S1_L1);
    NetDeviceContainer S2_L1 = p2p.Install(container_S2_L1);
    NetDeviceContainer S3_L1 = p2p.Install(container_S3_L1);
    NetDeviceContainer S4_L2 = p2p.Install(container_S4_L2);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("20ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (tamanho_fila_comum));
    NetDeviceContainer L1_G2 = p2p.Install(container_L1_G2);
    NetDeviceContainer L2_G1 = p2p.Install(container_L2_G1);
    NetDeviceContainer L1_L2 = p2p.Install(container_L1_L2);

    /* ########################## AMERICA DO NORTE ########################## */
    p2p.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("40ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (tamanho_fila_comum));
    NetDeviceContainer L3_G3 = p2p.Install(container_L3_G3);
    
    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("40ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (tamanho_fila_comum));
    NetDeviceContainer L3_L5 = p2p.Install(container_L3_L5);
    
    p2p.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("20ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (tamanho_fila_comum));
    NetDeviceContainer L3_L4 = p2p.Install(container_L3_L4);
    NetDeviceContainer L4_G3 = p2p.Install(container_L4_G3);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
    p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (tamanho_fila_comum));
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
    
    tch.Install (S1_L1);
    tch.Install (S2_L1);
    tch.Install (S3_L1);
    tch.Install (S4_L2);
    
    tch.Install (L1_G2);
    tch.Install (L2_G1);
    tch.Install (L1_L2);
    
    tch.Install (L3_G3);
    tch.Install (L3_L5);
    tch.Install (L3_L4);
    tch.Install (L4_G3);
    
    tch.Install (N1_L3);
    tch.Install (N2_L3);
    tch.Install (N3_L3);
    tch.Install (N4_L4);
    tch.Install (N5_L4);
    tch.Install (N6_L4);
    tch.Install (N7_L5);
    tch.Install (N8_L5);
    tch.Install (L5_G3);


    /* ##################### ATRIBUICAO DE ENDERECOS IPS ##################### */
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


    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    

    /* ######################## CONFIGURAR MULTICAST ######################## */
    #if 0
    
       Ptr<Node> sender = N1;
       Ptr<NetDevice> senderIf = N1_L3.Get(0);
       multicast.SetDefaultMulticastRoute (sender, senderIf);
       
       
    #endif


    /* ##################### TESTE PING DE N1 PARA TODOS #################### */
    #if 0
        std::list <Ipv4Address> lista_de_ips;
        lista_de_ips.push_back(Ipv4Address("10.1.7.1")); // N1
        lista_de_ips.push_back(Ipv4Address("10.1.8.1")); // N2
        lista_de_ips.push_back(Ipv4Address("10.1.9.1")); // N3
        lista_de_ips.push_back(Ipv4Address("10.1.3.1")); // N4
        lista_de_ips.push_back(Ipv4Address("10.1.5.1")); // N5
        lista_de_ips.push_back(Ipv4Address("10.1.2.1")); // N6
        lista_de_ips.push_back(Ipv4Address("10.1.11.1")); // N7
        lista_de_ips.push_back(Ipv4Address("10.1.12.1")); // N8
        lista_de_ips.push_back(Ipv4Address("10.55.4.1")); // S1
        lista_de_ips.push_back(Ipv4Address("10.55.2.1")); // S2
        lista_de_ips.push_back(Ipv4Address("10.55.3.1")); // S3
        lista_de_ips.push_back(Ipv4Address("10.55.7.1")); // S4

        testar_conexao(N1, lista_de_ips);
    #endif


    /* ##################### CONFIGURACAO DAS SIMULACOES #################### */

    DataRate dataRate = DataRate ("4Mbps");
    uint32_t packetSize = 1024;
    std::string protocolo_TCP = "ns3::TcpSocketFactory";
    std::string protocolo_UDP = "ns3::TcpSocketFactory";

    std::string nome_arquivo_saida = "padrao_simple-global-routing.flowmon";


    /* ####################### SIMULACOES EXECUTADAS ######################## */

    // simular_fluxo(protocolo_TCP, Seconds (.0), Seconds (.0), , Ipv4Address(""), , dataRate, packetSize, 10);

    double tempo_fim = 60.0;

    #if 0
        nome_arquivo_saida = "simulacao_01.xml";
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 10);
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 11);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 12);
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 13);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 14);
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 15);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 16);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 17);
    #endif
    
    #if 0
        nome_arquivo_saida = "simulacao_02.xml";
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 18);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 19);
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 20);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 21);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 22);
    #endif
    
    #if 0
        nome_arquivo_saida = "simulacao_03.xml";
        
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 23);
        
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 24);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 25);
        
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 26);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 27);
        
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 28);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 29);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 30);
    #endif
   
    #if 0
        nome_arquivo_saida = "simulacao_04.xml";
        
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 31);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 32);
        
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 33);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 34);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 35);
    #endif
    
    #if 0
        nome_arquivo_saida = "simulacao_05.xml";
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 36);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 37);
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 38);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 39);
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 40);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 41);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 42);
    #endif
    
    #if 0
        nome_arquivo_saida = "simulacao_06.xml";
        
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 43);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 44);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 45);
    #endif
    
    #if 0
        nome_arquivo_saida = "simulacao_07.xml";
        
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 46);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 47);
        
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 48);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 49);
        
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 50);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 51);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 52);
    #endif
    
    #if 1
        nome_arquivo_saida = "simulacao_08.xml";
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), N1, Ipv4Address("10.1.9.1"), N3, dataRate, packetSize, 53);
        simular_fluxo(protocolo_UDP, Seconds (0.0), Seconds (tempo_fim), S1, Ipv4Address("10.55.2.1"), S2, dataRate, packetSize, 54);
        simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (tempo_fim), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 55);
    #endif

    //nome_arquivo_saida = "pode_me_excluir.xml";
    //simular_fluxo(protocolo_TCP, Seconds (0.0), Seconds (3.0), N2, Ipv4Address("10.55.3.1"), S3, dataRate, packetSize, 10);

    // Flow Monitor
    FlowMonitorHelper flowmonHelper;
    if (enableFlowMonitor) {
        flowmonHelper.InstallAll ();  
    }

    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop (Seconds (200));
    Simulator::Run ();
    NS_LOG_INFO ("Done.");

    if (enableFlowMonitor) {
        flowmonHelper.SerializeToXmlFile (nome_arquivo_saida, false, false);
    }

    Simulator::Destroy ();

    return 0;
}
