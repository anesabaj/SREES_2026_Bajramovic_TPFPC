#include "MatpowerPowerFlow.h"

#include <filesystem>
#include <iostream>
#include <string>

#ifndef TPFPC_SOURCE_ROOT
#define TPFPC_SOURCE_ROOT "."
#endif

namespace
{
std::filesystem::path defaultCasePath()
{
    return std::filesystem::path(TPFPC_SOURCE_ROOT) / "res" / "t_case3p_a.m";
}

std::filesystem::path defaultOutputPath()
{
    return std::filesystem::path(TPFPC_SOURCE_ROOT) / "res" / "t_case3p_a_output.dmodl";
}
}

int main(int argc, const char* argv[])
{
    const std::filesystem::path casePath = argc > 1 ? std::filesystem::path(argv[1]) : defaultCasePath();
    const std::filesystem::path outputPath = argc > 2 ? std::filesystem::path(argv[2]) : defaultOutputPath();

    std::string error;
    tpfpc::matpower::MatpowerCase matpowerCase;
    if (!tpfpc::matpower::loadCase(casePath, matpowerCase, error))
    {
        std::cerr << "MATPOWER parse error: " << error << '\n';
        std::cerr << "Input file: " << casePath.string() << '\n';
        return 1;
    }

    tpfpc::matpower::PowerFlowModel model;
    if (!tpfpc::matpower::buildPowerFlowModel(matpowerCase, model, error))
    {
        std::cerr << "Power-flow model error: " << error << '\n';
        return 1;
    }

    if (!tpfpc::matpower::writeDmodl(model, casePath, outputPath, error))
    {
        std::cerr << "dmodl export error: " << error << '\n';
        return 1;
    }

    std::cout << tpfpc::matpower::makeSummary(model, outputPath);
    return 0;
}
