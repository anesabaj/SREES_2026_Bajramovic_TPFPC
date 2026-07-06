#include "MatpowerPowerFlow.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kZeroTolerance = 1.0e-12;
constexpr int kThreePhaseBusOffset = 1000000;

std::string stripComment(const std::string& line)
{
    const auto commentPos = line.find('%');
    if (commentPos == std::string::npos)
        return line;

    return line.substr(0, commentPos);
}

std::string readTextFile(const std::filesystem::path& filePath)
{
    std::ifstream input(filePath);
    if (!input)
        throw std::runtime_error("Cannot open MATPOWER file");

    std::ostringstream out;
    std::string line;
    while (std::getline(input, line))
        out << stripComment(line) << '\n';

    return out.str();
}

bool parseBaseMva(const std::string& text, double& baseMVA)
{
    const std::regex baseRegex(R"(mpc\s*\.\s*baseMVA\s*=\s*([-+0-9.eE]+)\s*;?)");
    std::smatch match;
    if (!std::regex_search(text, match, baseRegex))
        return false;

    baseMVA = std::stod(match[1].str());
    return true;
}

bool parseNumericToken(std::string token, double& value)
{
    token.erase(
        std::remove_if(
            token.begin(),
            token.end(),
            [](unsigned char ch)
            {
                return ch == '[' || ch == ']' || ch == ',';
            }),
        token.end());

    if (token.empty())
        return false;

    const auto slashPos = token.find('/');
    if (slashPos != std::string::npos)
    {
        double numerator = 0.0;
        double denominator = 0.0;
        if (!parseNumericToken(token.substr(0, slashPos), numerator) ||
            !parseNumericToken(token.substr(slashPos + 1), denominator) ||
            std::abs(denominator) <= kZeroTolerance)
        {
            return false;
        }

        value = numerator / denominator;
        return true;
    }

    try
    {
        std::size_t parsed = 0;
        value = std::stod(token, &parsed);
        return parsed == token.size();
    }
    catch (...)
    {
        return false;
    }
}

bool parseBaseKva(const std::string& text, double& baseKVA)
{
    const std::regex baseRegex(R"(mpc\s*\.\s*base_?kVA\s*=\s*([^;\s]+)\s*;?)");
    std::smatch match;
    if (!std::regex_search(text, match, baseRegex))
        return false;

    return parseNumericToken(match[1].str(), baseKVA);
}

bool extractMatrixText(const std::string& text, const char* matrixName, std::string& matrixText)
{
    const std::regex markerRegex(std::string(R"(mpc\s*\.\s*)") + matrixName + R"(\s*=)");
    std::smatch match;
    if (!std::regex_search(text, match, markerRegex))
        return false;

    const auto markerPos = static_cast<std::size_t>(match.position(0) + match.length(0));
    const auto bracketBegin = text.find('[', markerPos);
    if (bracketBegin == std::string::npos)
        return false;

    const auto bracketEnd = text.find("];", bracketBegin);
    if (bracketEnd == std::string::npos)
        return false;

    matrixText = text.substr(bracketBegin + 1, bracketEnd - bracketBegin - 1);
    return true;
}

std::vector<std::vector<double>> parseMatrixRows(const std::string& matrixText)
{
    std::string normalized = matrixText;
    std::replace(normalized.begin(), normalized.end(), ';', '\n');
    std::replace(normalized.begin(), normalized.end(), ',', ' ');

    std::vector<std::vector<double>> rows;
    std::istringstream rowStream(normalized);
    std::string line;
    while (std::getline(rowStream, line))
    {
        std::istringstream valueStream(line);
        std::vector<double> values;
        std::string token;
        while (valueStream >> token)
        {
            double value = 0.0;
            if (parseNumericToken(token, value))
                values.push_back(value);
        }

        if (!values.empty())
            rows.push_back(values);
    }

    return rows;
}

int roundedInt(double value)
{
    return static_cast<int>(std::lround(value));
}

double clamped(double value, double lower, double upper)
{
    return std::max(lower, std::min(upper, value));
}

double degToRad(double value)
{
    return value * kPi / 180.0;
}

bool isNonZero(const std::complex<double>& value)
{
    return std::abs(value) > kZeroTolerance;
}

bool isThreePhaseBusId(int bus)
{
    return bus >= kThreePhaseBusOffset;
}

int phaseBusId(int bus, int phaseIndex)
{
    return kThreePhaseBusOffset + bus * 10 + phaseIndex + 1;
}

int physicalBusId(int bus)
{
    if (!isThreePhaseBusId(bus))
        return bus;

    return (bus - kThreePhaseBusOffset) / 10;
}

int phaseIndexFromBusId(int bus)
{
    if (!isThreePhaseBusId(bus))
        return -1;

    return (bus - kThreePhaseBusOffset) % 10 - 1;
}

char phaseSuffix(int phaseIndex)
{
    static constexpr char phases[] = {'A', 'B', 'C'};
    if (phaseIndex < 0 || phaseIndex > 2)
        return 'X';

    return phases[phaseIndex];
}

std::string busName(int bus)
{
    if (isThreePhaseBusId(bus))
    {
        const int encoded = bus - kThreePhaseBusOffset;
        const int physicalBus = encoded / 10;
        const int phaseIndex = encoded % 10 - 1;
        return std::to_string(physicalBus) + phaseSuffix(phaseIndex);
    }

    return std::to_string(bus);
}

double powerBase(const tpfpc::matpower::MatpowerCase& matpowerCase)
{
    if (matpowerCase.isThreePhase && matpowerCase.baseKVA > 0.0)
        return matpowerCase.baseKVA;

    return matpowerCase.baseMVA;
}

const tpfpc::matpower::Bus* findBus(const tpfpc::matpower::MatpowerCase& matpowerCase, int busId)
{
    const auto it = std::find_if(
        matpowerCase.buses.begin(),
        matpowerCase.buses.end(),
        [busId](const tpfpc::matpower::Bus& bus)
        {
            return bus.id == busId;
        });

    if (it == matpowerCase.buses.end())
        return nullptr;

    return &(*it);
}

bool hasActiveGenerator(const tpfpc::matpower::MatpowerCase& matpowerCase, int busId)
{
    return std::any_of(
        matpowerCase.generators.begin(),
        matpowerCase.generators.end(),
        [busId](const tpfpc::matpower::Generator& generator)
        {
            return generator.bus == busId && generator.status != 0;
        });
}

double getGeneratorVoltageSetpoint(const tpfpc::matpower::MatpowerCase& matpowerCase, int busId, double fallback)
{
    for (const auto& generator : matpowerCase.generators)
    {
        if (generator.bus == busId && generator.status != 0)
            return generator.vg;
    }

    return fallback;
}

double getInjectionP(const tpfpc::matpower::PowerFlowModel& model, int busId)
{
    for (const auto& injection : model.injections)
    {
        if (injection.bus == busId)
            return injection.p;
    }

    return 0.0;
}

double getInjectionQ(const tpfpc::matpower::PowerFlowModel& model, int busId)
{
    for (const auto& injection : model.injections)
    {
        if (injection.bus == busId)
            return injection.q;
    }

    return 0.0;
}

bool containsBus(const std::vector<int>& buses, int busId)
{
    return std::find(buses.begin(), buses.end(), busId) != buses.end();
}

std::string busName(int bus);

void appendBusList(std::ostringstream& out, const char* label, const std::vector<int>& buses)
{
    out << label << " count: " << buses.size() << ", list:";
    if (buses.empty())
    {
        out << " -\n";
        return;
    }

    for (const auto bus : buses)
        out << ' ' << busName(bus);

    out << '\n';
}

std::string variableV(const tpfpc::matpower::PowerFlowModel& model, int busId)
{
    if (containsBus(model.slackBuses, busId))
        return "V_slack_" + busName(busId);

    return "V_" + busName(busId);
}

std::string variableDelta(const tpfpc::matpower::PowerFlowModel& model, int busId)
{
    if (containsBus(model.slackBuses, busId))
        return "delta_slack_" + busName(busId);

    return "delta_" + busName(busId);
}

std::vector<std::array<int, 3>> threePhaseBusGroups(const tpfpc::matpower::PowerFlowModel& model)
{
    std::map<int, std::array<int, 3>> groups;
    for (const auto& bus : model.inputCase.buses)
    {
        if (!isThreePhaseBusId(bus.id))
            continue;

        const int phaseIndex = phaseIndexFromBusId(bus.id);
        if (phaseIndex < 0 || phaseIndex > 2)
            continue;

        groups[physicalBusId(bus.id)][phaseIndex] = bus.id;
    }

    std::vector<std::array<int, 3>> result;
    result.reserve(groups.size());
    for (const auto& group : groups)
        result.push_back(group.second);

    return result;
}

void writeThreePhasePlotParams(std::ostringstream& out)
{
    out << "\tbusIdPlot = 0 [type=int out=true]\n";
    out << "\tvMagPlotA = 0 [out=true]\n";
    out << "\tvMagPlotB = 0 [out=true]\n";
    out << "\tvMagPlotC = 0 [out=true]\n";
    out << "\tanglePlotA = 0 [out=true]\n";
    out << "\tanglePlotB = 0 [out=true]\n";
    out << "\tanglePlotC = 0 [out=true]\n";
}

void writeThreePhasePlotPostProcessing(std::ostringstream& out, const tpfpc::matpower::PowerFlowModel& model)
{
    const auto groups = threePhaseBusGroups(model);
    if (groups.empty())
        return;

    out
        << "PostProc:\n"
        << "busIdPlot=0\n"
        << "vMagPlotA=0\n"
        << "vMagPlotB=0\n"
        << "vMagPlotC=0\n"
        << "anglePlotA=0\n"
        << "anglePlotB=0\n"
        << "anglePlotC=0\n";

    for (std::size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex)
    {
        const auto& group = groups[groupIndex];
        int physicalBus = static_cast<int>(groupIndex) + 1;
        for (int busId : group)
        {
            if (busId != 0)
            {
                physicalBus = physicalBusId(busId);
                break;
            }
        }

        out << "if plotIndex == " << (groupIndex + 1) << ":\n";
        out << "busIdPlot=" << physicalBus << "\n";

        for (int phaseIndex = 0; phaseIndex < 3; ++phaseIndex)
        {
            const int busId = group[phaseIndex];
            if (busId == 0)
                continue;

            const char phase = phaseSuffix(phaseIndex);
            out << "vMagPlot" << phase << "=" << variableV(model, busId) << "\n";
            out << "anglePlot" << phase << "=" << variableDelta(model, busId) << "*57.29577951308232\n";
        }

        out << "end\n";
    }

    out
        << "Repeats:\n"
        << "if plotIndex < " << groups.size() << ":\n"
        << "plotIndex += 1\n"
        << "repeat\n"
        << "end\n";
}

std::string formatDouble(double value)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(12) << value;
    return out.str();
}

std::string powerExpression(const tpfpc::matpower::PowerFlowModel& model, int busId, bool reactive)
{
    const std::string vi = variableV(model, busId);
    const std::string di = variableDelta(model, busId);
    std::vector<std::string> terms;

    for (const auto& entry : model.ybusSparse)
    {
        if (entry.rowBus != busId)
            continue;

        const std::string yName = "Y_" + busName(entry.rowBus) + "_" + busName(entry.colBus);
        const std::string thetaName = "theta_" + busName(entry.rowBus) + "_" + busName(entry.colBus);

        std::ostringstream term;
        term.imbue(std::locale::classic());
        term << std::setprecision(12);

        if (entry.colBus == busId)
        {
            if (reactive)
                term << "-" << vi << "*" << vi << "*" << yName << "*sin(" << thetaName << ")";
            else
                term << vi << "*" << vi << "*" << yName << "*cos(" << thetaName << ")";
        }
        else
        {
            const std::string vj = variableV(model, entry.colBus);
            const std::string dj = variableDelta(model, entry.colBus);
            if (reactive)
                term << vi << "*" << vj << "*" << yName << "*sin(" << di << "-" << thetaName << "-" << dj << ")";
            else
                term << vi << "*" << vj << "*" << yName << "*cos(" << di << "-" << thetaName << "-" << dj << ")";
        }

        terms.push_back(term.str());
    }

    if (terms.empty())
        return "0";

    std::ostringstream out;
    for (std::size_t i = 0; i < terms.size(); ++i)
    {
        const auto& term = terms[i];
        if (i == 0)
        {
            out << term;
            continue;
        }

        if (!term.empty() && term[0] == '-')
            out << " - " << term.substr(1);
        else
            out << " + " << term;
    }
    return out.str();
}

using ComplexMatrix3 = std::array<std::array<std::complex<double>, 3>, 3>;

bool invertMatrix3(const ComplexMatrix3& matrix, ComplexMatrix3& inverse)
{
    const auto& a = matrix;
    const auto det =
        a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
        a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
        a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);

    if (std::abs(det) <= kZeroTolerance)
        return false;

    inverse[0][0] = (a[1][1] * a[2][2] - a[1][2] * a[2][1]) / det;
    inverse[0][1] = (a[0][2] * a[2][1] - a[0][1] * a[2][2]) / det;
    inverse[0][2] = (a[0][1] * a[1][2] - a[0][2] * a[1][1]) / det;
    inverse[1][0] = (a[1][2] * a[2][0] - a[1][0] * a[2][2]) / det;
    inverse[1][1] = (a[0][0] * a[2][2] - a[0][2] * a[2][0]) / det;
    inverse[1][2] = (a[0][2] * a[1][0] - a[0][0] * a[1][2]) / det;
    inverse[2][0] = (a[1][0] * a[2][1] - a[1][1] * a[2][0]) / det;
    inverse[2][1] = (a[0][1] * a[2][0] - a[0][0] * a[2][1]) / det;
    inverse[2][2] = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) / det;
    return true;
}

void addYbusContribution(
    tpfpc::matpower::MatpowerCase& matpowerCase,
    int rowBus,
    int colBus,
    const std::complex<double>& value)
{
    if (!isNonZero(value))
        return;

    tpfpc::matpower::YbusContribution contribution;
    contribution.rowBus = rowBus;
    contribution.colBus = colBus;
    contribution.value = value;
    matpowerCase.ybusContributions.push_back(contribution);
}

void stampThreePhaseSeries(
    tpfpc::matpower::MatpowerCase& matpowerCase,
    int fromBus,
    int toBus,
    const ComplexMatrix3& seriesAdmittance)
{
    for (int rowPhase = 0; rowPhase < 3; ++rowPhase)
    {
        for (int colPhase = 0; colPhase < 3; ++colPhase)
        {
            const auto y = seriesAdmittance[rowPhase][colPhase];
            addYbusContribution(matpowerCase, phaseBusId(fromBus, rowPhase), phaseBusId(fromBus, colPhase), y);
            addYbusContribution(matpowerCase, phaseBusId(toBus, rowPhase), phaseBusId(toBus, colPhase), y);
            addYbusContribution(matpowerCase, phaseBusId(fromBus, rowPhase), phaseBusId(toBus, colPhase), -y);
            addYbusContribution(matpowerCase, phaseBusId(toBus, rowPhase), phaseBusId(fromBus, colPhase), -y);
        }
    }
}

tpfpc::matpower::Bus* findMutableBus(tpfpc::matpower::MatpowerCase& matpowerCase, int busId)
{
    const auto it = std::find_if(
        matpowerCase.buses.begin(),
        matpowerCase.buses.end(),
        [busId](const tpfpc::matpower::Bus& bus)
        {
            return bus.id == busId;
        });

    if (it == matpowerCase.buses.end())
        return nullptr;

    return &(*it);
}

double loadReactivePower(double activePower, double powerFactor)
{
    const double absPowerFactor = clamped(std::abs(powerFactor), 1.0e-6, 1.0);
    const double reactivePower = activePower * std::tan(std::acos(absPowerFactor));
    return powerFactor < 0.0 ? -reactivePower : reactivePower;
}

bool parseThreePhaseCase(const std::string& text, tpfpc::matpower::MatpowerCase& parsedCase, std::string& error)
{
    std::string matrixText;
    if (!extractMatrixText(text, "bus3p", matrixText))
    {
        error = "Missing mpc.bus matrix";
        return false;
    }

    const auto busRows = parseMatrixRows(matrixText);
    if (busRows.empty())
    {
        error = "Missing mpc.bus matrix and mpc.bus3p is empty";
        return false;
    }

    if (!parseBaseKva(text, parsedCase.baseKVA))
    {
        error = "Missing mpc.basekVA for MATPOWER three-phase case";
        return false;
    }

    parsedCase.isThreePhase = true;
    std::map<int, double> baseKvByBus;

    for (const auto& row : busRows)
    {
        if (row.size() < 9)
        {
            error = "mpc.bus3p row has fewer than 9 columns";
            return false;
        }

        const int busId = roundedInt(row[0]);
        const int busType = roundedInt(row[1]);
        baseKvByBus[busId] = row[2];

        for (int phase = 0; phase < 3; ++phase)
        {
            tpfpc::matpower::Bus bus;
            bus.id = phaseBusId(busId, phase);
            bus.type = busType;
            bus.vm = row[3 + phase];
            bus.vaDeg = row[6 + phase];
            parsedCase.buses.push_back(bus);
        }
    }

    parsedCase.threePhaseBusCount = static_cast<int>(busRows.size());

    if (extractMatrixText(text, "load3p", matrixText))
    {
        for (const auto& row : parseMatrixRows(matrixText))
        {
            if (row.size() < 9)
            {
                error = "mpc.load3p row has fewer than 9 columns";
                return false;
            }

            if (roundedInt(row[2]) == 0)
                continue;

            const int busId = roundedInt(row[1]);
            ++parsedCase.threePhaseLoadCount;
            for (int phase = 0; phase < 3; ++phase)
            {
                auto* bus = findMutableBus(parsedCase, phaseBusId(busId, phase));
                if (bus == nullptr)
                {
                    error = "mpc.load3p references an unknown bus3p";
                    return false;
                }

                const double activePower = row[3 + phase];
                bus->pd += activePower;
                bus->qd += loadReactivePower(activePower, row[6 + phase]);
            }
        }
    }

    if (extractMatrixText(text, "shunt3p", matrixText))
    {
        for (const auto& row : parseMatrixRows(matrixText))
        {
            if (row.size() < 9)
            {
                error = "mpc.shunt3p row has fewer than 9 columns";
                return false;
            }

            if (roundedInt(row[2]) == 0)
                continue;

            const int busId = roundedInt(row[1]);
            for (int phase = 0; phase < 3; ++phase)
            {
                auto* bus = findMutableBus(parsedCase, phaseBusId(busId, phase));
                if (bus == nullptr)
                {
                    error = "mpc.shunt3p references an unknown bus3p";
                    return false;
                }

                bus->gs += row[3 + phase];
                bus->bs += row[6 + phase];
            }
        }
    }

    if (extractMatrixText(text, "gen3p", matrixText))
    {
        for (const auto& row : parseMatrixRows(matrixText))
        {
            if (row.size() < 12)
            {
                error = "mpc.gen3p row has fewer than 12 columns";
                return false;
            }

            if (roundedInt(row[2]) == 0)
                continue;

            const int busId = roundedInt(row[1]);
            ++parsedCase.threePhaseGeneratorCount;
            for (int phase = 0; phase < 3; ++phase)
            {
                tpfpc::matpower::Generator generator;
                generator.bus = phaseBusId(busId, phase);
                generator.vg = row[3 + phase];
                generator.pg = row[6 + phase];
                generator.qg = row[9 + phase];
                generator.status = 1;
                parsedCase.generators.push_back(generator);
            }
        }
    }

    std::map<int, ComplexMatrix3> lineConstructionById;
    if (extractMatrixText(text, "lc", matrixText))
    {
        for (const auto& row : parseMatrixRows(matrixText))
        {
            if (row.size() < 19)
            {
                error = "mpc.lc row has fewer than 19 columns";
                return false;
            }

            ComplexMatrix3 impedance = {};
            impedance[0][0] = {row[1], row[7]};
            impedance[1][0] = impedance[0][1] = {row[2], row[8]};
            impedance[2][0] = impedance[0][2] = {row[3], row[9]};
            impedance[1][1] = {row[4], row[10]};
            impedance[2][1] = impedance[1][2] = {row[5], row[11]};
            impedance[2][2] = {row[6], row[12]};
            lineConstructionById[roundedInt(row[0])] = impedance;
        }
    }

    if (extractMatrixText(text, "line3p", matrixText))
    {
        for (const auto& row : parseMatrixRows(matrixText))
        {
            if (row.size() < 6)
            {
                error = "mpc.line3p row has fewer than 6 columns";
                return false;
            }

            if (roundedInt(row[3]) == 0)
                continue;

            const int fromBus = roundedInt(row[1]);
            const int toBus = roundedInt(row[2]);
            const int constructionId = roundedInt(row[4]);
            const auto constructionIt = lineConstructionById.find(constructionId);
            if (constructionIt == lineConstructionById.end())
            {
                error = "mpc.line3p references an unknown mpc.lc row";
                return false;
            }

            const auto baseKvIt = baseKvByBus.find(fromBus);
            if (baseKvIt == baseKvByBus.end() || parsedCase.baseKVA <= 0.0)
            {
                error = "Cannot compute three-phase line base impedance";
                return false;
            }

            const double lengthMiles = row[5];
            const double zBaseOhm = (baseKvIt->second * baseKvIt->second / 3.0) / (parsedCase.baseKVA / 1000.0);
            ComplexMatrix3 zPu = {};
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                    zPu[i][j] = constructionIt->second[i][j] * lengthMiles / zBaseOhm;
            }

            ComplexMatrix3 ySeries = {};
            if (!invertMatrix3(zPu, ySeries))
            {
                error = "Cannot invert three-phase line impedance matrix";
                return false;
            }

            stampThreePhaseSeries(parsedCase, fromBus, toBus, ySeries);
            ++parsedCase.threePhaseLineCount;

            tpfpc::matpower::Branch branchCountMarker;
            branchCountMarker.fromBus = phaseBusId(fromBus, 0);
            branchCountMarker.toBus = phaseBusId(toBus, 0);
            branchCountMarker.status = 0;
            parsedCase.branches.push_back(branchCountMarker);
        }
    }

    std::string xfmrMatrixText;
    if (!extractMatrixText(text, "xfmr3p", xfmrMatrixText))
        extractMatrixText(text, "xfrm3p", xfmrMatrixText);

    if (!xfmrMatrixText.empty())
    {
        for (const auto& row : parseMatrixRows(xfmrMatrixText))
        {
            if (row.size() < 9)
            {
                error = "mpc.xfmr3p row has fewer than 9 columns";
                return false;
            }

            if (roundedInt(row[3]) == 0)
                continue;

            std::complex<double> z(row[4], row[5]);
            if (std::abs(z) <= kZeroTolerance)
            {
                error = "Active three-phase transformer has zero impedance";
                return false;
            }

            const double transformerBaseKva = row[6];
            if (transformerBaseKva > 0.0 && parsedCase.baseKVA > 0.0)
                z *= parsedCase.baseKVA / (transformerBaseKva / 3.0);

            const auto y = 1.0 / z;
            ComplexMatrix3 ySeries = {};
            for (int phase = 0; phase < 3; ++phase)
                ySeries[phase][phase] = y;

            const int fromBus = roundedInt(row[1]);
            const int toBus = roundedInt(row[2]);
            stampThreePhaseSeries(parsedCase, fromBus, toBus, ySeries);
            ++parsedCase.threePhaseTransformerCount;

            tpfpc::matpower::Branch branchCountMarker;
            branchCountMarker.fromBus = phaseBusId(fromBus, 0);
            branchCountMarker.toBus = phaseBusId(toBus, 0);
            branchCountMarker.status = 0;
            parsedCase.branches.push_back(branchCountMarker);
        }
    }

    return true;
}
}

namespace tpfpc::matpower
{
bool loadCase(const std::filesystem::path& filePath, MatpowerCase& matpowerCase, std::string& error)
{
    try
    {
        const auto text = readTextFile(filePath);

        MatpowerCase parsedCase;
        if (!parseBaseMva(text, parsedCase.baseMVA))
        {
            error = "Missing mpc.baseMVA";
            return false;
        }

        std::string matrixText;
        if (!extractMatrixText(text, "bus", matrixText))
        {
            if (!parseThreePhaseCase(text, parsedCase, error))
                return false;

            matpowerCase = parsedCase;
            return true;
        }

        const auto busRows = parseMatrixRows(matrixText);
        if (busRows.empty())
        {
            std::string threePhaseText;
            if (extractMatrixText(text, "bus3p", threePhaseText))
            {
                if (!parseThreePhaseCase(text, parsedCase, error))
                    return false;

                matpowerCase = parsedCase;
                return true;
            }
        }

        for (const auto& row : busRows)
        {
            if (row.size() < 13)
            {
                error = "mpc.bus row has fewer than 13 columns";
                return false;
            }

            Bus bus;
            bus.id = roundedInt(row[0]);
            bus.type = roundedInt(row[1]);
            bus.pd = row[2];
            bus.qd = row[3];
            bus.gs = row[4];
            bus.bs = row[5];
            bus.vm = row[7];
            bus.vaDeg = row[8];
            parsedCase.buses.push_back(bus);
        }

        if (!extractMatrixText(text, "gen", matrixText))
        {
            error = "Missing mpc.gen matrix";
            return false;
        }

        for (const auto& row : parseMatrixRows(matrixText))
        {
            if (row.size() < 8)
            {
                error = "mpc.gen row has fewer than 8 columns";
                return false;
            }

            Generator generator;
            generator.bus = roundedInt(row[0]);
            generator.pg = row[1];
            generator.qg = row[2];
            generator.vg = row[5];
            generator.status = roundedInt(row[7]);
            parsedCase.generators.push_back(generator);
        }

        if (!extractMatrixText(text, "branch", matrixText))
        {
            error = "Missing mpc.branch matrix";
            return false;
        }

        for (const auto& row : parseMatrixRows(matrixText))
        {
            if (row.size() < 11)
            {
                error = "mpc.branch row has fewer than 11 columns";
                return false;
            }

            Branch branch;
            branch.fromBus = roundedInt(row[0]);
            branch.toBus = roundedInt(row[1]);
            branch.r = row[2];
            branch.x = row[3];
            branch.b = row[4];
            branch.ratio = row[8];
            branch.angleDeg = row[9];
            branch.status = roundedInt(row[10]);
            parsedCase.branches.push_back(branch);
        }

        if (parsedCase.buses.empty())
        {
            error = "MATPOWER case has no buses";
            return false;
        }

        matpowerCase = parsedCase;
        return true;
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
        return false;
    }
}

bool buildPowerFlowModel(const MatpowerCase& matpowerCase, PowerFlowModel& model, std::string& error)
{
    const double basePower = powerBase(matpowerCase);
    if (basePower == 0.0)
    {
        error = "MATPOWER power base must not be zero";
        return false;
    }

    PowerFlowModel builtModel;
    builtModel.inputCase = matpowerCase;

    std::map<int, std::size_t> busToIndex;
    for (std::size_t i = 0; i < matpowerCase.buses.size(); ++i)
    {
        if (busToIndex.count(matpowerCase.buses[i].id) != 0)
        {
            error = "Duplicate MATPOWER bus id";
            return false;
        }

        busToIndex[matpowerCase.buses[i].id] = i;
    }

    for (const auto& generator : matpowerCase.generators)
    {
        if (generator.status != 0 && busToIndex.count(generator.bus) == 0)
        {
            error = "Active generator references an unknown bus";
            return false;
        }
    }

    for (const auto& branch : matpowerCase.branches)
    {
        if (branch.status == 0)
            continue;

        if (busToIndex.count(branch.fromBus) == 0 || busToIndex.count(branch.toBus) == 0)
        {
            error = "Active branch references an unknown bus";
            return false;
        }
    }

    const auto nBuses = matpowerCase.buses.size();
    builtModel.ybusDense.assign(nBuses, std::vector<std::complex<double>>(nBuses, {0.0, 0.0}));

    for (const auto& bus : matpowerCase.buses)
    {
        if (bus.type == kBusTypeSlack)
            builtModel.slackBuses.push_back(bus.id);
        else if (bus.type == kBusTypePV && hasActiveGenerator(matpowerCase, bus.id))
            builtModel.pvBuses.push_back(bus.id);
        else
            builtModel.pqBuses.push_back(bus.id);
    }

    if (builtModel.slackBuses.empty())
    {
        error = "MATPOWER case has no slack/reference bus";
        return false;
    }

    for (const auto& branch : matpowerCase.branches)
    {
        if (branch.status == 0)
            continue;

        const std::complex<double> z(branch.r, branch.x);
        if (std::abs(z) <= kZeroTolerance)
        {
            error = "Active branch has zero impedance";
            return false;
        }

        const auto y = 1.0 / z;
        const std::complex<double> charging(0.0, branch.b / 2.0);
        const double tapMagnitude = branch.ratio == 0.0 ? 1.0 : branch.ratio;
        const auto tap = std::polar(tapMagnitude, degToRad(branch.angleDeg));
        const auto tapConj = std::conj(tap);

        const auto fromIndex = busToIndex[branch.fromBus];
        const auto toIndex = busToIndex[branch.toBus];

        builtModel.ybusDense[fromIndex][fromIndex] += (y + charging) / (tap * tapConj);
        builtModel.ybusDense[fromIndex][toIndex] += -y / tapConj;
        builtModel.ybusDense[toIndex][fromIndex] += -y / tap;
        builtModel.ybusDense[toIndex][toIndex] += y + charging;
    }

    for (const auto& bus : matpowerCase.buses)
    {
        const auto index = busToIndex[bus.id];
        builtModel.ybusDense[index][index] += std::complex<double>(bus.gs, bus.bs) / basePower;
    }

    for (const auto& contribution : matpowerCase.ybusContributions)
    {
        if (busToIndex.count(contribution.rowBus) == 0 || busToIndex.count(contribution.colBus) == 0)
        {
            error = "Three-phase Ybus contribution references an unknown bus";
            return false;
        }

        builtModel.ybusDense[busToIndex[contribution.rowBus]][busToIndex[contribution.colBus]] += contribution.value;
    }

    for (std::size_t row = 0; row < nBuses; ++row)
    {
        for (std::size_t col = 0; col < nBuses; ++col)
        {
            const auto value = builtModel.ybusDense[row][col];
            if (!isNonZero(value))
                continue;

            YbusEntry entry;
            entry.rowBus = matpowerCase.buses[row].id;
            entry.colBus = matpowerCase.buses[col].id;
            entry.value = value;
            entry.magnitude = std::abs(value);
            entry.angleRad = std::atan2(value.imag(), value.real());
            builtModel.ybusSparse.push_back(entry);
        }
    }

    for (const auto& bus : matpowerCase.buses)
    {
        BusInjection injection;
        injection.bus = bus.id;
        injection.p = -bus.pd / basePower;
        injection.q = -bus.qd / basePower;

        for (const auto& generator : matpowerCase.generators)
        {
            if (generator.status == 0 || generator.bus != bus.id)
                continue;

            injection.p += generator.pg / basePower;
            injection.q += generator.qg / basePower;
        }

        builtModel.injections.push_back(injection);
    }

    model = builtModel;
    return true;
}

std::string createDmodl(const PowerFlowModel& model)
{
    return createDmodl(model, {});
}

std::string createDmodl(const PowerFlowModel& model, const std::filesystem::path& inputPath)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(10);

    out
        << "Header:\n"
        << "\tmaxIter = 100\n"
        << "\treport = All\n"
        << "end\n\n"
        << "// input: MATPOWER case file";

    if (!inputPath.empty())
        out << " = " << inputPath.string();

    out
        << "\n"
        << "// converter: MATPOWER-to-dTwin polar power-flow converter\n"
        << "// voltage representation: V_i and delta_i";

    if (model.inputCase.isThreePhase)
        out << " (three-phase nodes use suffixes A, B, C)";

    out
        << "\n"
        << "// admittance representation: Y_i_j and theta_i_j\n"
        << "// equations: nonlinear power-flow equations in polar coordinates\n"
        << "// Generated by SREES_2026_Bajramovic_TPFPC MATPOWER polar converter\n"
        << "// Supported stable path: standard small MATPOWER cases with mpc.baseMVA, mpc.bus, mpc.gen, mpc.branch.\n"
        << "// Supported three-phase path: MATPOWER prototype fields mpc.bus3p, mpc.line3p, mpc.xfmr3p, mpc.load3p, mpc.gen3p, mpc.lc.\n"
        << "// Bus types: PQ=1, PV=2, Slack=3. Inactive generators/branches are ignored.\n"
        << "Model [type=NL domain=real eps=1e-8 name=\"PF in polar coordinates\"]:\n";

    out << "Vars [out=true]:\n";
    for (const auto& bus : model.inputCase.buses)
    {
        if (containsBus(model.slackBuses, bus.id))
            continue;

        out << "\tdelta_" << busName(bus.id) << " = " << degToRad(bus.vaDeg) << "\n";
        out << "\tV_" << busName(bus.id) << " = " << bus.vm << "\n";
    }

    out << "Params:\n";
    out << "\tbaseMVA = " << model.inputCase.baseMVA << "\n";
    out << "\tplotIndex = 1 [type=int out=true]\n";
    if (model.inputCase.isThreePhase)
    {
        out << "\tbaseKVA_3p = " << model.inputCase.baseKVA << "\n";
        writeThreePhasePlotParams(out);
    }

    for (const auto& bus : model.inputCase.buses)
    {
        if (!containsBus(model.slackBuses, bus.id))
            continue;

        out << "\tV_slack_" << busName(bus.id) << " = " << bus.vm << " [out=true]\n";
        out << "\tdelta_slack_" << busName(bus.id) << " = " << degToRad(bus.vaDeg) << " [out=true]\n";
    }

    for (const auto& bus : model.inputCase.buses)
    {
        out << "\tP_" << busName(bus.id) << " = " << getInjectionP(model, bus.id) << "\n";
        out << "\tQ_" << busName(bus.id) << " = " << getInjectionQ(model, bus.id) << "\n";
        if (containsBus(model.pvBuses, bus.id))
            out << "\tV_" << busName(bus.id) << "_sp = " << getGeneratorVoltageSetpoint(model.inputCase, bus.id, bus.vm) << "\n";
    }

    for (const auto& entry : model.ybusSparse)
    {
        out << "\tY_" << busName(entry.rowBus) << "_" << busName(entry.colBus) << " = " << entry.magnitude << "\n";
        out << "\ttheta_" << busName(entry.rowBus) << "_" << busName(entry.colBus) << " = " << entry.angleRad << "\n";
    }

    out << "NLEs:\n";
    for (const auto& bus : model.inputCase.buses)
    {
        if (containsBus(model.slackBuses, bus.id))
        {
            out << "\t// node " << busName(bus.id) << " - SLACK, V and delta fixed as params\n";
            continue;
        }

        if (containsBus(model.pqBuses, bus.id))
        {
            out << "\t// node " << busName(bus.id) << " - PQ\n";
            out << "\tP_" << busName(bus.id) << " = " << powerExpression(model, bus.id, false) << "\n";
            out << "\tQ_" << busName(bus.id) << " = " << powerExpression(model, bus.id, true) << "\n";
            continue;
        }

        out << "\t// node " << busName(bus.id) << " - PV\n";
        out << "\tP_" << busName(bus.id) << " = " << powerExpression(model, bus.id, false) << "\n";
        out << "\tV_" << busName(bus.id) << " = V_" << busName(bus.id) << "_sp\n";
    }

    if (model.inputCase.isThreePhase)
        writeThreePhasePlotPostProcessing(out, model);

    out << "end\n";
    return out.str();
}

std::string createVoltagePhaseVisualModel(const PowerFlowModel& model)
{
    if (!model.inputCase.isThreePhase)
        return {};

    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(10);

    out
        << "Header:\n"
        << "\tnewTab = false\n"
        << "\tdrawPlots = true\n"
        << "end\n\n"
        << "Model [name=\"TPFPC voltage plots\"]:\n"
        << "Plots [backColor=auto]:\n"
        << "\tlinePlot [xLabel=\"Bus\" yLabel=\"|V| [p.u.]\" name=\"Voltage magnitude by phase\" anchor=TR legend=true nCols=1 anchorX=130 anchorY=35]:\n"
        << "\t\t@x << busIdPlot\n"
        << "\t\t@y << vMagPlotA [name=\"A\"]\n"
        << "\t\t@y << vMagPlotB [name=\"B\"]\n"
        << "\t\t@y << vMagPlotC [name=\"C\"]\n"
        << "\tend\n\n"
        << "\tlinePlot [xLabel=\"Bus\" yLabel=\"delta [deg]\" name=\"Voltage angle by phase\" anchor=TR legend=true nCols=1 anchorX=130 anchorY=35]:\n"
        << "\t\t@x << busIdPlot\n"
        << "\t\t@y << anglePlotA [name=\"A\"]\n"
        << "\t\t@y << anglePlotB [name=\"B\"]\n"
        << "\t\t@y << anglePlotC [name=\"C\"]\n"
        << "\tend\n"
        << "end\n";

    return out.str();
}

bool writeDmodl(const PowerFlowModel& model, const std::filesystem::path& filePath, std::string& error)
{
    return writeDmodl(model, {}, filePath, error);
}

bool writeDmodl(const PowerFlowModel& model, const std::filesystem::path& inputPath, const std::filesystem::path& filePath, std::string& error)
{
    std::error_code ec;
    const auto parentPath = filePath.parent_path();
    if (!parentPath.empty())
        std::filesystem::create_directories(parentPath, ec);

    std::ofstream output(filePath, std::ios::binary);
    if (!output)
    {
        error = "Cannot create dmodl output file";
        return false;
    }

    output << createDmodl(model, inputPath);

    const auto visualModel = createVoltagePhaseVisualModel(model);
    if (!visualModel.empty())
    {
        std::filesystem::path visualPath = filePath;
        visualPath.replace_extension(".vmodl");

        std::ofstream visualOutput(visualPath, std::ios::binary);
        if (!visualOutput)
        {
            error = "Cannot create visual model output file";
            return false;
        }

        visualOutput << visualModel;
    }

    return true;
}

std::string makeSummary(const PowerFlowModel& model, const std::filesystem::path& dmodlPath)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(6);

    out << "MATPOWER-to-dTwin polar PF conversion: OK\n";
    out << "baseMVA: " << model.inputCase.baseMVA << '\n';
    if (model.inputCase.isThreePhase)
    {
        out << "baseKVA_3p: " << model.inputCase.baseKVA << '\n';
        out << "3-phase buses: " << model.inputCase.threePhaseBusCount << '\n';
        out << "phase nodes: " << model.inputCase.buses.size() << '\n';
        out << "3-phase generators: " << model.inputCase.threePhaseGeneratorCount << '\n';
        out << "3-phase loads: " << model.inputCase.threePhaseLoadCount << '\n';
        out << "3-phase lines: " << model.inputCase.threePhaseLineCount << '\n';
        out << "3-phase transformers: " << model.inputCase.threePhaseTransformerCount << '\n';
    }
    else
    {
        out << "buses: " << model.inputCase.buses.size() << '\n';
    }
    out << "generators: " << model.inputCase.generators.size() << '\n';
    out << "branches: " << model.inputCase.branches.size() << '\n';
    appendBusList(out, "PQ buses", model.pqBuses);
    appendBusList(out, "PV buses", model.pvBuses);
    appendBusList(out, "Slack buses", model.slackBuses);
    out << "Ybus sparse nnz: " << model.ybusSparse.size() << '\n';
    out << "Ybus polar entries:\n";
    for (const auto& entry : model.ybusSparse)
    {
        out << "  Y_" << busName(entry.rowBus) << "_" << busName(entry.colBus)
            << ": G=" << entry.value.real()
            << ", B=" << entry.value.imag()
            << ", mag=" << entry.magnitude
            << ", angleRad=" << entry.angleRad << '\n';
    }

    out << "Net injections p.u.:\n";
    for (const auto& injection : model.injections)
        out << "  bus " << busName(injection.bus) << ": P=" << injection.p << ", Q=" << injection.q << '\n';

    out << "Generated dmodl: " << dmodlPath.string() << '\n';
    return out.str();
}
}
