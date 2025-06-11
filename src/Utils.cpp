#include "Utils.h"

namespace dppc {
namespace utils {

// Geçersiz veya bilinmeyen konumun implementasyonu
const SourceLocation SourceLocation::Unknown("<unknown>", 0, 0); // Satır/Sütun 0, bilinmeyen/geçersiz konumu işaret edebilir

// SourceLocation nesnesini akışa yazdırmak için operatör aşırı yüklemesi
std::ostream& operator<<(std::ostream& os, const SourceLocation& loc) {
    if (loc == SourceLocation::Unknown) {
        os << "<unknown location>";
    } else {
        os << loc.Filename << ":" << loc.Line << ":" << loc.Column;
    }
    return os;
}

// MessageType enum'ını string'e çeviren yardımcı fonksiyon
const char* messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::Error:   return "error";
        case MessageType::Warning: return "warning";
        case MessageType::Info:    return "info";
        default:                   return "unknown";
    }
}

} // namespace utils
} // namespace dppc
