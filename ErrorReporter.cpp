#include "ErrorReporter.h"

namespace dppc {
namespace utils {
namespace messaging {

// ErrorReporter Constructor
ErrorReporter::ErrorReporter(std::ostream& output)
    : ErrorCount(0), WarningCount(0), Output(output)
{
     std::cerr << "DEBUG: ErrorReporter initialized." << std::endl; // Debug
}

// --- Mesaj Raporlama Fonksiyonları ---

void ErrorReporter::reportError(const SourceLocation& loc, const std::string& message) {
    Messages.emplace_back(MessageType::Error, loc, message);
    ErrorCount++;
    // İsteğe bağlı: Hata oluştuğunda hemen çıktıya yazdırılabilir
     printMessage(Messages.back());
}

void ErrorReporter::reportWarning(const SourceLocation& loc, const std::string& message) {
    Messages.emplace_back(MessageType::Warning, loc, message);
    WarningCount++;
    // İsteğe bağlı: Uyarı oluştuğunda hemen çıktıya yazdırılabilir
     printMessage(Messages.back());
}

void ErrorReporter::reportInfo(const SourceLocation& loc, const std::string& message) {
    Messages.emplace_back(MessageType::Info, loc, message);
    // Bilgi mesajları sayılmaz veya ayrı sayılabilir
    // İsteğe bağlı: Bilgi mesajı oluştuğunda hemen çıktıya yazdırılabilir
     printMessage(Messages.back());
}

// Genel bir mesaj raporlama (özetler veya özel durumlar için)
void ErrorReporter::reportMessage(const std::string& message, int count, MessageType type) {
     // Bu fonksiyon genellikle belirli bir konumla ilişkili olmayan özet mesajları için kullanılır
     // Örneğin: "Lexer aşaması başarılı." veya "3 hata bulundu."
     Output << message;
     if (count > 0) {
          Output << " (" << count << " " << messageTypeToString(type) << (count > 1 ? "s" : "") << ")";
     }
     Output << std::endl;
}


// --- Mesajları Yazdırma ve Özetleme ---

// Tek bir mesajı formatlayıp çıktıya yazdırır
void ErrorReporter::printMessage(const ErrorMessage& msg) {
    // Format: filename:line:column: type: message
    Output << msg.Location << ": " << messageTypeToString(msg.Type) << ": " << msg.Message << std::endl;
}

// Toplanan mesajları konuma göre sıralayıp çıktıya yazar ve özetler
void ErrorReporter::reportSummary() {
    if (Messages.empty()) {
        Output << "Derleme hatasız tamamlandı." << std::endl;
        return;
    }

    // Mesajları kaynak kod konumuna göre sırala
    std::sort(Messages.begin(), Messages.end());

    // Sıralanmış mesajları yazdır
    for (const auto& msg : Messages) {
        printMessage(msg);
    }

    // Özet raporunu yazdır
    Output << ErrorCount << (ErrorCount > 1 ? " hatalar" : " hata");
    Output << ", " << WarningCount << (WarningCount > 1 ? " uyarılar" : " uyarı");
    Output << " bulundu." << std::endl;
}

// Toplanan mesajları ve sayıları sıfırlar
void ErrorReporter::reset() {
    Messages.clear();
    ErrorCount = 0;
    WarningCount = 0;
}

} // namespace messaging
} // namespace utils
} // namespace dppc
