#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <array>
#include <functional>
#include <numeric>

#include "ns3/ipv4-flow-classifier.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/ssid.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/core-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/abort.h"
#include "ns3/eht-phy.h"
#include "ns3/enum.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/wifi-acknowledgment.h"




// This is a simple example in order to show how to configure an IEEE 802.11be Wi-Fi network.
//
// It outputs the UDP or TCP goodput for every EHT MCS value, which depends on the MCS value (0 to
// 13), the channel width (20, 40, 80 or 160 MHz) and the guard interval (800ns, 1600ns or 3200ns).
// The PHY bitrate is constant over all the simulation run. The user can also specify the distance
// between the access point and the station: the larger the distance the smaller the goodput.
//
// The simulation assumes a configurable number of stations in an infrastructure network:
//
//  STA     AP
//    *     *
//    |     |
//   n1     n2
//
// Packets in this simulation belong to BestEffort Access Class (AC_BE).
// By selecting an acknowledgment sequence for DL MU PPDUs, it is possible to aggregate a
// Round Robin scheduler to the AP, so that DL MU PPDUs are sent by the AP via DL OFDMA.

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("mag");

bool fileExists(const std::string& filename)
{
    std::ifstream f(filename.c_str());
    return f.good();   
}

int
main(int argc, char* argv[])
{
    double powSta = 15.0;       // dBm
    double ccaEdTr = -62;    // dBm

    int dataRate = 107;
    int scenario = 1;
    bool udp{true};
    bool downlink{false};
    bool useRts{false};
    bool use80Plus80{false};
    uint16_t mpduBufferSize{512};
    std::string rateAdaptationManager = "ns3::ThompsonSamplingWifiManager";
    std::string emlsrLinks;
    uint16_t paddingDelayUsec{32};
    uint16_t transitionDelayUsec{128};
    uint16_t channelSwitchDelayUsec{100};
    bool switchAuxPhy{false};
    uint16_t auxPhyChWidth{320};
    bool auxPhyTxCapable{true};
    Time simulationTime{"120"}; //120s
    std::string csvPath = "results.csv";
    meter_u distance{0.0};
    double frequency{6};  // whether the first link operates in the 2.4, 5 or 6 GHz
    double frequency2{0}; // whether the second link operates in the 2.4, 5 or 6 GHz (0 means no
                          // second link exists)
    double frequency3{
        0}; // whether the third link operates in the 2.4, 5 or 6 GHz (0 means no third link exists)
    std::size_t nStations{1};
    std::string dlAckSeqType{"NO-OFDMA"};
    bool enableUlOfdma{false};
    bool enableBsrp{false};
    int mcs{11}; // -1 indicates an unset value
    uint32_t payloadSize =
    1500; // must fit in the max TX duration when transmitting at MCS 0 over an RU of 26 tones
    Time tputInterval{0}; // interval for detailed throughput measurement
    double minExpectedThroughput{0};
    double maxExpectedThroughput{0};
    Time accessReqInterval{0};

    CommandLine cmd(__FILE__);
    cmd.AddValue ("csvPath", "Path to output CSV file", csvPath);
    cmd.AddValue("dataRate", "Transmission data rate used by STAs", dataRate);
    cmd.AddValue("scenario", "Name or ID of the simulation scenario to run", scenario);
    cmd.AddValue(
        "frequency",
        "Whether the first link operates in the 2.4, 5 or 6 GHz band (other values gets rejected)",
        frequency);
    cmd.AddValue(
        "frequency2",
        "Whether the second link operates in the 2.4, 5 or 6 GHz band (0 means the device has one "
        "link, otherwise the band must be different than first link and third link)",
        frequency2);
    cmd.AddValue(
        "frequency3",
        "Whether the third link operates in the 2.4, 5 or 6 GHz band (0 means the device has up to "
        "two links, otherwise the band must be different than first link and second link)",
        frequency3);
    cmd.AddValue("emlsrLinks",
                 "The comma separated list of IDs of EMLSR links (for MLDs only)",
                 emlsrLinks);
    cmd.AddValue("emlsrPaddingDelay",
                 "The EMLSR padding delay in microseconds (0, 32, 64, 128 or 256)",
                 paddingDelayUsec);
    cmd.AddValue("emlsrTransitionDelay",
                 "The EMLSR transition delay in microseconds (0, 16, 32, 64, 128 or 256)",
                 transitionDelayUsec);
    cmd.AddValue("emlsrAuxSwitch",
                 "Whether Aux PHY should switch channel to operate on the link on which "
                 "the Main PHY was operating before moving to the link of the Aux PHY. ",
                 switchAuxPhy);
    cmd.AddValue("emlsrAuxChWidth",
                 "The maximum channel width (MHz) supported by Aux PHYs.",
                 auxPhyChWidth);
    cmd.AddValue("emlsrAuxTxCapable",
                 "Whether Aux PHYs are capable of transmitting.",
                 auxPhyTxCapable);
    cmd.AddValue("channelSwitchDelay",
                 "The PHY channel switch delay in microseconds",
                 channelSwitchDelayUsec);
    cmd.AddValue("distance",
                 "Distance in meters between the station and the access point",
                 distance);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
    cmd.AddValue("downlink",
                 "Generate downlink flows if set to 1, uplink flows otherwise",
                 downlink);
    cmd.AddValue("useRts", "Enable/disable RTS/CTS", useRts);
    cmd.AddValue("use80Plus80", "Enable/disable use of 80+80 MHz", use80Plus80);
    cmd.AddValue("mpduBufferSize",
                 "Size (in number of MPDUs) of the BlockAck buffer",
                 mpduBufferSize);
    cmd.AddValue("nStations", "Number of non-AP EHT stations", nStations);
    cmd.AddValue("dlAckType",
                 "Ack sequence type for DL OFDMA (NO-OFDMA, ACK-SU-FORMAT, MU-BAR, AGGR-MU-BAR)",
                 dlAckSeqType);
    cmd.AddValue("enableUlOfdma",
                 "Enable UL OFDMA (useful if DL OFDMA is enabled and TCP is used)",
                 enableUlOfdma);
    cmd.AddValue("enableBsrp",
                 "Enable BSRP (useful if DL and UL OFDMA are enabled and TCP is used)",
                 enableBsrp);
    cmd.AddValue(
        "muSchedAccessReqInterval",
        "Duration of the interval between two requests for channel access made by the MU scheduler",
        accessReqInterval);
    cmd.AddValue("payloadSize", "The application payload size in bytes", payloadSize);
    cmd.AddValue("tputInterval", "duration of intervals for throughput measurement", tputInterval);
    cmd.AddValue("minExpectedThroughput",
                 "if set, simulation fails if the lowest throughput is below this value",
                 minExpectedThroughput);
    cmd.AddValue("maxExpectedThroughput",
                 "if set, simulation fails if the highest throughput is above this value",
                 maxExpectedThroughput);
    cmd.Parse(argc, argv);
    /*
    if (useRts)
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("0"));
        Config::SetDefault("ns3::WifiDefaultProtectionManager::EnableMuRts", BooleanValue(true));
    }
    */
    if (dlAckSeqType == "ACK-SU-FORMAT")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE));
    }
    /*
    else if (dlAckSeqType == "MU-BAR")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_TF_MU_BAR));
    }
    else if (dlAckSeqType == "AGGR-MU-BAR")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));
    }
    */
    else if (dlAckSeqType != "NO-OFDMA")
    {
        NS_ABORT_MSG("Invalid DL ack sequence type (must be NO-OFDMA, ACK-SU-FORMAT, MU-BAR or "
                     "AGGR-MU-BAR)");
    }

    int minMcs = 0;
    int maxMcs = 13;
    if (mcs >= 0 && mcs <= 13)
    {
        minMcs = mcs;
        maxMcs = mcs;
    }
    for (int mcs = minMcs; mcs <= maxMcs; mcs++)
    {
        int linksNumber = 0;
        uint8_t index = 0;
        uint16_t maxChannelWidth = 20;
        int minGi = 3200;
        for (int channelWidth = 20; channelWidth <= maxChannelWidth;) // MHz
        {
            const auto is80Plus80 = (use80Plus80 && (channelWidth == 160));
            const std::string widthStr = is80Plus80 ? "80+80" : std::to_string(channelWidth);
            const auto segmentWidthStr = is80Plus80 ? "80" : widthStr;
            for (int gi = 3200; gi >= minGi;) // Nanoseconds
            {
                /*if (!udp)
                {
                    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
                }*/

                NodeContainer wifiStaNodes;
                wifiStaNodes.Create(nStations);
                NodeContainer wifiApNode;
                wifiApNode.Create(1);

                NetDeviceContainer apDevice;
                NetDeviceContainer staDevices;
                WifiMacHelper mac;
                WifiHelper wifi;

                wifi.SetStandard(WIFI_STANDARD_80211be);
                wifi.SetRemoteStationManager (rateAdaptationManager);
                std::array<std::string, 3> channelStr;
                std::array<FrequencyRange, 3> freqRanges;
                uint8_t nLinks = 0;
                std::string dataModeStr = "EhtMcs" + std::to_string(mcs);
                std::string ctrlRateStr;
                uint64_t nonHtRefRateMbps = EhtPhy::GetNonHtReferenceRate(mcs) / 1e6;

                if (frequency2 == frequency || frequency3 == frequency ||
                    (frequency3 != 0 && frequency3 == frequency2))
                {
                    NS_FATAL_ERROR("Frequency values must be unique!");
                }

                for (auto freq : {frequency, frequency2, frequency3})
                {
                    if (nLinks > 0 && freq == 0)
                    {
                        break;
                    }
                    if (freq == 6)
                    {
                        linksNumber +=1;
                        if (scenario==1)
                        {
                            channelStr[nLinks] += "{0, 20, BAND_6GHZ, 0}";
                        }
                        else if (scenario==2)
                        {
                            channelStr[nLinks] += "{0, 40, BAND_6GHZ, 0}";
                        }
                        else if (scenario==3)
                        {
                            channelStr[nLinks] += "{0, 80, BAND_6GHZ, 0}"; 
                        }
                        else if (scenario==4)
                        {
                            channelStr[nLinks] += "{0, 160, BAND_6GHZ, 0}";
                        }
                        else if (scenario==5)
                        {
                            channelStr[nLinks] += "{0, 320, BAND_6GHZ, 0}";
                        }
                        else {
                            channelStr[nLinks] += "{0, 20, BAND_6GHZ, 0}";
                        }
                        freqRanges[nLinks] = WIFI_SPECTRUM_6_GHZ;
                        wifi.SetRemoteStationManager(nLinks,
                                                     "ns3::ConstantRateWifiManager",
                                                     "DataMode",
                                                     StringValue(dataModeStr),
                                                     "ControlMode",
                                                     StringValue(dataModeStr));
                    }
                    else if (freq == 5)
                    {
                        linksNumber +=1;
                        channelStr[nLinks] += "BAND_5GHZ, 0}";
                        freqRanges[nLinks] = WIFI_SPECTRUM_5_GHZ;
                        wifi.SetRemoteStationManager(nLinks,
                                                     rateAdaptationManager);
                    }
                    else if (freq == 2.4)
                    {
                        linksNumber +=1;
                        channelStr[nLinks] += "BAND_2_4GHZ, 0}";
                        freqRanges[nLinks] = WIFI_SPECTRUM_2_4_GHZ;
                        wifi.SetRemoteStationManager(nLinks,
                                                     rateAdaptationManager);
                    }
                    else
                    {
                        NS_FATAL_ERROR("Wrong frequency value!");
                    }

                    if (is80Plus80)
                    {
                        channelStr[nLinks] += std::string(";") + channelStr[nLinks];
                    }

                    nLinks++;
                }

                if (nLinks > 1 && !emlsrLinks.empty())
                {
                    wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(true));
                }

                Ssid ssid = Ssid("ns3-80211be");

                SpectrumWifiPhyHelper phy(nLinks);
                phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
                phy.Set("ChannelSwitchDelay", TimeValue(MicroSeconds(channelSwitchDelayUsec)));
                // Ustawienie mocy stacji
                phy.Set("TxPowerStart", DoubleValue(powSta));
                phy.Set("TxPowerEnd", DoubleValue(powSta));

                // Ustawienie progu Energy Detection Threshold (CCA)
                phy.Set("CcaEdThreshold", DoubleValue(ccaEdTr));

                mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid),"MaxMissedBeacons", UintegerValue(1200));

                mac.SetEmlsrManager("ns3::DefaultEmlsrManager",
                                    "EmlsrLinkSet",
                                    StringValue(emlsrLinks),
                                    "MainPhyId",
                                    UintegerValue(0),
                                    "EmlsrPaddingDelay",
                                    TimeValue(MicroSeconds(paddingDelayUsec)),
                                    "EmlsrTransitionDelay",
                                    TimeValue(MicroSeconds(transitionDelayUsec)),
                                    "SwitchAuxPhy",
                                    BooleanValue(switchAuxPhy),
                                    "AuxPhyTxCapable",
                                    BooleanValue(auxPhyTxCapable),
                                    "AuxPhyChannelWidth",
                                    UintegerValue(auxPhyChWidth));
                
                for (uint8_t linkId = 0; linkId < nLinks; linkId++)
                {
                    phy.Set(linkId, "ChannelSettings", StringValue(channelStr[linkId]));

                    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
                    auto lossModel = CreateObject<LogDistancePropagationLossModel>();
                    if (freqRanges[linkId] == WIFI_SPECTRUM_2_4_GHZ)  lossModel->SetAttribute("ReferenceLoss", DoubleValue(40.0)); // dla 2.4 GHz
                    else if (freqRanges[linkId] == WIFI_SPECTRUM_5_GHZ) lossModel->SetAttribute("ReferenceLoss", DoubleValue(46.6)); // dla 5 GHz
                    else if (freqRanges[linkId] == WIFI_SPECTRUM_6_GHZ) lossModel->SetAttribute("ReferenceLoss", DoubleValue(48.0)); // dla 6 GHz
                    spectrumChannel->AddPropagationLossModel(lossModel);
                    phy.AddChannel(spectrumChannel, freqRanges[linkId]);
                }
                staDevices = wifi.Install(phy, mac, wifiStaNodes);
                
                if (dlAckSeqType != "NO-OFDMA")
                {
                    mac.SetMultiUserScheduler("ns3::RrMultiUserScheduler",
                                              "EnableUlOfdma",
                                              BooleanValue(enableUlOfdma),
                                              "EnableBsrp",
                                              BooleanValue(enableBsrp),
                                              "AccessReqInterval",
                                              TimeValue(accessReqInterval));
                }
                mac.SetType("ns3::ApWifiMac",
                            "EnableBeaconJitter",
                            BooleanValue(false),
                            "Ssid",
                            SsidValue(ssid));
                apDevice = wifi.Install(phy, mac, wifiApNode);

                // Set guard interval and MPDU buffer size
                Config::Set(
                    "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval",
                    TimeValue(NanoSeconds(gi)));
                Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MpduBufferSize",
                            UintegerValue(mpduBufferSize));

                // mobility.
                MobilityHelper mobility;
                Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

                positionAlloc->Add(Vector(0.0, 0.0, 0.0));

                for (uint32_t i = 0; i < nStations; ++i) {
                    positionAlloc->Add(Vector(0.0, distance, 0.0)); // Pozycje w 2D, z=0.0
                }
                mobility.SetPositionAllocator(positionAlloc);

                mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

                mobility.Install(wifiApNode);
                mobility.Install(wifiStaNodes);

                // Print position of each node
                std::cout << std::endl << "Node positions" << std::endl;
                // For random positioning models, make sure AP is at (0, 0)
                Ptr<MobilityModel> mobilityAp = wifiApNode.Get(0)->GetObject<MobilityModel> ();
                Vector pos = mobilityAp->GetPosition ();  
                pos.x = 0;
                pos.y = 0;

                //  - AP position
                Ptr<MobilityModel> position = wifiApNode.Get(0)->GetObject<MobilityModel> ();
                pos = position->GetPosition ();
                std::cout << "AP:\tx=" << pos.x << ", y=" << pos.y << std::endl;

                // - station positions
                for (NodeContainer::Iterator j = wifiStaNodes.Begin (); j != wifiStaNodes.End (); ++j) {
                    Ptr<Node> object = *j;
                    Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
                    Vector pos = position->GetPosition ();
                    std::cout << "Sta " << (uint32_t) object->GetId() << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
                }

                /* Internet stack*/
                InternetStackHelper stack;
                stack.Install(wifiApNode);
                stack.Install(wifiStaNodes);

                Ipv4AddressHelper address;
                address.SetBase("192.168.1.0", "255.255.255.0");
                Ipv4InterfaceContainer staNodeInterfaces;
                Ipv4InterfaceContainer apNodeInterface;

                staNodeInterfaces = address.Assign(staDevices);
                apNodeInterface = address.Assign(apDevice);

                // Install FlowMonitor
                FlowMonitorHelper flowmon;
                Ptr<FlowMonitor> flowMonitor = flowmon.InstallAll ();
                flowMonitor->CheckForLostPackets();
                Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

                /* Setting applications */
                ApplicationContainer serverApp;
                auto serverNodes = downlink ? std::ref(wifiStaNodes) : std::ref(wifiApNode);
                Ipv4InterfaceContainer serverInterfaces;
                NodeContainer clientNodes;
                for (std::size_t i = 0; i < nStations; i++)
                {
                    serverInterfaces.Add(downlink ? staNodeInterfaces.Get(i)
                                                  : apNodeInterface.Get(0));
                    clientNodes.Add(downlink ? wifiApNode.Get(0) : wifiStaNodes.Get(i));
                }

                const auto maxLoad = dataRate*1024*1024;
                // std::cout <<  EhtPhy::GetDataRate(mcs, channelWidth, NanoSeconds(gi), 1) << std::endl;
                if (udp)
                {
                    // UDP flow
                    uint16_t port = 9;
                    UdpServerHelper server(port);
                    serverApp = server.Install(serverNodes.get());

                    serverApp.Start(Seconds(0.0));
                    serverApp.Stop(simulationTime + Seconds(1.0));
                    const auto packetInterval = 1528 * 8.0 / maxLoad;

                    for (std::size_t i = 0; i < nStations; i++)
                    {
                        UdpClientHelper client(serverInterfaces.GetAddress(i), port);
                        client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
                        client.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
                        client.SetAttribute("PacketSize", UintegerValue(payloadSize));
                        ApplicationContainer clientApp = client.Install(clientNodes.Get(i));

                        clientApp.Start(Seconds(1.0));
                        clientApp.Stop(simulationTime + Seconds(1.0));
                    }
                }
                //phy.EnablePcap ("ap1.pcap", apDevice.Get (0));
                //phy.EnablePcap ("ap2.pcap", apDevice2.Get (0));

                Simulator::Stop(simulationTime + Seconds(1.0));
                Simulator::Run();

                FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();

                // Show results     
                std::ofstream myfile;
                if (fileExists(csvPath)) {
                    myfile.open (csvPath, std::ios::app); 
                }
                else {
                    myfile.open (csvPath, std::ios::app);  
                    myfile << "rngRun,flow,nStations,distance,linksNumber,loss,jitter,latency,throughput,rxBytes,txBytes,rxPackets,txPackets,timeFirstRxPacket,timeFirstTxPacket,timeLastRxPacket,timeLastTxPacket,jitterSum,delaySum,dataRate,scenario,switchAuxPhy" << std::endl;
                }
                for (auto flow : stats)
                {
                    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
                    std::cout << "Flow ID: " << flow.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")" << std::endl;

                    // Latency (Average Delay)
                    double delaySum = flow.second.delaySum.GetSeconds();
                    uint64_t rxPackets = flow.second.rxPackets;
                    double avgLatency = rxPackets > 0 ? (delaySum / rxPackets) : 0;
                    std::cout << "  Average Latency: " << avgLatency << " s" << std::endl;

                    // Jitter (Variation in Delay)
                    double jitterSum = flow.second.jitterSum.GetSeconds();
                    double avgJitter = rxPackets > 1 ? (jitterSum / (rxPackets - 1)) : 0;
                    std::cout << "  Average Jitter: " << avgJitter << " s" << std::endl;

                    // Packet Loss
                    uint64_t txPackets = flow.second.txPackets;
                    double lossRate = txPackets > 0 ? ((txPackets - rxPackets) * 100.0 / txPackets) : 0;
                    std::cout << "  Packet Loss Rate: " << lossRate << "%" << std::endl;

                    // Throughput
                    uint64_t rxBytes = flow.second.rxBytes;
                    uint64_t txBytes = flow.second.txBytes;
                    double timeFirstRxPacket = flow.second.timeFirstRxPacket.GetSeconds();
                    double timeLastRxPacket = flow.second.timeLastRxPacket.GetSeconds();
                    double timeFirstTxPacket = flow.second.timeFirstRxPacket.GetSeconds();
                    double timeLastTxPacket = flow.second.timeLastRxPacket.GetSeconds();

                    double throughput = (rxBytes > 0 && delaySum > 0) ? rxBytes*8/(timeLastRxPacket-timeFirstTxPacket)/1024/1024 : 0;
                    std::cout << "  Throughput: " << throughput << " Mbit/s" << std::endl;
                    std::cout << "---------------------------------" << std::endl;
                    if (flow.second.rxPackets > 0)
                    {
                        double avgPacketSize = (double)flow.second.rxBytes / flow.second.rxPackets;
                        std::cout << "  Avg packet size: " << avgPacketSize << " bytes" << std::endl;
                    }
                            
                    myfile << RngSeedManager::GetRun () << ',' << flow.first << ',' << nStations << ',' << distance << ',' << linksNumber << ',' << lossRate << ',' << avgJitter<< ','<< avgLatency<< ',' << throughput<< ',' << rxBytes << ',' << txBytes << ',' << rxPackets << ',' << txPackets << ',' << timeFirstRxPacket << ',' << timeFirstTxPacket << ',' << timeLastRxPacket << ',' << timeLastTxPacket << ',' << jitterSum << ',' << delaySum << ',' << dataRate << ',' << scenario  << ',' << switchAuxPhy << std::endl;
                            
                }
                
                Simulator::Destroy();
                
                myfile.close(); 
                index++;
                gi /= 2;
            }
            channelWidth *= 2;
        }
    }
    return 0;
}
