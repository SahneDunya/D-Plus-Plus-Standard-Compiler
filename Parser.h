#ifndef DPP_COMPILER_PARSER_PARSER_H
#define DPP_COMPILER_PARSER_PARSER_H

#include "../lexer/Lexer.h" // Lexer sınıfı ve Token için
#include "../utils/ErrorReporter.h" // Hata raporlama için
#include "AST.h" // AST düğümleri için

#include <vector>
#include <string>
#include <memory>
#include <map>

namespace dppc {
namespace parser {

// İkili operatörlerin öncelik seviyeleri
// Değer ne kadar büyükse öncelik o kadar yüksektir.
static std::map<lexer::TokenType, int> BinOpPrecedence;

// Öncelik tablosunu başlatmak için fonksiyon
static void InitializeBinOpPrecedence();


class Parser {
private:
    lexer::Lexer& Lexer; // Token almak için lexer
    ErrorReporter& Reporter; // Hata raporlama

    lexer::Token CurrentToken; // Şu anda işlenen token

    // Yardımcı Fonksiyonlar
    lexer::Token consumeToken(); // Mevcut token'ı tüketir ve bir sonraki token'ı alır
    const lexer::Token& getCurrentToken() const; // Mevcut token'a bakar
    bool peekToken(lexer::TokenType type); // Mevcut token'ın türünü kontrol eder
    bool consumeIf(lexer::TokenType type); // Mevcut token beklenen türdeyse tüketir ve true döner, aksi halde false
    void expect(lexer::TokenType type, const std::string& errorMessage); // Mevcut token beklenen türde değilse hata verir ve tüketir

    void parseError(const std::string& message); // Hata raporlayıcıya hata iletmek için

    // Hata sonrası toparlanma (basit implementasyon)
    void synchronize();

    // Gramer Kurallarına Karşılık Gelen Ayrıştırma Fonksiyonları
    // Bu fonksiyonlar, dilin gramerine göre token akışını ayrıştırır ve AST düğümleri döndürür.

    std::unique_ptr<ast::DeclAST> parseDeclaration(); // Üst düzey bildirimleri ayrıştırır (fonksiyon, struct, trait vb.)
    std::unique_ptr<ast::StmtAST> parseStatement();   // Deyimleri ayrıştırır (if, while, let, return, ifade-deyimi vb.)
    std::unique_ptr<ast::ExprAST> parseExpression();  // Genel ifadeleri ayrıştırır (öncelik tabanlı)

    // Deyim Ayrıştırma Fonksiyonları
    std::unique_ptr<ast::StmtAST> parseBlockStmt();     // Blokları ayrıştırır ({ ... })
    std::unique_ptr<ast::StmtAST> parseIfStmt();        // If deyimlerini ayrıştırır
    std::unique_ptr<ast::StmtAST> parseWhileStmt();     // While döngülerini ayrıştırır
    std::unique_ptr<ast::StmtAST> parseForStmt();       // For döngülerini ayrıştırır
    std::unique_ptr<ast::StmtAST> parseReturnStmt();    // Return deyimlerini ayrıştırır
    std::unique_ptr<ast::StmtAST> parseBreakStmt();     // Break deyimlerini ayrıştırır
    std::unique_ptr<ast::StmtAST> parseContinueStmt();  // Continue deyimlerini ayrıştırır
    std::unique_ptr<ast::StmtAST> parseLetStmt();       // Let deyimlerini ayrıştırır (D++'a özgü)
    // D'ye özgü diğer deyimler (switch, do-while, foreach vb.) buraya eklenebilir.


    // İfade Ayrıştırma Fonksiyonları (Öncelik tabanlı parsingle beraber kullanılır)
    std::unique_ptr<ast::ExprAST> parsePrimaryExpr(); // En temel ifadeleri ayrıştırır (literal, identifier, parantezli ifade vb.)
    std::unique_ptr<ast::ExprAST> parseIdentifierExpr(); // Identifier ifadelerini ayrıştırır
    std::unique_ptr<ast::ExprAST> parseNumberExpr();     // Sayısal literal ifadeleri ayrıştırır
    std::unique_ptr<ast::ExprAST> parseStringExpr();     // String literal ifadeleri ayrıştırır
    std::unique_ptr<ast::ExprAST> parseBooleanExpr();    // Boolean literal ifadeleri ayrıştırır
    std::unique_ptr<ast::ExprAST> parseCallExpr(std::unique_ptr<ast::ExprAST> callee); // Fonksiyon çağrısı ifadelerini ayrıştırır
    std::unique_ptr<ast::ExprAST> parseUnaryOpExpr();    // Tekli operatör ifadelerini ayrıştırır
    std::unique_ptr<ast::ExprAST> parseBinaryOpExpr(int precedence, std::unique_ptr<ast::ExprAST> left); // İkili operatör ifadelerini önceliğe göre ayrıştırır
    std::unique_ptr<ast::ExprAST> parseMemberAccessOrCall(std::unique_ptr<ast::ExprAST> base); // . veya -> ile üye erişimi veya fonksiyon çağrısını ayrıştırır
    std::unique_ptr<ast::ExprAST> parseArrayAccess(std::unique_ptr<ast::ExprAST> base); // [] ile dizi erişimini ayrıştırır
     std::unique_ptr<ast::ExprAST> parseNewExpr(); // new ifadesini ayrıştırır

    // D++'a Özgü İfade Ayrıştırma
    std::unique_ptr<ast::ExprAST> parseMatchExpr(); // Match ifadesini ayrıştırır


    // Desen (Pattern) Ayrıştırma Fonksiyonları (Match ifadeleri için)
    std::unique_ptr<ast::PatternAST> parsePattern(); // Genel deseni ayrıştırır
    std::unique_ptr<ast::PatternAST> parseWildcardPattern(); // _ deseni
    std::unique_ptr<ast::PatternAST> parseIdentifierPattern(); // Identifier deseni (x, mut x)
    std::unique_ptr<ast::PatternAST> parseLiteralPattern(); // Literal deseni (1, "hello", true)
    std::unique_ptr<ast::PatternAST> parseStructDeconstructionPattern(const std::string& typeName); // Yapı deconstruction deseni
    std::unique_ptr<ast::PatternAST> parseEnumVariantPattern(const std::string& enumName, const std::string& variantName); // Enum varyant deseni
    std::unique_ptr<ast::PatternAST> parseTuplePattern(); // Tuple deseni
    std::unique_ptr<ast::PatternAST> parseReferencePattern(); // Referans deseni (&pat, &mut pat)
    // TODO: parseRangePattern(); // Range deseni (1..5)

    // Match Kolu (Arm) Ayrıştırma
    ast::MatchExprAST::MatchArm parseMatchArm();


    // Bildirim Ayrıştırma Fonksiyonları
    std::unique_ptr<ast::FunctionDeclAST> parseFunctionDecl(); // Fonksiyon bildirimini/tanımını ayrıştırır
    std::unique_ptr<ast::ParameterAST> parseParameter();     // Fonksiyon parametresini ayrıştırır
    std::vector<std::unique_ptr<ast::ParameterAST>> parseParameterList(); // Parametre listesini ayrıştırır
    std::unique_ptr<ast::StructDeclAST> parseStructDecl();   // Yapı (struct) bildirimini ayrıştırır
    std::unique_ptr<ast::StructFieldAST> parseStructField(); // Yapı alanını ayrıştırır
    std::unique_ptr<ast::TraitDeclAST> parseTraitDecl();     // Trait bildirimini ayrıştırır (D++'a özgü)
    std::unique_ptr<ast::ImplDeclAST> parseImplDecl();       // Implementasyon (impl) bildirimini ayrıştırır (D++'a özgü)
    std::unique_ptr<ast::EnumDeclAST> parseEnumDecl();       // Enum bildirimini ayrıştırır
    std::unique_ptr<ast::EnumVariantAST> parseEnumVariant(); // Enum varyantını ayrıştırır
    // D'ye özgü diğer bildirimler (class, interface, union, alias, module vb.) buraya eklenebilir.


    // Tip Ayrıştırma Fonksiyonları
    std::unique_ptr<ast::TypeAST> parseType();       // Genel tipi ayrıştırır
    std::unique_ptr<ast::TypeAST> parsePrimitiveType(); // İlkel tipi ayrıştırır (int, float vb.)
    std::unique_ptr<ast::TypeAST> parseIdentifierType(); // Identifier tipi ayrıştırır (kullanıcı tanımlı)
    std::unique_ptr<ast::TypeAST> parsePointerType(); // Pointer tipi ayrıştırır (*T)
    std::unique_ptr<ast::TypeAST> parseReferenceType(); // Referans tipi ayrıştırır (&T, &mut T)
    std::unique_ptr<ast::TypeAST> parseArrayType(); // Dizi tipi ayrıştırır ([T] veya [T; N])
    std::unique_ptr<ast::TypeAST> parseTupleType(); // Tuple tipi ayrıştırır ((T1, T2))
    std::unique_ptr<ast::TypeAST> parseOptionType(); // Option<T> tipini ayrıştırır (D++'a özgü)
    std::unique_ptr<ast::TypeAST> parseResultType(); // Result<T, E> tipini ayrıştırır (D++'a özgü)
    // TODO: parseFunctionType(), parseDelegateType(), parseTraitObjectType() vb.


public:
    // Constructor
    Parser(lexer::Lexer& lexer, ErrorReporter& reporter);

    // Kaynak dosyanın tamamını ayrıştırmak için ana fonksiyon
    std::unique_ptr<ast::SourceFileAST> parseSourceFile();
};

} // namespace parser
} // namespace dppc

#endif // DPP_COMPILER_PARSER_PARSER_H
