#ifndef DPP_COMPILER_UTILS_ERRORREPORTER_H
#define DPP_COMPILER_UTILS_ERRORREPORTER_H

#include "Utils.h" // SourceLocation ve MessageType için

#include <vector>
#include <string>
#include <iostream> // std::ostream için
#include <algorithm> // std::sort için

namespace dppc {
namespace utils {
namespace messaging {

// Tek bir hata/uyarı/bilgi mesajını temsil eden yapı
struct ErrorMessage {
    MessageType Type;         // Mesajın türü
    SourceLocation Location;  // Mesajla ilişkili kaynak kod konumu
    std::string Message;      // Mesaj metni

    // Constructor
    ErrorMessage(MessageType type, SourceLocation loc, std::string msg)
        : Type(type), Location(std::move(loc)), Message(std::move(msg)) {}

    // Karşılaştırma operatörü (konuma göre sıralamak için)
    bool operator<(const ErrorMessage& other) const {
        return Location < other.Location;
    }
};


// Hata Raporlayıcı Sınıfı
// Derleme sırasında oluşan tüm hata, uyarı ve bilgi mesajlarını yönetir.
class ErrorReporter {
private:
    std::vector<ErrorMessage> Messages; // Toplanan tüm mesajlar
    int ErrorCount;                     // Raporlanan hata sayısı
    int WarningCount;                   // Raporlanan uyarı sayısı
    std::ostream& Output;               // Mesajların yazılacağı çıktı akışı

    // Mesajları akışa yazdıran yardımcı fonksiyon
    void printMessage(const ErrorMessage& msg);


public:
    // Constructor
    ErrorReporter(std::ostream& output);

    // Hata, uyarı ve bilgi mesajları raporlama fonksiyonları
    void reportError(const SourceLocation& loc, const std::string& message);
    void reportWarning(const SourceLocation& loc, const std::string& message);
    void reportInfo(const SourceLocation& loc, const std::string& message);

    // Genel bir mesaj raporlama (özetler veya özel durumlar için)
    void reportMessage(const std::string& message, int count, MessageType type);

    // Toplanan mesajları konuma göre sıralayıp çıktıya yazar
    void reportSummary();

    // Hata raporlanıp raporlanmadığını kontrol eder
    bool hasErrors() const { return ErrorCount > 0; }

    // Hata ve uyarı sayılarını döndürür
    int getErrorCount() const { return ErrorCount; }
    int getWarningCount() const { return WarningCount; }

    // Toplanan mesajları ve sayıları sıfırlar
    void reset();
};

} // namespace messaging
} // namespace utils
} // namespace dppc

#endif // DPP_COMPILER_UTILS_ERRORREPORTER_H
