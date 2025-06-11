#ifndef DPP_COMPILER_LEXER_LEXER_H
#define DPP_COMPILER_LEXER_LEXER_H

#include "Token.h"
#include "../utils/ErrorReporter.h" // Hata raporlama için

#include <istream>
#include <string>
#include <map>

namespace dppc {
namespace lexer {

class Lexer {
private:
    std::istream& Input; // Okunacak girdi akımı
    std::string Filename; // Kaynak dosya adı
    ErrorReporter& Reporter; // Hata raporlama nesnesi

    char CurrentChar; // Şu anda işlenen karakter
    SourceLocation CurrentLoc; // Şu anki pozisyon

    // Anahtar kelimeleri hızlıca kontrol etmek için bir map
    std::map<std::string, TokenType> Keywords;

    // Yardımcı Fonksiyonlar
    char readChar(); // Sonraki karakteri okur ve pozisyonu günceller
    char peekChar(); // Sonraki karakteri okumadan bakar
    bool isAtEOF() const; // Dosya sonuna gelip gelmediğini kontrol eder

    void skipWhitespaceAndComments(); // Boşlukları ve yorumları atlar
    void skipSingleLineComment();   // Tek satırlık yorumu atlar (//)
    void skipMultiLineComment();    // Çok satırlı yorumu atlar (/* ... */)

    // Token türlerine göre lexing fonksiyonları
    Token lexIdentifier(); // Identifier veya anahtar kelime okur
    Token lexNumber();     // Sayı (integer veya float) okur
    Token lexString();     // String literal okur ("...")
    Token lexCharLiteral();// Karakter literal okur ('...')
    Token lexOperatorOrPunctuation(); // Operatör veya noktalama işaretini okur

    void error(const std::string& message); // Hata raporlayıcıya hata iletmek için

public:
    // Constructor
    Lexer(std::istream& input, const std::string& filename, ErrorReporter& reporter);

    // Sonraki token'ı almak için ana fonksiyon
    Token getNextToken();
};

} // namespace lexer
} // namespace dppc

#endif // DPP_COMPILER_LEXER_LEXER_H
