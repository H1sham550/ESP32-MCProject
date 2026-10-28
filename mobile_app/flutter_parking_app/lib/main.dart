import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

void main() {
  runApp(const SmartParkingApp());
}

class SmartParkingApp extends StatelessWidget {
  const SmartParkingApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Smart Parking Monitor',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        colorSchemeSeed: Colors.indigo,
        brightness: Brightness.dark,
        scaffoldBackgroundColor: const Color(0xFF0F172A),
      ),
      home: const ParkingDashboardScreen(),
    );
  }
}

class ParkingDashboardScreen extends StatefulWidget {
  const ParkingDashboardScreen({super.key});

  @override
  State<ParkingDashboardScreen> createState() => _ParkingDashboardScreenState();
}

class _ParkingDashboardScreenState extends State<ParkingDashboardScreen> {
  // Configurable Target ESP32 IP Address
  String espIp = "192.168.43.50";

  // Parking Slot States
  String slot1Status = "UNKNOWN";
  String slot2Status = "UNKNOWN";
  int availableSlots = 0;

  // Connection Tracking
  bool isConnected = false;
  bool isPolling = false;
  String lastError = "";
  DateTime? lastUpdated;

  Timer? _pollingTimer;

  @override
  void initState() {
    super.initState();
    startPolling();
  }

  @override
  void dispose() {
    _pollingTimer?.cancel();
    super.dispose();
  }

  void startPolling() {
    _pollingTimer?.cancel();
    // Poll every 800ms for responsive state updates
    _pollingTimer = Timer.periodic(const Duration(milliseconds: 800), (_) {
      fetchSlotData();
    });
    fetchSlotData();
  }

  Future<void> fetchSlotData() async {
    if (isPolling) return; // Prevent concurrent requests
    setState(() => isPolling = true);

    final url = Uri.parse("http://$espIp/slots");

    try {
      final response = await http.get(url).timeout(const Duration(milliseconds: 1500));

      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        setState(() {
          slot1Status = data["slot1"] ?? "UNKNOWN";
          slot2Status = data["slot2"] ?? "UNKNOWN";
          availableSlots = data["available"] ?? 0;
          isConnected = true;
          lastError = "";
          lastUpdated = DateTime.now();
        });
      } else {
        setState(() {
          isConnected = false;
          lastError = "Server Error: ${response.statusCode}";
        });
      }
    } catch (e) {
      setState(() {
        isConnected = false;
        lastError = "Unable to reach ESP32 ($espIp)";
      });
    } finally {
      if (mounted) {
        setState(() => isPolling = false);
      }
    }
  }

  void _showSettingsDialog() {
    final textController = TextEditingController(text: espIp);
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: const Color(0xFF1E293B),
        title: const Text("ESP32 Connection Settings"),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              "Enter the IP address of your ESP32 board:",
              style: TextStyle(color: Colors.white70, fontSize: 13),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: textController,
              keyboardType: TextInputType.url,
              decoration: InputDecoration(
                filled: true,
                fillColor: const Color(0xFF0F172A),
                hintText: "192.168.43.50",
                prefixIcon: const Icon(Icons.wifi_tethering),
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text("Cancel"),
          ),
          ElevatedButton(
            onPressed: () {
              final newIp = textController.text.trim();
              if (newIp.isNotEmpty) {
                setState(() {
                  espIp = newIp;
                  isConnected = false;
                });
                startPolling();
              }
              Navigator.pop(ctx);
            },
            child: const Text("Save & Reconnect"),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          "🅿️ Smart Parking",
          style: TextStyle(fontWeight: FontWeight.bold),
        ),
        centerTitle: true,
        backgroundColor: const Color(0xFF1E293B),
        elevation: 0,
        actions: [
          IconButton(
            icon: const Icon(Icons.settings),
            tooltip: "Settings",
            onPressed: _showSettingsDialog,
          ),
        ],
      ),
      body: RefreshIndicator(
        onRefresh: fetchSlotData,
        child: SingleChildScrollView(
          physics: const AlwaysScrollableScrollPhysics(),
          padding: const EdgeInsets.all(20),
          child: Column(
            children: [
            // Connection Status Banner
            _buildConnectionBanner(),

            const SizedBox(height: 20),

            // Live Available Slots Summary Counter
            _buildAvailableCounter(),

            const SizedBox(height: 24),

            // Slots Grid
            Row(
              children: [
                Expanded(
                  child: _buildSlotCard("Slot 1", slot1Status),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: _buildSlotCard("Slot 2", slot2Status),
                ),
              ],
            ),

            const SizedBox(height: 32),

            // Hardware & Network Telemetry Card
            _buildTelemetryCard(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildConnectionBanner() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      decoration: BoxDecoration(
        color: isConnected
            ? const Color(0xFF064E3B)
            : const Color(0xFF450A0A),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(
          color: isConnected ? Colors.greenAccent : Colors.redAccent,
          width: 1,
        ),
      ),
      child: Row(
        children: [
          Icon(
            isConnected ? Icons.wifi : Icons.wifi_off,
            color: isConnected ? Colors.greenAccent : Colors.redAccent,
            size: 20,
          ),
          const SizedBox(width: 10),
          Expanded(
            child: Text(
              isConnected
                  ? "Connected to ESP32 at $espIp"
                  : lastError.isNotEmpty
                      ? lastError
                      : "Connecting to $espIp...",
              style: TextStyle(
                color: isConnected ? Colors.greenAccent : Colors.redAccent,
                fontWeight: FontWeight.w600,
                fontSize: 13,
              ),
            ),
          ),
          if (!isConnected)
            IconButton(
              icon: const Icon(Icons.refresh, size: 20, color: Colors.white70),
              onPressed: fetchSlotData,
              padding: EdgeInsets.zero,
              constraints: const BoxConstraints(),
            ),
        ],
      ),
    );
  }

  Widget _buildAvailableCounter() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.symmetric(vertical: 24, horizontal: 20),
      decoration: BoxDecoration(
        gradient: const LinearGradient(
          colors: [
            Color(0xFF1E293B),
            Color(0xFF334155),
          ],
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
        ),
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withValues(alpha: 0.3),
            blurRadius: 10,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: Column(
        children: [
          Text(
            isConnected ? "$availableSlots" : "-",
            style: TextStyle(
              fontSize: 64,
              fontWeight: FontWeight.w900,
              color: availableSlots > 0 ? const Color(0xFF22C55E) : const Color(0xFFEF4444),
            ),
          ),
          const SizedBox(height: 4),
          const Text(
            "SLOTS AVAILABLE",
            style: TextStyle(
              letterSpacing: 2.0,
              fontWeight: FontWeight.bold,
              fontSize: 13,
              color: Colors.white70,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            availableSlots == 2
                ? "All slots are free"
                : availableSlots == 1
                    ? "1 slot occupied, 1 slot free"
                    : "Parking lot is full",
            style: const TextStyle(color: Colors.white38, fontSize: 12),
          ),
        ],
      ),
    );
  }

  Widget _buildSlotCard(String slotName, String status) {
    final bool isFree = (status == "FREE");
    final bool isOccupied = (status == "OCCUPIED");
    final Color cardColor = isFree
        ? const Color(0xFF14532D)
        : isOccupied
            ? const Color(0xFF7F1D1D)
            : const Color(0xFF1E293B);

    final Color accentColor = isFree
        ? const Color(0xFF22C55E)
        : isOccupied
            ? const Color(0xFFEF4444)
            : Colors.grey;

    return AnimatedContainer(
      duration: const Duration(milliseconds: 300),
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: cardColor.withValues(alpha: 0.35),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: accentColor, width: 2),
      ),
      child: Column(
        children: [
          Icon(
            isFree
                ? Icons.check_circle_outline
                : isOccupied
                    ? Icons.directions_car_filled
                    : Icons.help_outline,
            size: 48,
            color: accentColor,
          ),
          const SizedBox(height: 12),
          Text(
            slotName,
            style: const TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 10),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 6),
            decoration: BoxDecoration(
              color: accentColor,
              borderRadius: BorderRadius.circular(20),
            ),
            child: Text(
              status,
              style: TextStyle(
                fontWeight: FontWeight.w900,
                fontSize: 12,
                color: isFree
                    ? const Color(0xFF052E16)
                    : isOccupied
                        ? Colors.white
                        : Colors.white,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTelemetryCard() {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xFF1E293B),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white12),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Row(
            children: [
              Icon(Icons.info_outline, size: 18, color: Colors.blueAccent),
              SizedBox(width: 8),
              Text(
                "System & Node Info",
                style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
              ),
            ],
          ),
          const Divider(color: Colors.white12, height: 20),
          _infoRow("Microcontroller", "ESP32 NodeMCU (30-pin)"),
          _infoRow("Target Endpoint", "http://$espIp/slots"),
          _infoRow("Polling Frequency", "800 ms"),
          _infoRow(
            "Last Update",
            lastUpdated != null
                ? "${lastUpdated!.hour.toString().padLeft(2, '0')}:${lastUpdated!.minute.toString().padLeft(2, '0')}:${lastUpdated!.second.toString().padLeft(2, '0')}"
                : "None",
          ),
        ],
      ),
    );
  }

  Widget _infoRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(color: Colors.white54, fontSize: 12)),
          Text(value, style: const TextStyle(color: Colors.white, fontSize: 12, fontWeight: FontWeight.w500)),
        ],
      ),
    );
  }
}
