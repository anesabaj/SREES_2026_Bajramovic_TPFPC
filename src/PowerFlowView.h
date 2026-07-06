#pragma once

#include "TpfpcPluginApi.h"

#include <atomic>
#include <memory>
#include <thread>

#include <gui/Button.h>
#include <gui/CheckBox.h>
#include <gui/GridLayout.h>
#include <gui/Label.h>
#include <gui/LineEdit.h>
#include <gui/ProgressIndicator.h>
#include <gui/View.h>
#include <td/String.h>

class PowerFlowView : public gui::View
{
protected:
    gui::Label _systemTypeLabel;
    gui::Label _systemTypeValue;
    gui::CheckBox _balancedModeCheck;
    gui::Label _balancedModeHint;

    gui::Label _phaseHeader;
    gui::Label _pHeader;
    gui::Label _qHeader;
    gui::Label _magnitudeHeader;
    gui::Label _angleHeader;

    gui::Label _phaseALabel;
    gui::LineEdit _paEdit;
    gui::LineEdit _qaEdit;
    gui::Label _magnitudeAValue;
    gui::Label _angleAValue;

    gui::Label _phaseBLabel;
    gui::LineEdit _pbEdit;
    gui::LineEdit _qbEdit;
    gui::Label _magnitudeBValue;
    gui::Label _angleBValue;

    gui::Label _phaseCLabel;
    gui::LineEdit _pcEdit;
    gui::LineEdit _qcEdit;
    gui::Label _magnitudeCValue;
    gui::Label _angleCValue;

    gui::Label _totalLabel;
    gui::Label _pTotalValue;
    gui::Label _qTotalValue;
    gui::Label _magnitudeTotalValue;
    gui::Label _angleTotalValue;

   // gui::Label _denseInputLabel;
    gui::Label _denseOutputLabel;
    gui::Label _denseOutputHeaderLabel;
    gui::Label _denseOutputARowLabel;
    gui::Label _denseOutputBRowLabel;
    gui::Label _denseOutputCRowLabel;
    gui::Label _sparseInputLabel;
    gui::Label _sparseOutputLabel;
    gui::Label _consumerTypeLabel;

    gui::Label _progressLabel;
    gui::ProgressIndicator _progressIndicator;
    gui::Button _convertButton;
    gui::GridLayout _grid;

    std::thread _worker;
    std::thread _progressThread;
    std::atomic_bool _operationInProgress;
    std::atomic_bool _progressThreadRunning;
    std::shared_ptr<std::atomic_bool> _viewAlive;

public:
    PowerFlowView();
    ~PowerFlowView();

protected:
    bool onClick(gui::Button* pBtn) override;

private:
    struct ConversionDisplayData
    {
        double magnitudeA = 0.0;
        double angleA = 0.0;
        double magnitudeB = 0.0;
        double angleB = 0.0;
        double magnitudeC = 0.0;
        double angleC = 0.0;
        double pTotal = 0.0;
        double qTotal = 0.0;
        double magnitudeTotal = 0.0;
        double angleTotal = 0.0;
        double qA = 0.0;
        double qB = 0.0;
        double qC = 0.0;
        std::size_t sparseInputNonZeroCount = 0;
        std::size_t sparseOutputNonZeroCount = 0;
    };

    bool readInput(const gui::LineEdit& edit, const char* fieldName, double& value, td::String& status) const;
    bool readPluginInput(TpfpcPluginInput& input, td::String& status) const;
    void configureInput(gui::LineEdit& edit, const char* value);
    bool isBalancedMode() const;
    void updateSystemTypeDisplay();
    void applyBalancedInputToGui(const TpfpcPluginInput& input);
    void clearConversionResults();
    void setLabelValue(gui::Label& label, double value, const char* suffix = "");
    void setMatrixLabels(std::size_t sparseInputNonZeroCount, std::size_t sparseOutputNonZeroCount);
    void setStatus(const char* status);
    void setProgress(double value, const char* status);
    bool beginOperation();
    void finishOperation();
    void convert();
    void workerMethod(TpfpcPluginInput input);
    void progressMethod();
    void postProgress(double value);
    void postError();
    void finishConversion(const ConversionDisplayData& result);
    ConversionDisplayData makeDisplayData(const TpfpcPluginInput& input, const TpfpcPluginResult& result) const;
    void setDenseOutputMatrixLabels(const ConversionDisplayData& result);
    void setConsumerTypeLabel(const ConversionDisplayData& result);
};
