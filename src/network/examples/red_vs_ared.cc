/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 NITK Surathkal
 * 
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
 * Author: Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 * Thanks to: Authors of droptail_vs_red.cc
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"

#include <iostream>
#include <iomanip>
#include <map>

using namespace ns3;

int main (int argc, char *argv[])
{
  uint32_t    nLeaf = 10;
  uint32_t    maxPackets = 100;
  uint32_t    modeBytes  = 0;
  double      minTh = 5;
  double      maxTh = 15;
  uint32_t    pktSize = 512;
  std::string appDataRate = "10Mbps";
  std::string queueType = "RED";
  uint16_t port = 5001;
  std::string bottleNeckLinkBw = "1Mbps";
  std::string bottleNeckLinkDelay = "50ms";

  CommandLine cmd;
  cmd.AddValue ("nLeaf",     "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("maxPackets","Max Packets allowed in the queue", maxPackets);
  cmd.AddValue ("queueType", "Set Queue type to RED or ARED", queueType);
  cmd.AddValue ("appPktSize", "Set OnOff App Packet Size", pktSize);
  cmd.AddValue ("appDataRate", "Set OnOff App DataRate", appDataRate);
  cmd.AddValue ("modeBytes", "Set Queue mode to Packets <0> or bytes <1>", modeBytes);

  cmd.AddValue ("redMinTh", "RED queue minimum threshold", minTh);
  cmd.AddValue ("redMaxTh", "RED queue maximum threshold", maxTh);
  cmd.Parse (argc,argv);

  if ((queueType != "RED") && (queueType != "ARED"))
    {
      std::cout << "Invalid queue type: Use --queueType=RED or --queueType=ARED" << std::endl;
      exit (1);
    }

  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (appDataRate));

  if (queueType == "ARED")
    {
      // Adaptive RED Configuration
      minTh = maxTh = 0;
      Config::SetDefault ("ns3::RedQueue::Adaptive", BooleanValue (true));
      Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (0.0));
      Config::SetDefault ("ns3::RedQueue::LInterm", DoubleValue (10.0));
    }

  if (!modeBytes)
    {
      Config::SetDefault ("ns3::RedQueue::Mode", StringValue ("QUEUE_MODE_PACKETS"));
      Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (maxPackets));
    }
  else
    {
      Config::SetDefault ("ns3::RedQueue::Mode", StringValue ("QUEUE_MODE_BYTES"));
      Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (maxPackets * pktSize));
      minTh *= pktSize;
      maxTh *= pktSize;
    }

  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));
  bottleNeckLink.SetQueue ("ns3::RedQueue",
                           "MinTh", DoubleValue (minTh),
                           "MaxTh", DoubleValue (maxTh),
                           "LinkBandwidth", StringValue (bottleNeckLinkBw),
                           "LinkDelay", StringValue (bottleNeckLinkDelay));

  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("1ms"));

  PointToPointDumbbellHelper d (nLeaf, pointToPointLeaf,
                                nLeaf, pointToPointLeaf,
                                bottleNeckLink);

  // Install Stack
  InternetStackHelper stack;
  d.InstallStack (stack);

  // Assign IP Addresses
  d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  // Install on/off app on all right side nodes
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      sinkApps.Add (packetSinkHelper.Install (d.GetLeft (i)));
    }
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (30.0));

  ApplicationContainer clientApps;
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      // Create an on/off app sending packets to the left side
      AddressValue remoteAddress (InetSocketAddress (d.GetLeftIpv4Address (i), port));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (d.GetRight (i)));
    }
  clientApps.Start (Seconds (1.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (15.0)); // Stop before the sink

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  std::cout << "Running the simulation" << std::endl;
  Simulator::Run ();

  uint32_t totalRxBytesCounter = 0;
  for (uint32_t i = 0; i < sinkApps.GetN (); i++)
    {
      Ptr <Application> app = sinkApps.Get (i);
      Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
      totalRxBytesCounter += pktSink->GetTotalRx ();
    }

  if (queueType == "RED")
    {
      if (totalRxBytesCounter > 2738700)
        {
          std::cout << "RED Goodput is too high, should be about 9290.57 Bytes/sec" << std::endl;
          exit (-1);
        }
      else if (totalRxBytesCounter < 2322400)
        {
          std::cout << "RED Goodput is too low, should be about 9290.57 Bytes/sec" << std::endl;
          exit (-1);
        }
    }
  else if (queueType == "ARED")
    {
      if (totalRxBytesCounter > 2756100)
        {
          std::cout << "ARED Goodput is too high, should be about 10300.5 Bytes/sec" << std::endl;
          exit (-1);
        }
      else if (totalRxBytesCounter < 2317300)
        {
          std::cout << "ARED Goodput is too low, should be about 10300.5 Bytes/sec" << std::endl;
          exit (-1);
        }
    }

  std::cout << "----------------------------\nQueue Type:"
            << queueType
            << "\nGoodput Bytes/sec:" 
            << totalRxBytesCounter/Simulator::Now().GetSeconds() << std::endl;
  std::cout << "----------------------------" << std::endl;
  std::cout << "Destroying the simulation" << std::endl;

  Simulator::Destroy ();
  return 0;
}
