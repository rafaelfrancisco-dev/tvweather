#pragma once

#define Uses_TDialog
#include <tvision/tv.h>

#include "tvweather/Language.h"

class TRadioButtons;

class OptionsDialog : public TDialog {
public:
    explicit OptionsDialog(tvweather::Language language);

    tvweather::Language selectedLanguage() const;

private:
    TRadioButtons* languageButtons_ {nullptr};
};
