#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

enum class TranslationKey {
    HELLO,
    GOODBYE,
    WELCOME,
    ERROR,
    // Add more keys as needed
};

enum class Language {
    ENGLISH,
    JAPANESE,
    FRENCH,
    GERMAN,
    MANDARIN,
    KOREAN
};

class Localization {
public:
    Localization();
    void loadTranslations(Language lang);
    std::string getTranslation(TranslationKey key) const;
    void setLanguage(Language lang);

private:
    Language currentLanguage;
    std::unordered_map<TranslationKey, std::string> translations;
    
    static std::string getLanguageFileName(Language lang);
    static TranslationKey stringToTranslationKey(const std::string& str);
};