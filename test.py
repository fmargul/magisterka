import pyshark
from datetime import datetime, timedelta

# Funkcja do sprawdzenia, czy czas pakietu mieści się w zadanym przedziale (14-16 sekund)
def is_in_time_range(sniff_time, start_time, end_time):
    return start_time <= sniff_time <= end_time

# Określ przedział czasowy od 14 do 16 sekund
start_time = datetime(1970, 1, 1) + timedelta(seconds=14)
end_time = datetime(1970, 1, 1) + timedelta(seconds=16)

# Ustawienie ścieżki do tshark, jeśli jest zainstalowany w niestandardowej lokalizacji
pyshark.tshark.tshark_path = r'C:\Program Files\Wireshark\tshark.exe'  # Zmienna z odpowiednią ścieżką

# Wczytanie pliku .pcap
cap = pyshark.FileCapture('ap1.pcap-1-0-0.pcap')

# Przejdź przez pakiety i filtruj na podstawie czasu
for packet in cap:
    # Sprawdź czas przechwycenia pakietu
    sniff_time = packet.sniff_time
    
    # Jeśli czas pakietu mieści się w przedziale 14-16s, analizujemy pakiet
    if is_in_time_range(sniff_time, start_time, end_time):
        print(f"Packet number: {packet.number}")
        print(f"Timestamp: {sniff_time}")
        
        # Jeśli jest to pakiet UDP
        if 'UDP' in packet:
            print(f"Source IP: {packet.ip.src}")
            print(f"Destination IP: {packet.ip.dst}")
            print(f"Source Port: {packet.udp.srcport}")
            print(f"Destination Port: {packet.udp.dstport}")
            print(f"UDP Length: {packet.length}")
            print(f"UDP Data: {packet.udp.payload}")
        
        print("-" * 50)