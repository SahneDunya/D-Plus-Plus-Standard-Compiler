#ifndef DPP_COMPILER_UTILS_H
#define DPP_COMPILER_UTILS_H

#include <string>
#include <iostream> // std::ostream için

namespace dppc {
namespace utils {

// Kaynak Kod Konumu
struct SourceLocation {
    std::string Filename; // Dosya adı
    int Line;             // Satır numarası (1'den başlar)
    int Column;           // Sütun numarası (1'den başlar)

    // Constructor
    SourceLocation(std::string filename = "<unknown>", int line = 1, int column = 1)
        : Filename(std::move(filename)), Line(line), Column(column) {}

    // Geçersiz veya bilinmeyen konumu temsil eden statik üye
    static const SourceLocation Unknown;

    // Karşılaştırma operatörleri (örneğin hata mesajlarını sıralamak için kullanılabilir)
    bool operator<(const SourceLocation& other) const {
        if (Filename != other.Filename) return Filename < other.Filename;
        if (Line != other.Line) return Line < other.Line;
        return Column < other.Column;
    }

    bool operator==(const SourceLocation& other) const {
        return Filename == other.Filename && Line == other.Line && Column == other.Column;
    }

    bool operator!=(const SourceLocation& other) const {
        return !(*this == other);
    }
};

// SourceLocation nesnesini akışa yazdırmak için operatör aşırı yüklemesi
std::ostream& operator<<(std::ostream& os, const SourceLocation& loc);

// Mesaj Türleri (Error Reporter tarafından kullanılır)
enum class MessageType {
    Error,   // Hata - Derlemeyi durdurur veya başarısız sayar
    Warning, // Uyarı - Derleme devam edebilir, potansiyel sorun
    Info     // Bilgi - Kullanıcıya bilgi veren mesaj
};

// MessageType enum'ını string'e çeviren yardımcı fonksiyon
const char* messageTypeToString(MessageType type);


} // namespace utils
} // namespace dppc

#endif // DPP_COMPILER_UTILS_H
