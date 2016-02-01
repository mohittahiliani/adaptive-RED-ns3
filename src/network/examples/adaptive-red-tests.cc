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
 * Thanks to: Authors of red-tests.cc
 */

/**
 * NOTE: These validation tests are same as provided in ns-2 (ns/tcl/test/test-suite-adaptive-red.tcl)
 *
 * In this code the tests 1, 2, 6, 7, 8, 9, 10, 12, 13, 14 and 15 refer to tests named
 * red1, red1Adapt, fastlink, fastlinkAutowq, fastlinkAutothresh, fastlinkAdaptive, fastlinkAllAdapt,
 * fastlinkAllAdapt1, longlink, longlinkAdapt and longlinkAdapt1, respectively in the ns-2 file mentioned above.
 */

/** Network topology for tests: 1 and 2
 *
 *    10Mb/s, 2ms                            10Mb/s, 4ms
 * n0--------------|                    |---------------n4
 *                 |   1.5Mbps/s, 20ms  |
 *                 n2------------------n3
 *    10Mb/s, 3ms  |  QueueLimit = 25   |    10Mb/s, 5ms
 * n1--------------|                    |---------------n5
 *
 */

 /** Network topology for tests: 6, 7, 8, 9, 10 and 12
  *
  *    100Mb/s, 2ms                          100Mb/s, 4ms
  * n0--------------|                    |---------------n4
  *                 |   15Mbps/s, 20ms   |
  *                 n2------------------n3
  *    100Mb/s, 3ms |  QueueLimit = 1000 |   100Mb/s, 5ms
  * n1--------------|                    |---------------n5
  *
  */

  /** Network topology for tests: 13, 14 and 15
  *
  *    10Mb/s, 0ms                            10Mb/s, 2ms
  * n0--------------|                    |---------------n4
  *                 |   1.5Mbps/s, 100ms |
  *                 n2------------------n3
  *    10Mb/s, 1ms  |  QueueLimit = 100  |    10Mb/s, 3ms
  * n1--------------|                    |---------------n5
  *
  */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AdaptiveRedTests");

uint32_t checkTimes;
double avgQueueSize;

// The times
double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double client_start_time;
double client_stop_time;

NodeContainer n0n2;
NodeContainer n1n2;
NodeContainer n2n3;
NodeContainer n3n4;
NodeContainer n3n5;

Ipv4InterfaceContainer i0i2;
Ipv4InterfaceContainer i1i2;
Ipv4InterfaceContainer i2i3;
Ipv4InterfaceContainer i3i4;
Ipv4InterfaceContainer i3i5;

std::stringstream filePlotQueue;
std::stringstream filePlotQueueAvg;

void
CheckQueueSize (Ptr<Queue> queue)
{
  uint32_t qSize = StaticCast<RedQueue> (queue)->GetQueueSize ();

  avgQueueSize += qSize;
  checkTimes++;

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueSize, queue);

  std::ofstream fPlotQueue (filePlotQueue.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();

  std::ofstream fPlotQueueAvg (filePlotQueueAvg.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueueAvg << Simulator::Now ().GetSeconds () << " " << avgQueueSize / checkTimes << std::endl;
  fPlotQueueAvg.close ();
}

void
BuildAppsTest (uint32_t test)
{
  // SINK is in the right side
  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (n3n4.Get (1));
  sinkApp.Start (Seconds (sink_start_time));
  sinkApp.Stop (Seconds (sink_stop_time));

  // Connection one
  // Clients are in left side
  /*
   * Create the OnOff applications to send TCP to the server
   * onoffhelper is a client that send data to TCP destination
  */
  OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
  clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper1.SetAttribute ("PacketSize", UintegerValue (1000));

  // Connection two
  OnOffHelper clientHelper2 ("ns3::TcpSocketFactory", Address ());
  clientHelper2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper2.SetAttribute ("PacketSize", UintegerValue (1000));

  if (test == 6 || test == 7 || test == 8 || test == 9 || test == 10 || test == 12)
    {
      clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("100Mb/s")));
      clientHelper2.SetAttribute ("DataRate", DataRateValue (DataRate ("100Mb/s")));
    }
  else
    {
      clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
      clientHelper2.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mb/s")));
    }

  ApplicationContainer clientApps1;
  AddressValue remoteAddress (InetSocketAddress (i3i4.GetAddress (1), port));
  clientHelper1.SetAttribute ("Remote", remoteAddress);
  clientApps1.Add (clientHelper1.Install (n0n2.Get (0)));
  clientApps1.Start (Seconds (client_start_time));
  clientApps1.Stop (Seconds (client_stop_time));

  ApplicationContainer clientApps2;
  clientHelper2.SetAttribute ("Remote", remoteAddress);
  clientApps2.Add (clientHelper2.Install (n1n2.Get (0)));
  clientApps2.Start (Seconds (client_start_time));
  clientApps2.Stop (Seconds (client_stop_time));
}

int
main (int argc, char *argv[])
{
  LogComponentEnable ("RedQueue", LOG_LEVEL_INFO);

  uint32_t redTest;
  std::string redLinkDataRate = "1.5Mbps";
  std::string redLinkDelay = "20ms";

  std::string pathOut;
  bool writeForPlot = false;
  bool writePcap = false;
  bool flowMonitor = false;

  bool printRedStats = true;

  global_start_time = 0.0;
  sink_start_time = global_start_time;
  client_start_time = global_start_time + 1.5;
  global_stop_time = 7.0;
  sink_stop_time = global_stop_time + 3.0;
  client_stop_time = global_stop_time - 2.0;

  // Configuration and command line parameter parsing
  redTest = 1;
  // Will only save in the directory if enable opts below
  pathOut = "."; // Current directory
  CommandLine cmd;
  cmd.AddValue ("testNumber", "Run test 1, 2, 6, 7, 8, 9, 10, 12, 13, 14 or 15", redTest);
  cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
  cmd.AddValue ("writeForPlot", "<0/1> to write results for plot (gnuplot)", writeForPlot);
  cmd.AddValue ("writePcap", "<0/1> to write results in pcapfile", writePcap);
  cmd.AddValue ("writeFlowMonitor", "<0/1> to enable Flow Monitor and write their results", flowMonitor);

  cmd.Parse (argc, argv);
  if ( (redTest != 1) && (redTest != 2) && (redTest != 6) && (redTest != 7) && (redTest != 8) && (redTest != 9) && (redTest != 10) && (redTest != 12) && (redTest != 13) && (redTest != 14) && (redTest != 15) )
    {
      std::cout << "Invalid test number. Supported tests are 1, 2, 6, 7, 8, 9, 10, 12, 13, 14 or 15" << std::endl;
      exit (1);
    }

  NS_LOG_INFO ("Create nodes");
  NodeContainer c;
  c.Create (6);
  Names::Add ( "N0", c.Get (0));
  Names::Add ( "N1", c.Get (1));
  Names::Add ( "N2", c.Get (2));
  Names::Add ( "N3", c.Get (3));
  Names::Add ( "N4", c.Get (4));
  Names::Add ( "N5", c.Get (5));
  n0n2 = NodeContainer (c.Get (0), c.Get (2));
  n1n2 = NodeContainer (c.Get (1), c.Get (2));
  n2n3 = NodeContainer (c.Get (2), c.Get (3));
  n3n4 = NodeContainer (c.Get (3), c.Get (4));
  n3n5 = NodeContainer (c.Get (3), c.Get (5));

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpReno"));
  // 42 = headers size
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000 - 42));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  uint32_t meanPktSize = 500;

  // RED params
  NS_LOG_INFO ("Set RED params");
  Config::SetDefault ("ns3::RedQueue::Mode", StringValue ("QUEUE_MODE_PACKETS"));
  Config::SetDefault ("ns3::RedQueue::MeanPktSize", UintegerValue (meanPktSize));
  Config::SetDefault ("ns3::RedQueue::Wait", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueue::Gentle", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (0.002));
  Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (5));
  Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (15));
  Config::SetDefault ("ns3::RedQueue::LInterm", DoubleValue (10));
  Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (1000));

  if (redTest == 1) // test 1:
    {
      Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (25));
    }
  else if (redTest == 2) // test 2: Adaptive is set to TRUE; MinTh, MaxTh and QW are automatically set
    {
      Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (0.0));
      Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::Adaptive", BooleanValue (true));
      Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (25));
    }
  else if (redTest == 7) // test 7:
    {
      Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (0.0));
    }
  else if (redTest == 8) // test 8:
    {
      Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (0));
    }
  else if (redTest == 9) // test 9:
    {
      Config::SetDefault ("ns3::RedQueue::Adaptive", BooleanValue (true));
    }
  else if (redTest == 10) // test 10:
    {
      Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (0.0));
      Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::Adaptive", BooleanValue (true));
    }
  else if (redTest == 12) // test 12:
    {
      Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (0.0));
      Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::Adaptive", BooleanValue (true));
      Config::SetDefault ("ns3::RedQueue::TargetDelay", TimeValue (Seconds (0.2)));
    }
  else if (redTest == 13) // test 13:
    {
      Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (100));
    }
  else if (redTest == 14) // test 14:
    {
      Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (0.0));
      Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::Adaptive", BooleanValue (true));
      Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (100));
    }
  else if (redTest == 15) // test 15:
    {
      Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (-1.0));
      Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (0));
      Config::SetDefault ("ns3::RedQueue::Adaptive", BooleanValue (true));
      Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (100));
    }

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  internet.Install (c);

  NS_LOG_INFO ("Create channels");
  PointToPointHelper p2p;

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer devn0n2 = p2p.Install (n0n2);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));
  NetDeviceContainer devn1n2 = p2p.Install (n1n2);

  p2p.SetQueue ("ns3::RedQueue", // only backbone link has RED queue
                "LinkBandwidth", StringValue (redLinkDataRate),
                "LinkDelay", StringValue (redLinkDelay));
  p2p.SetDeviceAttribute ("DataRate", StringValue (redLinkDataRate));
  p2p.SetChannelAttribute ("Delay", StringValue (redLinkDelay));
  NetDeviceContainer devn2n3 = p2p.Install (n2n3);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("4ms"));
  NetDeviceContainer devn3n4 = p2p.Install (n3n4);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
  NetDeviceContainer devn3n5 = p2p.Install (n3n5);

  if (redTest == 13 || redTest == 14 || redTest == 15)
    {
      p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue ("0ms"));

      p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));

      p2p.SetQueue ("ns3::RedQueue", // only backbone link has RED queue
                    "LinkBandwidth", StringValue (redLinkDataRate),
                    "LinkDelay", StringValue ("100ms"));
      p2p.SetDeviceAttribute ("DataRate", StringValue (redLinkDataRate));
      p2p.SetChannelAttribute ("Delay", StringValue ("100ms"));

      p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

      p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));
    }
  else if (redTest == 6 || redTest == 7 || redTest == 8 || redTest == 9 || redTest == 10 || redTest == 12)
    {
      p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

      p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));

      p2p.SetQueue ("ns3::RedQueue", // only backbone link has RED queue
                    "LinkBandwidth", StringValue ("15Mbps"),
                    "LinkDelay", StringValue (redLinkDelay));
      p2p.SetDeviceAttribute ("DataRate", StringValue ("15Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue (redLinkDelay));

      p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue ("4ms"));

      p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
    }

  NS_LOG_INFO ("Assign IP Addresses");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  i0i2 = ipv4.Assign (devn0n2);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  i1i2 = ipv4.Assign (devn1n2);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  i2i3 = ipv4.Assign (devn2n3);

  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  i3i4 = ipv4.Assign (devn3n4);

  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  i3i5 = ipv4.Assign (devn3n5);

  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  BuildAppsTest (redTest);

  if (writePcap)
    {
      PointToPointHelper ptp;
      std::stringstream stmp;
      stmp << pathOut << "/red";
      ptp.EnablePcapAll (stmp.str ().c_str ());
    }

  Ptr<FlowMonitor> flowmon;
  if (flowMonitor)
    {
      FlowMonitorHelper flowmonHelper;
      flowmon = flowmonHelper.InstallAll ();
    }

  if (writeForPlot)
    {
      filePlotQueue << pathOut << "/" << "red-queue.plotme";
      filePlotQueueAvg << pathOut << "/" << "red-queue_avg.plotme";

      remove (filePlotQueue.str ().c_str ());
      remove (filePlotQueueAvg.str ().c_str ());
      Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (devn2n3.Get (0));
      Ptr<Queue> queue = nd->GetQueue ();
      Simulator::ScheduleNow (&CheckQueueSize, queue);
    }

  Simulator::Stop (Seconds (sink_stop_time));
  Simulator::Run ();

  Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (devn2n3.Get (0));
  RedQueue::Stats st = StaticCast<RedQueue> (nd->GetQueue ())->GetStats ();

  if (redTest == 1)
    {
      if (st.unforcedDrop < 4 || st.unforcedDrop > 6)
        {
          std::cout << "Drops due to prob mark not in expected range, should be around 5" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop < 37 || st.forcedDrop > 39)
        {
          std::cout << "Drops due to hard mark not in expected range, should be around 38" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop < 37 || st.qLimDrop > 39)
        {
          std::cout << "Drops due to queue full not in expected range, should be around 38" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 2)
    {
      if (st.unforcedDrop < 10 || st.unforcedDrop > 16)
        {
          std::cout << "Drops due to prob mark not in expected range, should be around 11" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop < 35 || st.forcedDrop > 42)
        {
          std::cout << "Drops due to hard mark not in expected range, should be around 36" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop < 35 || st.qLimDrop > 42)
        {
          std::cout << "Drops due to queue full not in expected range, should be around 36" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 6)
    {
      if (st.unforcedDrop < 50 || st.unforcedDrop > 126)
        {
          std::cout << "Drops due to prob mark not in expected range, should be around 96" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop < 47 || st.forcedDrop > 62)
        {
          std::cout << "Drops due to hard mark not in expected range, should be around 55" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop != 0)
        {
          std::cout << "There should be no drops due to queue full" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 7)
    {
      if (st.unforcedDrop < 46 || st.unforcedDrop > 91)
        {
          std::cout << "Number of drops due to prob mark is not in expected range, should be around 65" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop < 26 || st.forcedDrop > 43)
        {
          std::cout << "Number of drops due to hard mark is not in expected range, should be around 39" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop != 0)
        {
          std::cout << "There should be no drops due to queue full" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 8)
    {
      if (st.unforcedDrop < 130 || st.unforcedDrop > 179)
        {
          std::cout << "Number of drops due to prob mark is not in expected range, should be around 163" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop != 0)
        {
          std::cout << "There should be no drops due to hard mark" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop != 0)
        {
          std::cout << "There should be no drops due to queue full" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 9)
    {
      if (st.unforcedDrop < 64 || st.unforcedDrop > 94)
        {
          std::cout << "Number of drops due to prob mark is not in expected range, should be around 68" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop < 49 || st.forcedDrop > 76)
        {
          std::cout << "Number of drops due to hard mark is not in expected range, should be around 56" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop != 0)
        {
          std::cout << "There should be no drops due to queue full" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 10)
    {
      if (st.unforcedDrop < 99 || st.unforcedDrop > 185)
        {
          std::cout << "Number of drops due to prob mark is not in expected range, should be around 101" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop != 0)
        {
          std::cout << "There should be no drops due to hard mark" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop != 0)
        {
          std::cout << "There should be no drops due to queue full" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 12)
    {
      if (st.unforcedDrop != 275)
        {
          std::cout << "Number of drops due to prob mark should be 275" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop != 0)
        {
          std::cout << "There should be no drops due to hard mark" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop != 0)
        {
          std::cout << "There should be no drops due to queue full" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 13)
    {
      if (st.unforcedDrop < 32 || st.unforcedDrop > 60)
        {
          std::cout << "Number of drops due to prob mark is not in expected range, should be around 57" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop < 56 || st.forcedDrop > 89)
        {
          std::cout << "Number of drops due to hard mark is not in expected range, should be around 60" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop < 4 || st.qLimDrop > 33)
        {
          std::cout << "Number of drops due to queue full is not in expected range, should be around 10" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 14)
    {
      if (st.unforcedDrop < 99 || st.unforcedDrop > 185)
        {
          std::cout << "Number of drops due to prob mark is not in expected range, should be around 101" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop != 0)
        {
          std::cout << "There should be no drops due to hard mark" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop != 0)
        {
          std::cout << "There should be no drops due to queue full" << std::endl;
          exit (-1);
        }
    }
  else if (redTest == 15)
    {
      if (st.unforcedDrop < 99 || st.unforcedDrop > 185)
        {
          std::cout << "Number of drops due to prob mark is not in expected range, should be around 101" << std::endl;
          exit (-1);
        }
      else if (st.forcedDrop != 0)
        {
          std::cout << "There should be no drops due to hard mark" << std::endl;
          exit (-1);
        }
      else if (st.qLimDrop != 0)
        {
          std::cout << "There should be no drops due to queue full" << std::endl;
          exit (-1);
        }
    }

  if (flowMonitor)
    {
      std::stringstream stmp;
      stmp << pathOut << "/red.flowmon";

      flowmon->SerializeToXmlFile (stmp.str ().c_str (), false, false);
    }

  if (printRedStats)
    {
      std::cout << "*** RED stats from Node 2 queue ***" << std::endl;
      std::cout << "\t " << st.unforcedDrop << " drops due prob mark" << std::endl;
      std::cout << "\t " << st.forcedDrop << " drops due hard mark" << std::endl;
      std::cout << "\t " << st.qLimDrop << " drops due queue full" << std::endl;
    }

  Simulator::Destroy ();

  return 0;
}
