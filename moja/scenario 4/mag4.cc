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

NS_LOG_COMPONENT_DEFINE("mag4");


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

    int dataRate1 = 107;
    int dataRate2 = 107;
    int dataRate3 = 107;
    int channelWidth1=20;
    int channelWidth2=20;
    int channelWidth3=20;
    double radius = 20.0;
    int MLOn = 0;
    int idSLD = 0;
    int nAP = 4;
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
    uint16_t auxPhyChWidth{320};
    bool auxPhyTxCapable{true};
    std::string csvPath = "results.csv";
    Time simulationTime{"120"};
    meter_u distance{1.0};
    double frequency{2.4};  // whether the first link operates in the 2.4, 5 or 6 GHz
    double frequency2{5}; // whether the second link operates in the 2.4, 5 or 6 GHz (0 means no
                          // second link exists)
    double frequency3{6}; // whether the third link operates in the 2.4, 5 or 6 GHz (0 means no third link exists)
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
    cmd.AddValue ("MLOn", "Number of BSS using MLO", MLOn);
    cmd.AddValue ("dataRate1", "Data rate for devices using 2.4 GHz band", dataRate1);
    cmd.AddValue ("dataRate2", "Data rate for devices using 5 GHz band", dataRate2);
    cmd.AddValue ("dataRate3", "Data rate for devices using 6 GHz band", dataRate3);
    cmd.AddValue("idSLD", "Identifier of the link used by the SLD", idSLD);
    cmd.AddValue ("channelWidth1", "2.4 GHz chennel width", channelWidth1);
    cmd.AddValue ("channelWidth2", "5 GHz chennel width", channelWidth2);
    cmd.AddValue ("channelWidth3", "6 GHz chennel width", channelWidth3);
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

    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("999999"));
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

    NodeContainer wifiStaNodes3;
    wifiStaNodes3.Create(nStations);
    NodeContainer wifiApNode3;
    wifiApNode3.Create(1);

    NodeContainer wifiStaNodes4;
    wifiStaNodes4.Create(nStations);
    NodeContainer wifiApNode4;
    wifiApNode4.Create(1);

    NetDeviceContainer apDevice;
    NetDeviceContainer staDevices;

    NetDeviceContainer apDevice2;
    NetDeviceContainer staDevices2;

    NetDeviceContainer apDevice3;
    NetDeviceContainer staDevices3;

    NetDeviceContainer apDevice4;
    NetDeviceContainer staDevices4;


    WifiMacHelper mac;
    WifiHelper wifi;

    wifi.SetStandard(WIFI_STANDARD_80211be);

    std::array<std::string, 3> channelStr;
    std::array<FrequencyRange, 3> freqRanges;
    int nLinks = 0;
    std::string dataModeStr = "EhtMcs" + std::to_string(mcs);
    std::string ctrlRateStr;
    uint64_t nonHtRefRateMbps = EhtPhy::GetNonHtReferenceRate(mcs) / 1e6;

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
            if (channelWidth3==20)
            {
                channelStr[nLinks] += "{0, 20, BAND_6GHZ, 0}";
            }
            else if (channelWidth3==40)
            {
                channelStr[nLinks] += "{0, 40, BAND_6GHZ, 0}";
            }
            else if (channelWidth3==80)
            {
                channelStr[nLinks] += "{0, 80, BAND_6GHZ, 0}"; 
            }
            else if (channelWidth3==160)
            {
                channelStr[nLinks] += "{0, 160, BAND_6GHZ, 0}";
            }
            else if (channelWidth3==320)
            {
                channelStr[nLinks] += "{0, 320, BAND_6GHZ, 0}";
            }
            

            freqRanges[nLinks] = WIFI_SPECTRUM_6_GHZ;
        }
        else if (freq == 5)
        {
            if (channelWidth2==20)
            {
                channelStr[nLinks] += "{0, 20, BAND_5GHZ, 0}";
            }
            else if (channelWidth2==40)
            {
                channelStr[nLinks] += "{0, 40, BAND_5GHZ, 0}";
            }
            else if (channelWidth2==80)
            {
                channelStr[nLinks] += "{0, 80, BAND_5GHZ, 0}"; 
            }
            else if (channelWidth2==160)
            {
                channelStr[nLinks] += "{0, 160, BAND_5GHZ, 0}";
            }

            freqRanges[nLinks] = WIFI_SPECTRUM_5_GHZ;
            ctrlRateStr = "OfdmRate" + std::to_string(nonHtRefRateMbps) + "Mbps";
            wifi.SetRemoteStationManager(nLinks,
                                            "ns3::ConstantRateWifiManager",
                                            "DataMode",
                                            StringValue(dataModeStr),
                                            "ControlMode",
                                            StringValue(ctrlRateStr));
        }
        else if (freq == 2.4)
        {
            ctrlRateStr = "ErpOfdmRate" + std::to_string(nonHtRefRateMbps) + "Mbps";
            wifi.SetRemoteStationManager(nLinks,
                                            "ns3::ConstantRateWifiManager",
                                            "DataMode",
                                            StringValue(dataModeStr),
                                            "ControlMode",
                                            StringValue(ctrlRateStr));
            if (channelWidth1==20)
            {
                channelStr[nLinks] += "{0, 20, BAND_2_4GHZ, 0}";
            }
            else if (channelWidth1==40)
            {
                channelStr[nLinks] += "{0, 40, BAND_2_4GHZ, 0}";
            }

            freqRanges[nLinks] = WIFI_SPECTRUM_2_4_GHZ;
        }
        else
        {
            NS_FATAL_ERROR("Wrong frequency value!");
        }


        nLinks++;
    }


    Ssid ssid;

    wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(true));
    SpectrumWifiPhyHelper phyB(2);
    SpectrumWifiPhyHelper phyD(3);

    phyB.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phyB.Set("ChannelSwitchDelay", TimeValue(MicroSeconds(channelSwitchDelayUsec)));
    // Ustawienie mocy stacji
    phyB.Set("TxPowerStart", DoubleValue(powSta));
    phyB.Set("TxPowerEnd", DoubleValue(powSta));

    // Ustawienie progu Energy Detection Threshold (CCA)
    phyB.Set("CcaEdThreshold", DoubleValue(ccaEdTr));

    phyD.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phyD.Set("ChannelSwitchDelay", TimeValue(MicroSeconds(channelSwitchDelayUsec)));
    // Ustawienie mocy stacji
    phyD.Set("TxPowerStart", DoubleValue(powSta));
    phyD.Set("TxPowerEnd", DoubleValue(powSta));

    // Ustawienie progu Energy Detection Threshold (CCA)
    phyD.Set("CcaEdThreshold", DoubleValue(ccaEdTr));

    for (uint8_t linkId = 0; linkId < nLinks; linkId++)
    {
        if (linkId != 2)
        {
            phyB.Set(linkId, "ChannelSettings", StringValue(channelStr[linkId]));
            phyD.Set(linkId, "ChannelSettings", StringValue(channelStr[linkId]));
        }
        if (linkId == 2)
        {
            phyD.Set(linkId, "ChannelSettings", StringValue(channelStr[linkId]));
        }
        auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
        auto lossModel = CreateObject<LogDistancePropagationLossModel>();
        if (freqRanges[linkId] == WIFI_SPECTRUM_2_4_GHZ)  lossModel->SetAttribute("ReferenceLoss", DoubleValue(40.0)); // dla 2.4 GHz
        else if (freqRanges[linkId] == WIFI_SPECTRUM_5_GHZ) lossModel->SetAttribute("ReferenceLoss", DoubleValue(46.6)); // dla 5 GHz
        else if (freqRanges[linkId] == WIFI_SPECTRUM_6_GHZ) lossModel->SetAttribute("ReferenceLoss", DoubleValue(48.0)); // dla 6 GHz
        spectrumChannel->AddPropagationLossModel(lossModel);

        if (linkId != 2)
        {
            phyB.AddChannel(spectrumChannel, freqRanges[linkId]);
            phyD.AddChannel(spectrumChannel, freqRanges[linkId]);
        }
        if (linkId == 2)
        {
            phyD.AddChannel(spectrumChannel, freqRanges[linkId]);
        }
    }


    mac.SetEmlsrManager("ns3::DefaultEmlsrManager",
        "EmlsrLinkSet",
        StringValue("0,1"),
        "MainPhyId",
        UintegerValue(0),
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

    ssid = Ssid("ns3-80211be-1");

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid),"MaxMissedBeacons", UintegerValue(1200));
    staDevices = wifi.Install(phyB, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
            "EnableBeaconJitter",
            BooleanValue(false),
            "Ssid",
            SsidValue(ssid));
    
    apDevice = wifi.Install(phyB, mac, wifiApNode);
    
    ssid = Ssid("ns3-80211be-2");

    mac.SetEmlsrManager("ns3::DefaultEmlsrManager",
        "EmlsrLinkSet",
        StringValue("0,1"),
        "MainPhyId",
        UintegerValue(0),
        "EmlsrPaddingDelay",
        TimeValue(MicroSeconds(paddingDelayUsec)),
        "EmlsrTransitionDelay",
        TimeValue(MicroSeconds(transitionDelayUsec)),
        "SwitchAuxPhy",
        BooleanValue(false),
        "AuxPhyTxCapable",
        BooleanValue(true),
        "AuxPhyChannelWidth",
        UintegerValue(auxPhyChWidth));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid),"MaxMissedBeacons", UintegerValue(1200));

    staDevices2 = wifi.Install(phyB, mac, wifiStaNodes2);

    mac.SetType("ns3::ApWifiMac",
                "EnableBeaconJitter",
                BooleanValue(false),
                "Ssid",
                SsidValue(ssid));
    apDevice2 = wifi.Install(phyB, mac, wifiApNode2);

    ssid = Ssid("ns3-80211be-3");
    mac.SetEmlsrManager("ns3::DefaultEmlsrManager",
        "EmlsrLinkSet",
        StringValue("0,1"),
        "MainPhyId",
        UintegerValue(1),
        "EmlsrPaddingDelay",
        TimeValue(MicroSeconds(paddingDelayUsec)),
        "EmlsrTransitionDelay",
        TimeValue(MicroSeconds(transitionDelayUsec)),
        "SwitchAuxPhy",
        BooleanValue(false),
        "AuxPhyTxCapable",
        BooleanValue(true),
        "AuxPhyChannelWidth",
        UintegerValue(auxPhyChWidth));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid),"MaxMissedBeacons", UintegerValue(1200));

    staDevices3 = wifi.Install(phyB, mac, wifiStaNodes3);

    mac.SetType("ns3::ApWifiMac",
                "EnableBeaconJitter",
                BooleanValue(false),
                "Ssid",
                SsidValue(ssid));
    apDevice3 = wifi.Install(phyB, mac, wifiApNode3);
    
    wifi.SetRemoteStationManager(2,
                                    "ns3::ConstantRateWifiManager",
                                    "DataMode",
                                    StringValue(dataModeStr),
                                    "ControlMode",
                                    StringValue(dataModeStr));

    ssid = Ssid("ns3-80211be-4");
    mac.SetEmlsrManager("ns3::DefaultEmlsrManager",
        "EmlsrLinkSet",
        StringValue("0,1,2"),
        "MainPhyId",
        UintegerValue(0),
        "EmlsrPaddingDelay",
        TimeValue(MicroSeconds(paddingDelayUsec)),
        "EmlsrTransitionDelay",
        TimeValue(MicroSeconds(transitionDelayUsec)),
        "SwitchAuxPhy",
        BooleanValue(false),
        "AuxPhyTxCapable",
        BooleanValue(true),
        "AuxPhyChannelWidth",
        UintegerValue(auxPhyChWidth));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid),"MaxMissedBeacons", UintegerValue(1200));

    staDevices4 = wifi.Install(phyD, mac, wifiStaNodes4);

    mac.SetType("ns3::ApWifiMac",
                "EnableBeaconJitter",
                BooleanValue(false),
                "Ssid",
                SsidValue(ssid));
    apDevice4 = wifi.Install(phyD, mac, wifiApNode4);

    int64_t streamNumber = 392;

    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    streamNumber += WifiHelper::AssignStreams(apDevice2, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices2, streamNumber);

    streamNumber += WifiHelper::AssignStreams(apDevice3, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices3, streamNumber);

    streamNumber += WifiHelper::AssignStreams(apDevice4, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices4, streamNumber);

    // Set guard interval and MPDU buffer size
    Config::Set(
        "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval",
        TimeValue(NanoSeconds(gi)));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MpduBufferSize",
                UintegerValue(mpduBufferSize));

    // mobility.
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    for (int i = 0; i < nAP; ++i) {
        double angle = 2 * M_PI * i / nAP;
        double x_ap = radius * std::cos(angle);
        double y_ap = radius * std::sin(angle);

        double x_sta = (radius + distance) * std::cos(angle);
        double y_sta = (radius + distance) * std::sin(angle);

        positionAlloc->Add(Vector(x_ap, y_ap, 0.0));
        positionAlloc->Add(Vector(x_sta, y_sta, 0.0));
    }

    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(wifiStaNodes);
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes2);
    mobility.Install(wifiApNode2);
    mobility.Install(wifiStaNodes3);
    mobility.Install(wifiApNode3);
    mobility.Install(wifiStaNodes4);
    mobility.Install(wifiApNode4);


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
    for (NodeContainer::Iterator j = wifiApNode3.Begin (); j != wifiApNode3.End (); ++j) {
        Ptr<Node> object = *j;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        Vector pos = position->GetPosition ();
        std::cout << "AP " << (uint32_t) (object->GetId()-3) << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }
    for (NodeContainer::Iterator j = wifiApNode4.Begin (); j != wifiApNode4.End (); ++j) {
        Ptr<Node> object = *j;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        Vector pos = position->GetPosition ();
        std::cout << "AP " << (uint32_t) (object->GetId()-4) << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
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
        for (NodeContainer::Iterator j = wifiStaNodes3.Begin (); j != wifiStaNodes3.End (); ++j) {
        Ptr<Node> object = *j;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        Vector pos = position->GetPosition ();
        std::cout << "Sta " << (uint32_t) object->GetId()-2 << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }
    for (NodeContainer::Iterator j = wifiStaNodes4.Begin (); j != wifiStaNodes4.End (); ++j) {
        Ptr<Node> object = *j;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        Vector pos = position->GetPosition ();
        std::cout << "Sta " << (uint32_t) object->GetId()-3 << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }

    /* Internet stack*/
    InternetStackHelper stack;

    // Instalacja stosu na AP i stacjach
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    stack.Install(wifiApNode2);
    stack.Install(wifiStaNodes2);

    stack.Install(wifiApNode3);
    stack.Install(wifiStaNodes3);

    stack.Install(wifiApNode4);
    stack.Install(wifiStaNodes4);


    // Przydział streamów
    streamNumber += stack.AssignStreams(wifiApNode, streamNumber);
    streamNumber += stack.AssignStreams(wifiStaNodes, streamNumber);

    streamNumber += stack.AssignStreams(wifiApNode2, streamNumber);
    streamNumber += stack.AssignStreams(wifiStaNodes2, streamNumber);

    streamNumber += stack.AssignStreams(wifiApNode3, streamNumber);
    streamNumber += stack.AssignStreams(wifiStaNodes3, streamNumber);

    streamNumber += stack.AssignStreams(wifiApNode4, streamNumber);
    streamNumber += stack.AssignStreams(wifiStaNodes4, streamNumber);

    // Adresacja IP i przypisanie do urządzeń

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

    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces3;
    Ipv4InterfaceContainer apNodeInterface3;
    staNodeInterfaces3 = address.Assign(staDevices3);
    apNodeInterface3 = address.Assign(apDevice3);

    address.SetBase("192.168.4.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces4;
    Ipv4InterfaceContainer apNodeInterface4;
    staNodeInterfaces4 = address.Assign(staDevices4);
    apNodeInterface4 = address.Assign(apDevice4);

    // Grupa 1
    ApplicationContainer serverApp;
    auto serverNodes = downlink ? std::ref(wifiStaNodes) : std::ref(wifiApNode);
    Ipv4InterfaceContainer serverInterfaces;
    NodeContainer clientNodes;
    for (std::size_t i = 0; i < nStations; i++)
    {
        serverInterfaces.Add(downlink ? staNodeInterfaces.Get(i) : apNodeInterface.Get(0));
        clientNodes.Add(downlink ? wifiApNode.Get(0) : wifiStaNodes.Get(i));
    }
    if (udp)
    {
        uint16_t port1 = 9;
        UdpServerHelper server1(port1);
        serverApp = server1.Install(serverNodes.get());
        streamNumber += server1.AssignStreams(serverNodes.get(), streamNumber);

        serverApp.Start(Seconds(0.0));
        serverApp.Stop(simulationTime + Seconds(1.0));

        const auto packetInterval = 1528 * 8.0 / (dataRate1 * 1024 * 1024);

        for (std::size_t i = 0; i < nStations; i++)
        {
            UdpClientHelper client1(serverInterfaces.GetAddress(i), port1);
            client1.SetAttribute("MaxPackets", UintegerValue(4294967295U));
            client1.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
            client1.SetAttribute("PacketSize", UintegerValue(payloadSize));
            ApplicationContainer clientApp1 = client1.Install(clientNodes.Get(i));
            streamNumber += client1.AssignStreams(clientNodes.Get(i), streamNumber);

            clientApp1.Start(Seconds(1.0));
            clientApp1.Stop(simulationTime + Seconds(1.0));
        }
    }

    // Grupa 2
    ApplicationContainer serverApp2;
    auto serverNodes2 = downlink ? std::ref(wifiStaNodes2) : std::ref(wifiApNode2);
    Ipv4InterfaceContainer serverInterfaces2;
    NodeContainer clientNodes2;
    for (std::size_t i = 0; i < nStations; i++)
    {
        serverInterfaces2.Add(downlink ? staNodeInterfaces2.Get(i) : apNodeInterface2.Get(0));
        clientNodes2.Add(downlink ? wifiApNode2.Get(0) : wifiStaNodes2.Get(i));
    }
    if (udp)
    {
        uint16_t port2 = 10;
        UdpServerHelper server2(port2);
        serverApp2 = server2.Install(serverNodes2.get());
        streamNumber += server2.AssignStreams(serverNodes2.get(), streamNumber);

        serverApp2.Start(Seconds(0.0));
        serverApp2.Stop(simulationTime + Seconds(1.0));

        const auto packetInterval = 1528 * 8.0 / (dataRate2 * 1024 * 1024);

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

    // Grupa 3

    ApplicationContainer serverApp3;
    auto serverNodes3 = downlink ? std::ref(wifiStaNodes3) : std::ref(wifiApNode3);
    Ipv4InterfaceContainer serverInterfaces3;
    NodeContainer clientNodes3;
    for (std::size_t i = 0; i < nStations; i++)
    {
        serverInterfaces3.Add(downlink ? staNodeInterfaces3.Get(i) : apNodeInterface3.Get(0));
        clientNodes3.Add(downlink ? wifiApNode3.Get(0) : wifiStaNodes3.Get(i));
    }
    if (udp)
    {
        uint16_t port3 = 11;
        UdpServerHelper server3(port3);
        serverApp3 = server3.Install(serverNodes3.get());
        streamNumber += server3.AssignStreams(serverNodes3.get(), streamNumber);

        serverApp3.Start(Seconds(0.0));
        serverApp3.Stop(simulationTime + Seconds(1.0));

        const auto packetInterval = 1528 * 8.0 / (dataRate2 * 1024 * 1024);

        for (std::size_t i = 0; i < nStations; i++)
        {
            UdpClientHelper client3(serverInterfaces3.GetAddress(i), port3);
            client3.SetAttribute("MaxPackets", UintegerValue(4294967295U));
            client3.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
            client3.SetAttribute("PacketSize", UintegerValue(payloadSize));
            ApplicationContainer clientApp3 = client3.Install(clientNodes3.Get(i));
            streamNumber += client3.AssignStreams(clientNodes3.Get(i), streamNumber);

            clientApp3.Start(Seconds(1.0));
            clientApp3.Stop(simulationTime + Seconds(1.0));
        }
    }

    // Grupa 4
    ApplicationContainer serverApp4;
    auto serverNodes4 = downlink ? std::ref(wifiStaNodes4) : std::ref(wifiApNode4);
    Ipv4InterfaceContainer serverInterfaces4;
    NodeContainer clientNodes4;
    for (std::size_t i = 0; i < nStations; i++)
    {
        serverInterfaces4.Add(downlink ? staNodeInterfaces4.Get(i) : apNodeInterface4.Get(0));
        clientNodes4.Add(downlink ? wifiApNode4.Get(0) : wifiStaNodes4.Get(i));
    }
    if (udp)
    {
        uint16_t port4 = 12;
        UdpServerHelper server4(port4);
        serverApp4 = server4.Install(serverNodes4.get());
        streamNumber += server4.AssignStreams(serverNodes4.get(), streamNumber);

        serverApp4.Start(Seconds(0.0));
        serverApp4.Stop(simulationTime + Seconds(1.0));

        const auto packetInterval = 1528 * 8.0 / (dataRate3 * 1024 * 1024);

        for (std::size_t i = 0; i < nStations; i++)
        {
            UdpClientHelper client4(serverInterfaces4.GetAddress(i), port4);
            client4.SetAttribute("MaxPackets", UintegerValue(4294967295U));
            client4.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
            client4.SetAttribute("PacketSize", UintegerValue(payloadSize));
            ApplicationContainer clientApp4 = client4.Install(clientNodes4.Get(i));
            streamNumber += client4.AssignStreams(clientNodes4.Get(i), streamNumber);

            clientApp4.Start(Seconds(1.0));
            clientApp4.Stop(simulationTime + Seconds(1.0));
        }
    }

    // std::cout << "test" << std::endl;
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    phyB.EnablePcap ("ap3.pcap", apDevice3.Get (0));
    phyD.EnablePcap ("ap4.pcap", apDevice4.Get (0));
    phyB.EnablePcap ("sta3.pcap", staDevices3.Get (0));
    phyD.EnablePcap ("sta4.pcap", staDevices4.Get (0));
    
    Simulator::Stop(simulationTime + Seconds(1.0));
    Simulator::Run();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

    // Show results     
    std::ofstream myfile;
    if (fileExists(csvPath)) {
        myfile.open (csvPath, std::ios::app); 
    }
    else {
        myfile.open (csvPath, std::ios::app);  
        myfile << "rngRun,flow,nStations,distance,linksNumber,loss,jitter,latency,throughput,rxBytes,txBytes,rxPackets,txPackets,timeFirstRxPacket,timeFirstTxPacket,timeLastRxPacket,timeLastTxPacket,jitterSum,delaySum,rateAdaptationManager,scenario,MLOn" << std::endl;
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

        myfile << RngSeedManager::GetRun () << ',' << flow.first << ',' << nStations << ',' << distance << ',' << nLinks << ',' << lossRate << ',' << avgJitter<< ','<< avgLatency<< ',' << throughput<< ',' << rxBytes << ',' << txBytes << ',' << rxPackets << ',' << txPackets << ',' << timeFirstRxPacket << ',' << timeFirstTxPacket << ',' << timeLastRxPacket << ',' << timeLastTxPacket << ',' << jitterSum << ',' << delaySum << ',' << rateAdaptationManager << ',' << scenario << ',' << MLOn << std::endl;
                
    }

    Simulator::Destroy();
    myfile.close(); 

    return 0;
}
