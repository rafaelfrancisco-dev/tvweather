#define Uses_TDialog
#define Uses_TButton
#define Uses_TRect
#define Uses_TRadioButtons
#define Uses_TStaticText
#define Uses_TSItem
#include <tvision/tv.h>

#include "tvweather/OptionsDialog.h"
#include "tvweather/Localization.h"

namespace {

ushort languageIndex(tvweather::Language language)
{
    return language == tvweather::Language::Portuguese ? 1 : 0;
}

tvweather::Language languageFromIndex(ushort index)
{
    return index == 1 ? tvweather::Language::Portuguese
                      : tvweather::Language::English;
}

} // namespace

OptionsDialog::OptionsDialog(tvweather::Language language)
    : TWindowInit(TWindow::initFrame),
      TDialog(TRect(0, 0, 36, 13), tvweather::uiStrings(language).optionsDialogTitle)
{
    const auto& strings = tvweather::uiStrings(language);
    options |= ofCentered;

    insert(new TStaticText(TRect(3, 2, 20, 3), strings.languageLabel));

    languageButtons_ = new TRadioButtons(
        TRect(3, 4, 20, 6),
        new TSItem(strings.languageEnglish,
        new TSItem(strings.languagePortuguese, nullptr)));
    insert(languageButtons_);

    ushort initialLanguage = languageIndex(language);
    languageButtons_->setData(&initialLanguage);

    insert(new TButton(TRect(8, 9, 18, 11), strings.ok, cmOK, bfDefault));
    insert(new TButton(TRect(20, 9, 30, 11), strings.cancel, cmCancel, bfNormal));
}

tvweather::Language OptionsDialog::selectedLanguage() const
{
    ushort selected = 0;
    languageButtons_->getData(&selected);
    return languageFromIndex(selected);
}
