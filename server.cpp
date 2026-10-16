#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <memory>
#include <algorithm>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// ============================================================================
// 1. MINIMAL SELF-CONTAINED JSON PARSER & SERIALIZER (ZERO EXTERNAL LIBS)
// ============================================================================
namespace MiniJson {
    inline std::string extractString(const std::string& json, const std::string& key, const std::string& defVal = "") {
        std::string pattern = "\"" + key + "\"";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return defVal;
        size_t colon = json.find(':', pos);
        if (colon == std::string::npos) return defVal;
        size_t startQuote = json.find('\"', colon);
        if (startQuote == std::string::npos) return defVal;
        size_t endQuote = json.find('\"', startQuote + 1);
        if (endQuote == std::string::npos) return defVal;
        return json.substr(startQuote + 1, endQuote - startQuote - 1);
    }

    inline double extractDouble(const std::string& json, const std::string& key, double defVal = 0.0) {
        std::string pattern = "\"" + key + "\"";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return defVal;
        size_t colon = json.find(':', pos);
        if (colon == std::string::npos) return defVal;
        size_t valStart = json.find_first_of("-0123456789.", colon + 1);
        if (valStart == std::string::npos) return defVal;
        size_t valEnd = json.find_first_not_of("-0123456789.", valStart);
        std::string s = json.substr(valStart, (valEnd == std::string::npos ? json.length() : valEnd) - valStart);
        try { return std::stod(s); } catch (...) { return defVal; }
    }

    inline int extractInt(const std::string& json, const std::string& key, int defVal = 0) {
        return static_cast<int>(extractDouble(json, key, static_cast<double>(defVal)));
    }

    inline std::vector<double> extractDoubleArray(const std::string& json, const std::string& key) {
        std::vector<double> result;
        std::string pattern = "\"" + key + "\"";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return result;
        size_t startBracket = json.find('[', pos);
        size_t endBracket = json.find(']', startBracket);
        if (startBracket == std::string::npos || endBracket == std::string::npos) return result;

        std::string content = json.substr(startBracket + 1, endBracket - startBracket - 1);
        std::stringstream ss(content);
        std::string item;
        while (std::getline(ss, item, ',')) {
            size_t valStart = item.find_first_of("-0123456789.");
            if (valStart != std::string::npos) {
                try { result.push_back(std::stod(item.substr(valStart))); } catch (...) {}
            }
        }
        return result;
    }
}

// ============================================================================
// 2. OOP DOMAIN MODELS & ENCAPSULATION
// ============================================================================
struct DrivingConditions {
    double speedKmh;
    int engagedGear;
    int passengers;
    double cargoWeightKg;
    double gradientPct;
    std::string roadType;

    double getTotalPayloadMass() const {
        return (passengers * 75.0) + cargoWeightKg;
    }
};

struct ResistanceForces {
    double aerodynamic;
    double rolling;
    double gradient;
    double total;
};

struct GearCandidate {
    int gearNumber;
    int engineRpm;
    double fuelRateL100km;
    std::string status;       // "feasible", "lugging", "overrev"
    std::string suitability;  // Descriptive text
};

struct OptimizationResult {
    int currentRpm;
    double currentFuelL100km;
    int optimalGear;
    double optimalFuelL100km;
    std::string adviceType;   // "maintain", "upshift", "downshift"
    std::string explanation;
    ResistanceForces resistance;
    std::vector<GearCandidate> candidates;
};

// ============================================================================
// 3. ENCAPSULATED TRANSMISSION COMPONENT
// ============================================================================
class Transmission {
private:
    std::vector<double> gearRatios_;
    double finalDriveRatio_;

public:
    Transmission(std::vector<double> ratios, double finalDrive)
        : gearRatios_(std::move(ratios)), finalDriveRatio_(finalDrive) {}

    size_t getGearCount() const { return gearRatios_.size(); }

    double getRatio(int gearNumber) const {
        if (gearNumber < 1 || gearNumber > static_cast<int>(gearRatios_.size())) return 1.0;
        return gearRatios_[gearNumber - 1];
    }

    int computeRpm(double speedKmh, int gearNumber, double wheelRadiusMeters) const {
        if (speedKmh <= 0.0) return 800; // Engine idle floor
        double speedMs = speedKmh / 3.6;
        double wheelOmega = speedMs / wheelRadiusMeters;
        double engineOmega = wheelOmega * getRatio(gearNumber) * finalDriveRatio_;
        return static_cast<int>(std::round((engineOmega * 60.0) / (2.0 * M_PI)));
    }
};

// ============================================================================
// 4. ABSTRACT VEHICLE BASE CLASS & RUNTIME POLYMORPHISM
// ============================================================================
class Vehicle {
protected:
    std::string name_;
    double curbWeightKg_;
    double dragCoefficient_;
    double frontalAreaSqm_;
    double wheelRadiusMeters_;
    Transmission transmission_;
    int luggingRpmLimit_;
    int ecoRpmMin_;
    int ecoRpmMax_;
    int redlineRpm_;
    double baseBsfc_;

public:
    Vehicle(std::string name, double curbWeight, double cd, double frontalArea,
            double wheelRadius, Transmission transmission, int lugRpm,
            int ecoMin, int ecoMax, int redline, double bsfc)
        : name_(std::move(name)), curbWeightKg_(curbWeight), dragCoefficient_(cd),
          frontalAreaSqm_(frontalArea), wheelRadiusMeters_(wheelRadius),
          transmission_(std::move(transmission)), luggingRpmLimit_(lugRpm),
          ecoRpmMin_(ecoMin), ecoRpmMax_(ecoMax), redlineRpm_(redline),
          baseBsfc_(bsfc) {}

    virtual ~Vehicle() = default;

    // Getters
    const std::string& getName() const { return name_; }
    double getCurbWeight() const { return curbWeightKg_; }
    double getCd() const { return dragCoefficient_; }
    double getFrontalArea() const { return frontalAreaSqm_; }
    double getWheelRadius() const { return wheelRadiusMeters_; }
    const Transmission& getTransmission() const { return transmission_; }
    int getLuggingRpm() const { return luggingRpmLimit_; }
    int getEcoRpmMin() const { return ecoRpmMin_; }
    int getEcoRpmMax() const { return ecoRpmMax_; }
    int getRedlineRpm() const { return redlineRpm_; }
    double getBaseBsfc() const { return baseBsfc_; }

    // Pure Virtual Interfaces
    virtual double calculateBSFCMultiplier(int rpm) const = 0;
    virtual double getRollingCoefficient(const std::string& roadType) const = 0;
};

// Concrete Class: Passenger Car
class PassengerCar : public Vehicle {
public:
    using Vehicle::Vehicle;

    double getRollingCoefficient(const std::string& roadType) const override {
        if (roadType == "city") return 0.015;
        if (roadType == "rough") return 0.024;
        return 0.011;
    }

    double calculateBSFCMultiplier(int rpm) const override {
        if (rpm < luggingRpmLimit_) return 1.40;
        if (rpm <= ecoRpmMax_) return 1.00;
        double excessRatio = static_cast<double>(rpm - ecoRpmMax_) / (redlineRpm_ - ecoRpmMax_);
        return 1.0 + std::pow(excessRatio, 1.8) * 0.65;
    }
};

// Concrete Class: Commercial Heavy Truck
class CommercialTruck : public Vehicle {
public:
    using Vehicle::Vehicle;

    double getRollingCoefficient(const std::string& roadType) const override {
        if (roadType == "city") return 0.018;
        if (roadType == "rough") return 0.032;
        return 0.013;
    }

    double calculateBSFCMultiplier(int rpm) const override {
        if (rpm < luggingRpmLimit_) return 1.65;
        if (rpm <= ecoRpmMax_) return 0.95;
        double excessRatio = static_cast<double>(rpm - ecoRpmMax_) / (redlineRpm_ - ecoRpmMax_);
        return 1.0 + std::pow(excessRatio, 2.0) * 0.85;
    }
};

// Concrete Class: Motorcycle
class Motorcycle : public Vehicle {
public:
    using Vehicle::Vehicle;

    double getRollingCoefficient(const std::string& roadType) const override {
        if (roadType == "city") return 0.016;
        if (roadType == "rough") return 0.028;
        return 0.012;
    }

    double calculateBSFCMultiplier(int rpm) const override {
        if (rpm < luggingRpmLimit_) return 1.50;
        if (rpm <= ecoRpmMax_) return 1.00;
        double excessRatio = static_cast<double>(rpm - ecoRpmMax_) / (redlineRpm_ - ecoRpmMax_);
        return 1.0 + std::pow(excessRatio, 1.5) * 0.55;
    }
};

// ============================================================================
// 5. VEHICLE FACTORY PATTERN
// ============================================================================
class VehicleFactory {
public:
    static std::unique_ptr<Vehicle> createFromRawJson(const std::string& body) {
        std::string name = MiniJson::extractString(body, "name", "Custom Vehicle");
        double weight = MiniJson::extractDouble(body, "curbWeight", 1050.0);
        double cd = MiniJson::extractDouble(body, "cd", 0.31);
        double area = MiniJson::extractDouble(body, "frontalArea", 2.15);
        double radius = MiniJson::extractDouble(body, "wheelRadius", 0.305);
        double finalDrive = MiniJson::extractDouble(body, "finalDrive", 4.05);
        std::vector<double> ratios = MiniJson::extractDoubleArray(body, "gearRatios");
        if (ratios.empty()) ratios = {3.55, 1.95, 1.30, 0.89, 0.71};

        int lug = MiniJson::extractInt(body, "rpmLug", 1200);
        int ecoMin = MiniJson::extractInt(body, "rpmEcoMin", 1700);
        int ecoMax = MiniJson::extractInt(body, "rpmEcoMax", 2400);
        int redline = MiniJson::extractInt(body, "rpmRedline", 6200);
        double bsfc = MiniJson::extractDouble(body, "baseBSFC", 245.0);

        Transmission trans(ratios, finalDrive);

        if (weight > 3000.0) {
            return std::make_unique<CommercialTruck>(name, weight, cd, area, radius, trans, lug, ecoMin, ecoMax, redline, bsfc);
        } else if (weight < 350.0) {
            return std::make_unique<Motorcycle>(name, weight, cd, area, radius, trans, lug, ecoMin, ecoMax, redline, bsfc);
        } else {
            return std::make_unique<PassengerCar>(name, weight, cd, area, radius, trans, lug, ecoMin, ecoMax, redline, bsfc);
        }
    }
};

// ============================================================================
// 6. GEAR OPTIMIZATION SERVICE (BUSINESS LOGIC)
// ============================================================================
class GearOptimizationService {
public:
    OptimizationResult optimize(const Vehicle& vehicle, const DrivingConditions& cond) const {
        double vMs = cond.speedKmh / 3.6;
        const double g = 9.81;
        const double airDensity = 1.225;

        double totalMass = vehicle.getCurbWeight() + cond.getTotalPayloadMass();
        double crr = vehicle.getRollingCoefficient(cond.roadType);
        double theta = std::atan(cond.gradientPct / 100.0);

        double fRolling = totalMass * g * crr * std::cos(theta);
        double fAero = 0.5 * airDensity * vehicle.getCd() * vehicle.getFrontalArea() * std::pow(vMs, 2);
        double fGrade = totalMass * g * std::sin(theta);

        ResistanceForces forces;
        forces.aerodynamic = std::max(0.0, fAero);
        forces.rolling = std::max(0.0, fRolling);
        forces.gradient = fGrade;
        forces.total = std::max(0.0, fRolling + fAero + fGrade);

        const Transmission& trans = vehicle.getTransmission();
        int currentRpm = trans.computeRpm(cond.speedKmh, cond.engagedGear, vehicle.getWheelRadius());
        double currentFuel = calculateFuel(vehicle, cond.speedKmh, forces.total, currentRpm);

        std::vector<GearCandidate> candidates;
        int optimalGear = cond.engagedGear;
        double lowestCost = 1e9;

        for (size_t i = 1; i <= trans.getGearCount(); ++i) {
            int gNum = static_cast<int>(i);
            int rpm = trans.computeRpm(cond.speedKmh, gNum, vehicle.getWheelRadius());
            double fuel = calculateFuel(vehicle, cond.speedKmh, forces.total, rpm);

            std::string status = "feasible";
            std::string suitability = "Acceptable";

            if (rpm < vehicle.getLuggingRpm()) {
                status = "lugging";
                suitability = "Unsafe (Lugging Risk)";
            } else if (rpm > vehicle.getRedlineRpm()) {
                status = "overrev";
                suitability = "Unsafe (Engine Redline)";
            } else if (rpm >= vehicle.getEcoRpmMin() && rpm <= vehicle.getEcoRpmMax()) {
                suitability = "Optimal Eco Window";
            }

            double cost = fuel;
            if (status == "lugging") cost += 15.0;
            if (status == "overrev") cost += 50.0;
            if (gNum == cond.engagedGear) cost -= 0.15; // Shift hysteresis

            if (cost < lowestCost) {
                lowestCost = cost;
                optimalGear = gNum;
            }

            candidates.push_back({gNum, rpm, fuel, status, suitability});
        }

        std::string adviceType = "maintain";
        std::string explanation = "Current gear operates smoothly inside the vehicle's safe operating window.";

        if (optimalGear > cond.engagedGear) {
            adviceType = "upshift";
            explanation = "Shifting up to Gear " + std::to_string(optimalGear) +
                          " drops engine speed into the eco band (" + std::to_string(vehicle.getEcoRpmMin()) +
                          "-" + std::to_string(vehicle.getEcoRpmMax()) + " RPM), mitigating throttling loss.";
        } else if (optimalGear < cond.engagedGear) {
            adviceType = "downshift";
            if (currentRpm < vehicle.getLuggingRpm()) {
                explanation = "Engine speed is below the safe threshold of " + std::to_string(vehicle.getLuggingRpm()) +
                              " RPM. Downshift immediately to prevent engine lugging.";
            } else {
                explanation = "Higher wheel torque is required for the gradient and cargo payload. Downshifting secures essential torque margin.";
            }
        }

        int optimalRpm = trans.computeRpm(cond.speedKmh, optimalGear, vehicle.getWheelRadius());
        double optimalFuel = calculateFuel(vehicle, cond.speedKmh, forces.total, optimalRpm);

        return {currentRpm, currentFuel, optimalGear, optimalFuel, adviceType, explanation, forces, candidates};
    }

private:
    double calculateFuel(const Vehicle& vehicle, double speedKmh, double dragN, int rpm) const {
        if (speedKmh <= 1.5) return 0.8;
        double speedMs = speedKmh / 3.6;
        double powerKw = (dragN * speedMs) / (1000.0 * 0.88);
        double bsfcMultiplier = vehicle.calculateBSFCMultiplier(rpm);
        double fuelGramsPerHour = powerKw * vehicle.getBaseBsfc() * bsfcMultiplier;
        double fuelLitersPerHour = fuelGramsPerHour / 745.0;
        return std::max(1.8, (fuelLitersPerHour / speedKmh) * 100.0);
    }
};

// ============================================================================
// 7. LIGHTWEIGHT NATIVE WINSOCK HTTP SERVER
// ============================================================================
std::string buildResponseJson(const OptimizationResult& res) {
    std::ostringstream ss;
    ss << "{"
       << "\"currentRpm\":" << res.currentRpm << ","
       << "\"currentL100km\":" << res.currentFuelL100km << ","
       << "\"optimalGear\":" << res.optimalGear << ","
       << "\"optimalL100km\":" << res.optimalFuelL100km << ","
       << "\"adviceType\":\"" << res.adviceType << "\","
       << "\"explanation\":\"" << res.explanation << "\","
       << "\"drag\":{"
       << "\"total\":" << res.resistance.total << ","
       << "\"aero\":" << res.resistance.aerodynamic << ","
       << "\"rolling\":" << res.resistance.rolling << ","
       << "\"grade\":" << res.resistance.gradient
       << "},"
       << "\"gearEvaluations\":[";

    for (size_t i = 0; i < res.candidates.size(); ++i) {
        const auto& c = res.candidates[i];
        ss << "{"
           << "\"gear\":" << c.gearNumber << ","
           << "\"rpm\":" << c.engineRpm << ","
           << "\"fuelRate\":" << c.fuelRateL100km << ","
           << "\"status\":\"" << c.status << "\","
           << "\"suitability\":\"" << c.suitability << "\""
           << "}" << (i + 1 < res.candidates.size() ? "," : "");
    }
    ss << "]}";
    return ss.str();
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(18080);

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port 18080. Port might already be in use.\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "=========================================================\n";
    std::cout << " C++ OOP GEAR OPTIMIZER ENGINE LISTENING ON PORT 18080\n";
    std::cout << " Pure Native C++ | Zero External Dependencies\n";
    std::cout << " Ready for dashboard requests...\n";
    std::cout << "=========================================================\n";

    GearOptimizationService optimizerService;

    while (true) {
        SOCKET clientSock = accept(listenSock, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) continue;

        char buffer[8192];
        int bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string req(buffer);

            // Handle CORS Pre-flight OPTIONS
            if (req.rfind("OPTIONS", 0) == 0) {
                std::string res = 
                    "HTTP/1.1 200 OK\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                    "Access-Control-Allow-Headers: Content-Type\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n\r\n";
                send(clientSock, res.c_str(), static_cast<int>(res.length()), 0);
            }
            // Handle POST /api/optimize
            else if (req.rfind("POST", 0) == 0) {
                size_t bodyPos = req.find("\r\n\r\n");
                std::string body = (bodyPos != std::string::npos) ? req.substr(bodyPos + 4) : "";

                auto vehicle = VehicleFactory::createFromRawJson(body);

                DrivingConditions cond;
                cond.speedKmh = MiniJson::extractDouble(body, "speed", 60.0);
                cond.engagedGear = MiniJson::extractInt(body, "engagedGear", 3);
                cond.passengers = MiniJson::extractInt(body, "passengers", 1);
                cond.cargoWeightKg = MiniJson::extractDouble(body, "cargoKg", 20.0);
                cond.gradientPct = MiniJson::extractDouble(body, "gradientPct", 0.0);
                cond.roadType = MiniJson::extractString(body, "roadType", "highway");

                OptimizationResult result = optimizerService.optimize(*vehicle, cond);

                // Terminal console live logger
                std::cout << "[POST /api/optimize] Speed: " << cond.speedKmh 
                          << " km/h | Engaged: Gear " << cond.engagedGear
                          << " -> Recommended: Gear " << result.optimalGear
                          << " (" << result.adviceType << ") | Fuel: " 
                          << result.optimalFuelL100km << " L/100km" << std::endl;

                std::string jsonPayload = buildResponseJson(result);

                std::string res = 
                    "HTTP/1.1 200 OK\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: " + std::to_string(jsonPayload.length()) + "\r\n"
                    "Connection: close\r\n\r\n" + jsonPayload;

                send(clientSock, res.c_str(), static_cast<int>(res.length()), 0);
            }
        }
        closesocket(clientSock);
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}