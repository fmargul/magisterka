

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/eht-phy.h"
#include "ns3/enum.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-server.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/node-list.h"
#include "ns3/core-module.h"
#include <array>
#include <functional>
#include <numeric>



using namespace ns3;

NS_LOG_COMPONENT_DEFINE("mag2");


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
    int idSLD = 0;
    bool udp{true};
    bool downlink{false};
    bool useRts{false};
    uint16_t mpduBufferSize{512};
    std::string rateAdaptationManager = "ns3::ThompsonSamplingWifiManager";
    std::string emlsrLinks;
    uint16_t paddingDelayUsec{32};
    uint16_t transitionDelayUsec{128};
    uint16_t channelSwitchDelayUsec{100};
    bool switchAuxPhy{false};
    uint16_t auxPhyChWidth{160};
    bool auxPhyTxCapable{true};
    std::string csvPath = "results.csv";
    Time simulationTime{"120"};
    meter_u distance{1.0};
    double frequency{2.4};  // whether the first link operates in the 2.4, 5 or 6 GHz 
    double frequency2{}; // whether the second link operates in the 2.4, 5 or 6 GHz (0 means no
                          // second link exists)
    double frequency3{}; // whether the third link operates in the 2.4, 5 or 6 GHz (0 means no third link exists)
    std::size_t nStations{1};

    std::string dlAckSeqType{"NO-OFDMA"};
    bool enableUlOfdma{false};
    bool enableBsrp{false};
    int mcs{11}; // -1 indicates an unset value
    uint32_t payloadSize =
        1500; // must fit in the max TX duration when transmitting at MCS 0 over an RU of 26 tones
    std::string scenario = "A";
    Time accessReqInterval{0};
    u_int32_t gi{3200};

    CommandLine cmd(__FILE__);
    cmd.AddValue("idSLD", "Identifier of the link used by the SLD", idSLD);
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
    cmd.AddValue ("csvPath", "Path to output CSV file", csvPath);
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
    cmd.AddValue("mcs", "if set, limit testing to a specific MCS (0-11)", mcs);
    cmd.AddValue("payloadSize", "The application payload size in bytes", payloadSize);
    cmd.AddValue("rateAdaptationManager", "Wifi rate adaptation manager", rateAdaptationManager);

    cmd.Parse(argc, argv);

    if (useRts)
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("0"));
        Config::SetDefault("ns3::WifiDefaultProtectionManager::EnableMuRts", BooleanValue(true));
    }

    if (dlAckSeqType == "ACK-SU-FORMAT")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE));
    }
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
    else if (dlAckSeqType != "NO-OFDMA")
    {
        NS_ABORT_MSG("Invalid DL ack sequence type (must be NO-OFDMA, ACK-SU-FORMAT, MU-BAR or "
                     "AGGR-MU-BAR)");
    }

    if (!udp)
    {
        Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
    }

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nStations);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNodes2;
    wifiStaNodes2.Create(nStations);
    NodeContainer wifiApNode2;
    wifiApNode2.Create(1);

    NetDeviceContainer apDevice;
    NetDeviceContainer staDevices;
    NetDeviceContainer apDevice2;
    NetDeviceContainer staDevices2;

    WifiMacHelper mac;
    WifiHelper wifi;

    wifi.SetStandard(WIFI_STANDARD_80211be);

    std::array<std::string, 3> channelStr;
    std::array<FrequencyRange, 3> freqRanges;
    int nLinks = 0;

    if (frequency2 == frequency || frequency3 == frequency ||
        (frequency3 != 0 && frequency3 == frequency2))
    {
        NS_FATAL_ERROR("Frequency values must be unique!");
    }

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
            channelStr[nLinks] += "{0, 20, BAND_6GHZ, 0}";

            freqRanges[nLinks] = WIFI_SPECTRUM_6_GHZ;
            wifi.SetRemoteStationManager(nLinks,
                rateAdaptationManager);
        }
        else if (freq == 5)
        {

            channelStr[nLinks] += "{0, 20, BAND_5GHZ, 0}";


            freqRanges[nLinks] = WIFI_SPECTRUM_5_GHZ;
            wifi.SetRemoteStationManager(nLinks,
                rateAdaptationManager);
        }
        else if (freq == 2.4)
        {
            channelStr[nLinks] += "{0, 20, BAND_2_4GHZ, 0}";
            freqRanges[nLinks] = WIFI_SPECTRUM_2_4_GHZ;
            wifi.SetRemoteStationManager(nLinks,
                rateAdaptationManager);
        }
        else
        {
            NS_FATAL_ERROR("Wrong frequency value!");
        }


        nLinks++;
    }


    Ssid ssid;


    if(nLinks > 1 && !emlsrLinks.empty())
    {
        wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(true));
    }
    SpectrumWifiPhyHelper phy(nLinks);
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.Set("ChannelSwitchDelay", TimeValue(MicroSeconds(channelSwitchDelayUsec)));
    // Ustawienie mocy stacji
    phy.Set("TxPowerStart", DoubleValue(powSta));
    phy.Set("TxPowerEnd", DoubleValue(powSta));

    // Ustawienie progu Energy Detection Threshold (CCA)
    phy.Set("CcaEdThreshold", DoubleValue(ccaEdTr));
    


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



    ssid = Ssid("ns3-80211be");

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid),"MaxMissedBeacons", UintegerValue(1200));
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
            "EnableBeaconJitter",
            BooleanValue(false),
            "Ssid",
            SsidValue(ssid));
    
    apDevice = wifi.Install(phy, mac, wifiApNode);

    ssid = Ssid("ns3-80211be-SLO");

    mac.SetEmlsrManager("ns3::DefaultEmlsrManager",
        "EmlsrLinkSet",
        StringValue(emlsrLinks),
        "MainPhyId",
        UintegerValue(idSLD),
        "EmlsrPaddingDelay",
        TimeValue(MicroSeconds(paddingDelayUsec)),
        "EmlsrTransitionDelay",
        TimeValue(MicroSeconds(transitionDelayUsec)),
        "SwitchAuxPhy",
        BooleanValue(false),
        "AuxPhyTxCapable",
        BooleanValue(false),
        "AuxPhyChannelWidth",
        UintegerValue(auxPhyChWidth));  


    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid),"MaxMissedBeacons", UintegerValue(1200));

    staDevices2 = wifi.Install(phy, mac, wifiStaNodes2);

    mac.SetType("ns3::ApWifiMac",
                "EnableBeaconJitter",
                BooleanValue(false),
                "Ssid",
                SsidValue(ssid));
    apDevice2 = wifi.Install(phy, mac, wifiApNode2);

    int64_t streamNumber = 392;
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);
    streamNumber += WifiHelper::AssignStreams(apDevice2, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices2, streamNumber);

    // Set guard interval and MPDU buffer size
    Config::Set(
        "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval",
        TimeValue(NanoSeconds(gi)));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MpduBufferSize",
                UintegerValue(mpduBufferSize));

    // mobility.
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, -20.0, 0.0));
    positionAlloc->Add(Vector(distance, -20.0, 0.0));

    positionAlloc->Add(Vector(0.0, 20.0, 0.0));
    positionAlloc->Add(Vector(distance, 20.0, 0.0));

    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);
    mobility.Install(wifiApNode2);
    mobility.Install(wifiStaNodes2);

    // Print position of each node
    std::cout << std::endl << "Node positions" << std::endl;

    //  - APs positions
    for (NodeContainer::Iterator j = wifiApNode.Begin (); j != wifiApNode.End (); ++j) {
        Ptr<Node> object = *j;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        Vector pos = position->GetPosition ();
        std::cout << "AP " << (uint32_t) (object->GetId()-1) << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }
    for (NodeContainer::Iterator j = wifiApNode2.Begin (); j != wifiApNode2.End (); ++j) {
        Ptr<Node> object = *j;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        Vector pos = position->GetPosition ();
        std::cout << "AP " << (uint32_t) (object->GetId()-2) << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }


    // - stations positions
    for (NodeContainer::Iterator j = wifiStaNodes.Begin (); j != wifiStaNodes.End (); ++j) {
        Ptr<Node> object = *j;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        Vector pos = position->GetPosition ();
        std::cout << "Sta " << (uint32_t) object->GetId() << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }
    for (NodeContainer::Iterator j = wifiStaNodes2.Begin (); j != wifiStaNodes2.End (); ++j) {
        Ptr<Node> object = *j;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        Vector pos = position->GetPosition ();
        std::cout << "Sta " << (uint32_t) object->GetId()-1 << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }

    /* Internet stack*/
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);
    stack.Install(wifiApNode2);
    stack.Install(wifiStaNodes2);

    streamNumber += stack.AssignStreams(wifiApNode, streamNumber);
    streamNumber += stack.AssignStreams(wifiStaNodes, streamNumber);
    streamNumber += stack.AssignStreams(wifiApNode2, streamNumber);
    streamNumber += stack.AssignStreams(wifiStaNodes2, streamNumber);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces;
    Ipv4InterfaceContainer apNodeInterface;
    staNodeInterfaces = address.Assign(staDevices);
    apNodeInterface = address.Assign(apDevice);

    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces2;
    Ipv4InterfaceContainer apNodeInterface2;
    staNodeInterfaces2 = address.Assign(staDevices2);
    apNodeInterface2 = address.Assign(apDevice2);

    std::cout << "AP IP: " << apNodeInterface.GetAddress(0) << std::endl;
    std::cout << "STA IP: " << staNodeInterfaces.GetAddress(0) << std::endl;

    std::cout << "AP2 IP: " << apNodeInterface2.GetAddress(0) << std::endl;
    std::cout << "STA2 IP: " << staNodeInterfaces2.GetAddress(0) << std::endl;



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

    const auto maxLoad = //dataRate*1e6;
    dataRate*1024*1024;

    if (udp)
    {
        // UDP flow
        uint16_t port = 9;
        UdpServerHelper server(port);
        serverApp = server.Install(serverNodes.get());
        streamNumber += server.AssignStreams(serverNodes.get(), streamNumber);

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
            streamNumber += client.AssignStreams(clientNodes.Get(i), streamNumber);

            clientApp.Start(Seconds(1.0));
            clientApp.Stop(simulationTime + Seconds(1.0));
        }
    }



    //Setting applications for second AP and STA
    ApplicationContainer serverApp2;
    auto serverNodes2 = downlink ? std::ref(wifiStaNodes2) : std::ref(wifiApNode2);
    Ipv4InterfaceContainer serverInterfaces2;
    NodeContainer clientNodes2;
    for (std::size_t i = 0; i < nStations; i++) // Single station for SLO
    {
        serverInterfaces2.Add(downlink ? staNodeInterfaces2.Get(i) : apNodeInterface2.Get(0));
        clientNodes2.Add(downlink ? wifiApNode2.Get(0) : wifiStaNodes2.Get(i));
    }

    if (udp)
    {
        // UDP flow
        uint16_t port2 = 10;
        UdpServerHelper server2(port2);
        serverApp2 = server2.Install(serverNodes2.get());
        streamNumber += server2.AssignStreams(serverNodes2.get(), streamNumber);

        serverApp2.Start(Seconds(0.0));
        serverApp2.Stop(simulationTime + Seconds(1.0));

        const auto packetInterval = 1528 * 8.0 / maxLoad;

        for (std::size_t i = 0; i < nStations; i++)
        {
            UdpClientHelper client2(serverInterfaces2.GetAddress(i), port2);
            client2.SetAttribute("MaxPackets", UintegerValue(4294967295U));
            client2.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
            client2.SetAttribute("PacketSize", UintegerValue(payloadSize));
            ApplicationContainer clientApp2 = client2.Install(clientNodes2.Get(i));
            streamNumber += client2.AssignStreams(clientNodes2.Get(i), streamNumber);

            clientApp2.Start(Seconds(1.0));
            clientApp2.Stop(simulationTime + Seconds(1.0));
        }
    }

    //phy.EnablePcap ("ap1.pcap", apDevice.Get (0));
    //phy.EnablePcap ("ap2.pcap", apDevice2.Get (0));
    //phy.EnablePcap ("sta1.pcap", staDevices.Get (0));
    //phy.EnablePcap ("sta2.pcap", staDevices2.Get (0));
    // LogComponentEnable("WifiPhy", LOG_LEVEL_DEBUG);

    // LogComponentEnable("WifiMac", LOG_LEVEL_DEBUG);
    // LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    // LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    // std::cout << "test" << std::endl;
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    // std::vector<uint64_t> cumulRxBytes(nStations, 0);

    Simulator::Stop(simulationTime + Seconds(1.0));
    Simulator::Run();


    Time Jitter;
    Time Delay;

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    // Show results     
    std::ofstream myfile;
    if (fileExists(csvPath)) {
        myfile.open (csvPath, std::ios::app); 
    }
    else {
        myfile.open (csvPath, std::ios::app);  
        myfile << "rngRun,flow,nStations,distance,linksNumber,loss,jitter,latency,throughput,rxBytes,txBytes,rxPackets,txPackets,timeFirstRxPacket,timeFirstTxPacket,timeLastRxPacket,timeLastTxPacket,jitterSum,delaySum,rateAdaptationManager,scenario" << std::endl;
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

        double throughput = (rxBytes > 0 && delaySum > 0) ? rxBytes*8/(timeLastRxPacket-timeFirstRxPacket)/1024/1024 : 0;  // Throughput in Mbit/s
        std::cout << "  Throughput: " << throughput << " Mbit/s" << std::endl;
        std::cout << "---------------------------------" << std::endl;

        if (flow.second.rxPackets > 0)
        {
            double avgPacketSize = (double)flow.second.rxBytes / flow.second.rxPackets;
            std::cout << "  Avg packet size: " << avgPacketSize << " bytes" << std::endl;
        }

        myfile << RngSeedManager::GetRun () << ',' << flow.first << ',' << nStations << ',' << distance << ',' << nLinks << ',' << lossRate << ',' << avgJitter<< ','<< avgLatency<< ',' << throughput<< ',' << rxBytes << ',' << txBytes << ',' << rxPackets << ',' << txPackets << ',' << timeFirstRxPacket << ',' << timeFirstTxPacket << ',' << timeLastRxPacket << ',' << timeLastTxPacket << ',' << jitterSum << ',' << delaySum << ',' << rateAdaptationManager << ',' << scenario<< std::endl;
                
    }

    Simulator::Destroy();
    myfile.close(); 

    return 0;
}
