#include "PowerFlowView.h"

#include <charconv>
#include <cctype>
#include <cstdio>
#include <string>

#include <gui/GridComposer.h>
#include <gui/Thread.h>
#include <thread/Thread.h>

namespace
{
const char* getConsumerType(double q)
    {
        if (q > 0.0)
            return "induktivni";
        if (q < 0.0)
            return "kapacitivni";
        return "aktivni";
    }
bool isBlankText(const td::String& text)
{
    for (const char* pCh = text.c_str(); *pCh != '\0'; ++pCh)
    {
        const unsigned char ch = static_cast<unsigned char>(*pCh);
        if (!std::isspace(ch))
            return false;
    }

    return true;
}

bool parseInvariantDouble(const td::String& text, double& value)
{
    std::string normalized;
    normalized.reserve(text.length());

    for (const char* pCh = text.c_str(); *pCh != '\0'; ++pCh)
    {
        const unsigned char ch = static_cast<unsigned char>(*pCh);
        if (std::isspace(ch))
            continue;

        normalized.push_back(*pCh == ',' ? '.' : *pCh);
    }

    const char* begin = normalized.data();
    const char* end = begin + normalized.size();
    auto [ptr, err] = std::from_chars(begin, end, value);

    return err == std::errc() && ptr == end;
}
}

PowerFlowView::PowerFlowView()
    : gui::View(8, 8, 8, 8)
    , _systemTypeLabel("Tip sistema: ")
    , _systemTypeValue("Nebalansiran")
    , _balancedModeCheck("Balansirani režim")
    , _balancedModeHint("Balansirani: unesite Pa/Qa;  vrijednosti za faze B i C preuzimaju se iz faze A. Nebalansirani režim: unesite vrijednosti za sve faze.")
    , _phaseHeader("Faza")
    , _pHeader("P")
    , _qHeader("Q")
    , _magnitudeHeader("Intenzitet")
    , _angleHeader("Ugao [stepeni]")
    , _phaseALabel("A")
    , _magnitudeAValue("-")
    , _angleAValue("-")
    , _phaseBLabel("B")
    , _magnitudeBValue("-")
    , _angleBValue("-")
    , _phaseCLabel("C")
    , _magnitudeCValue("-")
    , _angleCValue("-")
    , _totalLabel("Ukupno")
    , _pTotalValue("-")
    , _qTotalValue("-")
    , _magnitudeTotalValue("-")
    , _angleTotalValue("-")
    , _denseOutputLabel("Izlazna dense matrica polarnih koordinata")
	, _denseOutputHeaderLabel("        Intenzitet          Ugao [stepeni]")
    , _denseOutputARowLabel("A: -")
	, _denseOutputBRowLabel("B: -")
	, _denseOutputCRowLabel("C: -")
    , _sparseInputLabel("Rijetka ulazna matrica nnz: -")
    , _sparseOutputLabel("Rijetka izlazna matrica nnz: -")
    , _progressLabel("Napredak: ")
    , _convertButton("Konvertuj")
    , _grid(20, 6)
    , _operationInProgress(false)
    , _progressThreadRunning(false)
    , _viewAlive(std::make_shared<std::atomic_bool>(true))
	, _consumerTypeLabel("Tip potrosaca: -")
{
    configureInput(_paEdit, "");
    configureInput(_qaEdit, "");
    configureInput(_pbEdit, "");
    configureInput(_qbEdit, "");
    configureInput(_pcEdit, "");
    configureInput(_qcEdit, "");

    _balancedModeCheck.setChecked(false, false);
    _balancedModeCheck.onClick([this]()
    {
        updateSystemTypeDisplay();
    });

    _systemTypeValue.setResizable(16);
    _denseOutputHeaderLabel.setResizable(60);
	_denseOutputARowLabel.setResizable(60);
	_denseOutputBRowLabel.setResizable(60);
    _denseOutputCRowLabel.setResizable(60);
    _balancedModeHint.setResizable(92);
    _magnitudeAValue.setResizable(12);
    _angleAValue.setResizable(12);
    _magnitudeBValue.setResizable(12);
    _angleBValue.setResizable(12);
    _magnitudeCValue.setResizable(12);
    _angleCValue.setResizable(12);
    _pTotalValue.setResizable(12);
    _qTotalValue.setResizable(12);
    _magnitudeTotalValue.setResizable(12);
    _angleTotalValue.setResizable(12);
    _denseOutputLabel.setResizable(82);
    _sparseInputLabel.setResizable(60);
    _sparseOutputLabel.setResizable(60);
    _progressLabel.setResizable(24);
    _consumerTypeLabel.setResizable(120);
    _progressIndicator.setValue(0.0);
    _convertButton.setType(gui::Button::Type::Default);
    _grid.setSpaceBetweenCells(6, 10);

    gui::GridComposer gridComposer(_grid);
    gridComposer.appendRow(_systemTypeLabel);
    gridComposer.appendCol(_systemTypeValue);
    gridComposer.appendCol(_balancedModeCheck);
    gridComposer.appendCol(_balancedModeHint, 3);

    gridComposer.appendRow(_phaseHeader);
    gridComposer.appendCol(_pHeader);
    gridComposer.appendCol(_qHeader);
    gridComposer.appendCol(_magnitudeHeader);
    gridComposer.appendCol(_angleHeader);

    gridComposer.appendRow(_phaseALabel);
    gridComposer.appendCol(_paEdit);
    gridComposer.appendCol(_qaEdit);
    gridComposer.appendCol(_magnitudeAValue);
    gridComposer.appendCol(_angleAValue);

    gridComposer.appendRow(_phaseBLabel);
    gridComposer.appendCol(_pbEdit);
    gridComposer.appendCol(_qbEdit);
    gridComposer.appendCol(_magnitudeBValue);
    gridComposer.appendCol(_angleBValue);

    gridComposer.appendRow(_phaseCLabel);
    gridComposer.appendCol(_pcEdit);
    gridComposer.appendCol(_qcEdit);
    gridComposer.appendCol(_magnitudeCValue);
    gridComposer.appendCol(_angleCValue);

    gridComposer.appendRow(_totalLabel);
    gridComposer.appendCol(_pTotalValue);
    gridComposer.appendCol(_qTotalValue);
    gridComposer.appendCol(_magnitudeTotalValue);
    gridComposer.appendCol(_angleTotalValue);

   // gridComposer.appendRow(_denseInputLabel, 0);
    gridComposer.appendRow(_denseOutputLabel, 0);
    gridComposer.appendRow(_denseOutputHeaderLabel, 0);
	gridComposer.appendRow(_denseOutputARowLabel, 0);
	gridComposer.appendRow(_denseOutputBRowLabel, 0);
	gridComposer.appendRow(_denseOutputCRowLabel, 0);
    gridComposer.appendRow(_consumerTypeLabel, 0);
    gridComposer.appendRow(_sparseInputLabel);
    gridComposer.appendCol(_sparseOutputLabel, 3);
    gridComposer.appendRow(_progressLabel);
    gridComposer.appendCol(_progressIndicator, 5);
    gridComposer.appendRow(_convertButton, 0, td::HAlignment::Right);

    setLayout(&_grid);
}

PowerFlowView::~PowerFlowView()
{
    *_viewAlive = false;
    _progressThreadRunning = false;
    _operationInProgress = false;

    if (_progressThread.joinable())
        _progressThread.join();

    if (_worker.joinable())
        _worker.join();
}

bool PowerFlowView::onClick(gui::Button* pBtn)
{
    if (pBtn == &_convertButton)
    {
        convert();
        return true;
    }

    return false;
}

bool PowerFlowView::readInput(const gui::LineEdit& edit, const char* fieldName, double& value, td::String& status) const
{
    const td::String text = edit.getText();
    if (isBlankText(text))
    {
        status.format("GRESKA! %s je prazno", fieldName);
        return false;
    }

    if (!parseInvariantDouble(text, value))
    {
        status.format("GRESKA! %s nije ispravan broj", fieldName);
        return false;
    }

    return true;
}

bool PowerFlowView::readPluginInput(TpfpcPluginInput& input, td::String& status) const
{
    if (!readInput(_paEdit, "Pa", input.phaseA.p, status)
        || !readInput(_qaEdit, "Qa", input.phaseA.q, status))
    {
        return false;
    }

    if (isBalancedMode())
    {
        input.phaseB = input.phaseA;
        input.phaseC = input.phaseA;
        return true;
    }

    return readInput(_pbEdit, "Pb", input.phaseB.p, status)
        && readInput(_qbEdit, "Qb", input.phaseB.q, status)
        && readInput(_pcEdit, "Pc", input.phaseC.p, status)
        && readInput(_qcEdit, "Qc", input.phaseC.q, status);
}

void PowerFlowView::configureInput(gui::LineEdit& edit, const char* value)
{
    edit = value;
}

bool PowerFlowView::isBalancedMode() const
{
    return _balancedModeCheck.isChecked();
}

void PowerFlowView::updateSystemTypeDisplay()
{
    _systemTypeValue.setTitle(isBalancedMode() ? "Balansiran" : "Nebalansiran");
}

void PowerFlowView::applyBalancedInputToGui(const TpfpcPluginInput& input)
{
    char pText[96];
    char qText[96];
    std::snprintf(pText, sizeof(pText), "%.10g", input.phaseA.p);
    std::snprintf(qText, sizeof(qText), "%.10g", input.phaseA.q);

    configureInput(_pbEdit, pText);
    configureInput(_qbEdit, qText);
    configureInput(_pcEdit, pText);
    configureInput(_qcEdit, qText);
}

void PowerFlowView::clearConversionResults()
{
    _magnitudeAValue.setTitle("-");
    _angleAValue.setTitle("-");
    _magnitudeBValue.setTitle("-");
    _angleBValue.setTitle("-");
    _magnitudeCValue.setTitle("-");
    _angleCValue.setTitle("-");
    _pTotalValue.setTitle("-");
    _qTotalValue.setTitle("-");
    _magnitudeTotalValue.setTitle("-");
    _angleTotalValue.setTitle("-");
    _sparseInputLabel.setTitle("Rijetka ulazna matrica nnz: -");
    _sparseOutputLabel.setTitle("Rijetka izlazna matrica nnz: -");
    _denseOutputARowLabel.setTitle("A: -");
    _denseOutputBRowLabel.setTitle("B: -");
    _denseOutputCRowLabel.setTitle("C: -");
    _consumerTypeLabel.setTitle("Tip potrosaca: -");
}

void PowerFlowView::setLabelValue(gui::Label& label, double value, const char* suffix)
{
    char text[96];
    std::snprintf(text, sizeof(text), "%.2f%s", value, suffix);
    label.setTitle(text);
}

void PowerFlowView::setMatrixLabels(std::size_t sparseInputNonZeroCount, std::size_t sparseOutputNonZeroCount)
{
    char inputText[64];
    char outputText[64];
    std::snprintf(inputText, sizeof(inputText), "Rijetka ulazna matrica - nnz: %zu", sparseInputNonZeroCount); //nnz-non zero elements
    std::snprintf(outputText, sizeof(outputText), "Rijetka izlazna matrica - nnz: %zu", sparseOutputNonZeroCount);
    _sparseInputLabel.setTitle(inputText);
    _sparseOutputLabel.setTitle(outputText);
}

void PowerFlowView::setDenseOutputMatrixLabels(const ConversionDisplayData& result)
{
    char rowText[96];

    std::snprintf(rowText, sizeof(rowText), "A: %.2f        %.2f stepeni", result.magnitudeA, result.angleA);
    _denseOutputARowLabel.setTitle(rowText);

    std::snprintf(rowText, sizeof(rowText), "B: %.2f        %.2f stepeni", result.magnitudeB, result.angleB);
    _denseOutputBRowLabel.setTitle(rowText);

    std::snprintf(rowText, sizeof(rowText), "C: %.2f        %.2f stepeni", result.magnitudeC, result.angleC);
    _denseOutputCRowLabel.setTitle(rowText);
}

void PowerFlowView::setStatus(const char* status)
{
    _progressIndicator.setValue(0.0);
    _progressLabel.setTitle(status);
}

void PowerFlowView::setProgress(double value, const char* status)
{
    _progressIndicator.setValue(value);

    char text[64];
    std::snprintf(text, sizeof(text), "%s %.0f%%", status, value * 100.0);
    _progressLabel.setTitle(text);
}

bool PowerFlowView::beginOperation()
{
    bool expected = false;
    if (!_operationInProgress.compare_exchange_strong(expected, true))
        return false;

    if (_worker.joinable())
        _worker.join();

    if (_progressThread.joinable())
        _progressThread.join();

    _convertButton.disable();
    setProgress(0.0, "Napredak:");

    _progressThreadRunning = true;
    _progressThread = std::thread(&PowerFlowView::progressMethod, this);

    return true;
}

void PowerFlowView::finishOperation()
{
    _progressThreadRunning = false;
    _operationInProgress = false;
    setProgress(1.0, "Napredak:");
    _convertButton.enable();

    if (_progressThread.joinable())
        _progressThread.join();

    if (_worker.joinable())
        _worker.join();
}

PowerFlowView::ConversionDisplayData PowerFlowView::makeDisplayData(const TpfpcPluginInput& input, const TpfpcPluginResult& result) const
{
    ConversionDisplayData displayData;

    displayData.qA = input.phaseA.q;
    displayData.qB = input.phaseB.q;
    displayData.qC = input.phaseC.q;

    displayData.magnitudeA = result.phaseA.magnitude;
    displayData.angleA = result.phaseA.angleDeg;
    displayData.magnitudeB = result.phaseB.magnitude;
    displayData.angleB = result.phaseB.angleDeg;
    displayData.magnitudeC = result.phaseC.magnitude;
    displayData.angleC = result.phaseC.angleDeg;
    displayData.pTotal = result.pTotal;
    displayData.qTotal = result.qTotal;
    displayData.magnitudeTotal = result.total.magnitude;
    displayData.angleTotal = result.total.angleDeg;
    displayData.sparseInputNonZeroCount = result.sparseInputNonZeroCount;
    displayData.sparseOutputNonZeroCount = result.sparseOutputNonZeroCount;

    return displayData;
}

void PowerFlowView::convert()
{
    if (_operationInProgress.load())
        return;

    clearConversionResults();
    updateSystemTypeDisplay();

    TpfpcPluginInput input;
    td::String status;
    if (!readPluginInput(input, status))
    {
        setStatus(status.c_str());
        return;
    }

    if (isBalancedMode())
        applyBalancedInputToGui(input);

    if (!beginOperation())
        return;

    _worker = std::thread(&PowerFlowView::workerMethod, this, input);
}

void PowerFlowView::workerMethod(TpfpcPluginInput input)
{
    try
    {
        thread::sleepMilliSeconds(120);

        TpfpcPluginResult result;
        if (!tpfpcConvertThreePhase(&input, &result))
        {
            postError();
            return;
        }

        const auto displayData = makeDisplayData(input, result);
        thread::sleepMilliSeconds(240);

        auto viewAlive = _viewAlive;
        gui::thread::asyncExecInMainThread([this, displayData, viewAlive]()
        {
            if (!*viewAlive)
                return;

            finishConversion(displayData);
        });
    }
    catch (...)
    {
        postError();
    }
}

void PowerFlowView::progressMethod()
{
    int progressPercent = 0;

    while (_progressThreadRunning && _operationInProgress)
    {
        postProgress(static_cast<double>(progressPercent) / 100.0);

        if (progressPercent < 95)
            progressPercent += 5;

        thread::sleepMilliSeconds(60);
    }
}

void PowerFlowView::postProgress(double value)
{
    auto viewAlive = _viewAlive;
    gui::thread::asyncExecInMainThread([this, value, viewAlive]()
    {
        if (!*viewAlive)
            return;

        if (!_operationInProgress)
            return;

        setProgress(value, "Napredak:");
    });
}

void PowerFlowView::postError()
{
    auto viewAlive = _viewAlive;
    gui::thread::asyncExecInMainThread([this, viewAlive]()
    {
        if (!*viewAlive)
            return;

        _progressThreadRunning = false;
        _operationInProgress = false;
        setProgress(0.0, "Greska napretka:");
        _convertButton.enable();

        if (_progressThread.joinable())
            _progressThread.join();

        if (_worker.joinable())
            _worker.join();
    });
}

void PowerFlowView::finishConversion(const ConversionDisplayData& result)
{
    setLabelValue(_magnitudeAValue, result.magnitudeA);
    setLabelValue(_angleAValue, result.angleA, " stepeni");
    setLabelValue(_magnitudeBValue, result.magnitudeB);
    setLabelValue(_angleBValue, result.angleB, " stepeni");
    setLabelValue(_magnitudeCValue, result.magnitudeC);
    setLabelValue(_angleCValue, result.angleC, " stepeni");

    setLabelValue(_pTotalValue, result.pTotal);
    setLabelValue(_qTotalValue, result.qTotal);
    setLabelValue(_magnitudeTotalValue, result.magnitudeTotal);
    setLabelValue(_angleTotalValue, result.angleTotal, " stepeni");
    setMatrixLabels(result.sparseInputNonZeroCount, result.sparseOutputNonZeroCount);

    setDenseOutputMatrixLabels(result);
    setConsumerTypeLabel(result);

    finishOperation();
}

void PowerFlowView::setConsumerTypeLabel(const ConversionDisplayData& result)
{
    char text[160];

    std::snprintf(
        text,
        sizeof(text),
        "Tip potrosaca: A=%s, B=%s, C=%s, Ukupno=%s",
        getConsumerType(result.qA),
        getConsumerType(result.qB),
        getConsumerType(result.qC),
        getConsumerType(result.qTotal));

    _consumerTypeLabel.setTitle(text);
}