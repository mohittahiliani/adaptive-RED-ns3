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
 * Author: Mohit P. Tahiliani (tahiliani@nitk.edu.in)
 *
 */

#include "ns3/test.h"
#include "ns3/red-queue.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"

using namespace ns3;

// Tests to verify the working of *automatically set* parameters in ARED
class AutoRedQueueTestCase : public TestCase
{
public:
  AutoRedQueueTestCase ();
  virtual void DoRun (void);
private:
  void Enqueue (Ptr<RedQueue> queue, uint32_t size, uint32_t nPkt);
  void RunAutoRedTest (StringValue mode);
};

AutoRedQueueTestCase::AutoRedQueueTestCase ()
  : TestCase ("Sanity check on automatically set parameters of ARED")
{
}

void
AutoRedQueueTestCase::RunAutoRedTest (StringValue mode)
{
  uint32_t pktSize = 1000;
  uint32_t modeSize = 1;                                                                // 1 for packets; pktSize for bytes
  Ptr<RedQueue> queue = CreateObject<RedQueue> ();

  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");

  if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      modeSize = pktSize;
    }

  double minTh = 70 * modeSize;
  double maxTh = 150 * modeSize;
  uint32_t qSize = 300 * modeSize;

  // test 1: Verify automatic setting of QW. [QW = 0.0 with default LinkBandwidth]
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.0)), true,
                         "Verify that we can actually set the attribute QW");
  Enqueue (queue, pktSize, 300);
  RedQueue::Stats st = StaticCast<RedQueue> (queue)->GetStats ();
  if (queue->GetMode () == RedQueue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be zero dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }
  else if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be zero dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }


  // test 2: Verify automatic setting of QW. [QW = 0.0 with lesser LinkBandwidth]
  queue = CreateObject<RedQueue> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.0)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LinkBandwidth", DataRateValue (DataRate ("0.015Mbps"))), true,
                         "Verify that we can actually set the attribute LinkBandwidth");
  Enqueue (queue, pktSize, 300);
  st = StaticCast<RedQueue> (queue)->GetStats ();
  if (queue->GetMode () == RedQueue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 44, "There should be 44 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }
  else if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 75, "There should be 75 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }


  // test 3: Verify automatic setting of QW. [QW = -1.0 with default LinkBandwidth]
  queue = CreateObject<RedQueue> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (-1.0)), true,
                         "Verify that we can actually set the attribute QW");
  Enqueue (queue, pktSize, 300);
  st = StaticCast<RedQueue> (queue)->GetStats ();
  if (queue->GetMode () == RedQueue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be zero dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }
  else if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be zero dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }


  // test 4: Verify automatic setting of QW. [QW = -1.0 with lesser LinkBandwidth]
  queue = CreateObject<RedQueue> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (-1.0)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LinkBandwidth", DataRateValue (DataRate ("0.015Mbps"))), true,
                         "Verify that we can actually set the attribute LinkBandwidth");
  Enqueue (queue, pktSize, 300);
  st = StaticCast<RedQueue> (queue)->GetStats ();
  if (queue->GetMode () == RedQueue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 32, "There should be 32 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }
  else if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 58, "There should be 58 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }


  // test 5: Verify automatic setting of QW. [QW = -2.0 with default LinkBandwidth]
  queue = CreateObject<RedQueue> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (-2.0)), true,
                         "Verify that we can actually set the attribute QW");
  Enqueue (queue, pktSize, 300);
  st = StaticCast<RedQueue> (queue)->GetStats ();
  if (queue->GetMode () == RedQueue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 29, "There should be 29 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }
  else if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 55, "There should be 55 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }


  // test 6: Verify automatic setting of QW. [QW = -2.0 with lesser LinkBandwidth]
  queue = CreateObject<RedQueue> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (-2.0)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LinkBandwidth", DataRateValue (DataRate ("0.015Mbps"))), true,
                         "Verify that we can actually set the attribute LinkBandwidth");
  Enqueue (queue, pktSize, 300);
  st = StaticCast<RedQueue> (queue)->GetStats ();
  if (queue->GetMode () == RedQueue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 44, "There should be 44 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }
  else if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 76, "There should be 76 dropped packets  due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }


  // test 7: Verify automatic setting of minTh and maxTh. [minTh = maxTh = 0.0, with default LinkBandwidth]
  minTh = maxTh = 0;
  queue = CreateObject<RedQueue> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  Enqueue (queue, pktSize, 300);
  st = StaticCast<RedQueue> (queue)->GetStats ();
  if (queue->GetMode () == RedQueue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 43, "There should be 43 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 215, "There should be 215 dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }
  else if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 80, "There should be 80 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 202, "There should be 202 dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }


  // test 8: Verify automatic setting of minTh and maxTh. [minTh = maxTh = 0.0, with higher LinkBandwidth]
  minTh = maxTh = 0;
  queue = CreateObject<RedQueue> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinTh", DoubleValue (minTh)), true,
                         "Verify that we can actually set the attribute MinTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxTh", DoubleValue (maxTh)), true,
                         "Verify that we can actually set the attribute MaxTh");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LinkBandwidth", DataRateValue (DataRate ("150Mbps"))), true,
                         "Verify that we can actually set the attribute LinkBandwidth");
  Enqueue (queue, pktSize, 300);
  st = StaticCast<RedQueue> (queue)->GetStats ();
  if (queue->GetMode () == RedQueue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 159, "There should be 159 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be zero dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }
  else if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 211, "There should be 211 dropped packets due to probability mark");
      NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should be 0 dropped packets due to hard mark");
      NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should be zero dropped packets due to queue full");
    }
}

void
AutoRedQueueTestCase::Enqueue (Ptr<RedQueue> queue, uint32_t size, uint32_t nPkt)
{
  for (uint32_t i = 0; i < nPkt; i++)
    {
      queue->Enqueue (Create<Packet> (size));
    }
}

void
AutoRedQueueTestCase::DoRun (void)
{
  RunAutoRedTest (StringValue ("QUEUE_MODE_PACKETS"));
  RunAutoRedTest (StringValue ("QUEUE_MODE_BYTES"));
  Simulator::Destroy ();
}


// Tests to verify the working of *adaptive* parameter in ARED
class AdaptiveRedQueueTestCase : public TestCase
{
public:
  AdaptiveRedQueueTestCase ();
  virtual void DoRun (void);
private:
  void RunAdaptiveRedTest (StringValue mode);
};

AdaptiveRedQueueTestCase::AdaptiveRedQueueTestCase ()
  : TestCase ("Sanity check on adaptive parameter of ARED")
{
}

void
AdaptiveRedQueueTestCase::RunAdaptiveRedTest (StringValue mode)
{
  uint32_t    pktSize = 512;
  uint32_t    modeSize = 1;                                             // 1 for packets; pktSize for bytes
  double      minTh = 0;
  double      maxTh = 0;
  uint32_t    qSize = 100;
  uint32_t    nLeaf = 3;

  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (512));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("10Mbps"));

  Ptr<RedQueue> queue = CreateObject<RedQueue> ();

  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Adaptive", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute Adaptive");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.0)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (10.0)), true,
                         "Verify that we can actually set the attribute LInterm");

  if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      modeSize = pktSize;
      queue->SetTh (minTh * pktSize, maxTh * pktSize);
      queue->SetQueueLimit (qSize * pktSize);
    }

  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute  ("DataRate", StringValue ("1Mbps"));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue ("50ms"));
  bottleNeckLink.SetQueue ("ns3::RedQueue",
                           "MinTh", DoubleValue (minTh * modeSize),
                           "MaxTh", DoubleValue (maxTh * modeSize),
                           "LinkBandwidth", StringValue ("1Mbps"),
                           "LinkDelay", StringValue ("50ms"));

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
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), 5001));
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
      AddressValue remoteAddress (InetSocketAddress (d.GetLeftIpv4Address (i), 5001));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (d.GetRight (i)));
    }
  clientApps.Start (Seconds (1.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (15.0)); // Stop before the sink

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Run ();

  uint32_t totalRxBytesCounter = 0;
  for (uint32_t i = 0; i < sinkApps.GetN (); i++)
    {
      Ptr <Application> app = sinkApps.Get (i);
      Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
      totalRxBytesCounter += pktSink->GetTotalRx ();
    }

  if (queue->GetMode () == RedQueue::QUEUE_MODE_PACKETS)
    {
      NS_TEST_EXPECT_MSG_EQ (totalRxBytesCounter, 1773056, "Total received bytes should be 1773056");
    }
  else if (queue->GetMode () == RedQueue::QUEUE_MODE_BYTES)
    {
      NS_TEST_EXPECT_MSG_EQ (totalRxBytesCounter, 1735168, "Total received bytes should be 1735168");
    }

  Simulator::Destroy ();
}

void
AdaptiveRedQueueTestCase::DoRun (void)
{
  RunAdaptiveRedTest (StringValue ("QUEUE_MODE_PACKETS"));
  RunAdaptiveRedTest (StringValue ("QUEUE_MODE_BYTES"));
  Simulator::Destroy ();
}

static class AredQueueTestSuite : public TestSuite
{
public:
  AredQueueTestSuite ()
    : TestSuite ("adaptive-red-queue", UNIT)
  {
    AddTestCase (new AutoRedQueueTestCase (), TestCase::QUICK);         // Tests for automatically set parameters of ARED
    AddTestCase (new AdaptiveRedQueueTestCase (), TestCase::QUICK);     // Tests for adaptive parameter of ARED
  }
} g_aredQueueTestSuite;
