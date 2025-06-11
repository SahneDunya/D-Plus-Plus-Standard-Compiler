#ifndef DPP_COMPILER_LEXER_TOKEN_H
#define DPP_COMPILER_LEXER_TOKEN_H

#include <string>
#include <iostream> // Debug çıktıları için
#include <utility>  // std::move için

namespace dppc {
namespace lexer {

// Token'ın kaynak kodundaki konumu
struct SourceLocation {
    std::string filename;
    int line;
    int column;

    SourceLocation(std::string fn = "", int l = 1, int c = 1)
        : filename(std::move(fn)), line(l), column(c) {}

    // Konumu yazdırmak için helper
    friend std::ostream& operator<<(std::ostream& os, const SourceLocation& loc) {
        os << loc.filename << ":" << loc.line << ":" << loc.column;
        return os;
    }
};

// Token Türleri (TokenType)
// Dilinizdeki tüm anahtar kelimeler, operatörler, noktalama işaretleri, literaller vb. için birer tür.
enum class TokenType {
    // End of File
    EndOfFile,

    // Hata Durumu
    Error,

    // İdentifiers (Değişken adları, fonksiyon adları vb.)
    Identifier,

    // Literaller (Sabit değerler)
    IntegerLiteral,
    FloatingLiteral,
    StringLiteral,
    CharacterLiteral,
    BooleanLiteral, // true, false

    // Anahtar Kelimeler (D ve D++'a özgü olanlar)
    // D anahtar kelimeleri: auto, bool, break, case, cast, catch, class, const, continue, default, delegate, delete, do, else, enum, export, extern, false, final, finally, float, for, foreach, foreach_reverse, function, goto, if, immutable, import, in, inout, int, interface, invariant, is, lazy, short, long, super, switch, synchronized, template, this, throw, true, try, typeof, typeid, uint, ulong, union, unittest, ushort, version, void, volatile, while, with, __FILE__, __LINE__, __MODULE__, __PACKAGE__, __FUNCTION__, __PRETTY_FUNCTION__, __traits, __vector, __vec, body, debug, deprecated, new, override, package, pragma, scope, static, struct, mixin, enum_member, module, typeof, typeid // D'den bazıları
    // D++ eklenenler: match, trait, impl, fn, let, mut, ref, box, Result, Option // Rust'tan esinlenenler ve yeni tipler
    Kw_auto, Kw_bool, Kw_break, Kw_case, Kw_cast, Kw_catch, Kw_class, Kw_const, Kw_continue, Kw_default, Kw_delegate, Kw_delete, Kw_do, Kw_else, Kw_enum, Kw_export, Kw_extern, Kw_false, Kw_final, Kw_finally, Kw_float, Kw_for, Kw_foreach, Kw_foreach_reverse, Kw_function, Kw_goto, Kw_if, Kw_immutable, Kw_import, Kw_in, Kw_inout, Kw_int, Kw_interface, Kw_invariant, Kw_is, Kw_lazy, Kw_short, Kw_long, Kw_super, Kw_switch, Kw_synchronized, Kw_template, Kw_this, Kw_throw, Kw_true, Kw_try, Kw_typeof, Kw_typeid, Kw_uint, Kw_ulong, Kw_union, Kw_unittest, Kw_ushort, Kw_version, Kw_void, Kw_volatile, Kw_while, Kw_with, Kw_FILE, Kw_LINE, Kw_MODULE, Kw_PACKAGE, Kw_FUNCTION, Kw_PRETTY_FUNCTION, Kw_traits, Kw_vector, Kw_vec, Kw_body, Kw_debug, Kw_deprecated, Kw_new, Kw_override, Kw_package, Kw_pragma, Kw_scope, Kw_static, Kw_struct, Kw_mixin, Kw_enum_member, Kw_module,

    // D++'a Eklenen Anahtar Kelimeler ve Tipler
    Kw_match,   // Pattern matching için
    Kw_trait,   // Trait tanımlamak için
    Kw_impl,    // Trait implementasyonu veya inherent implementasyonlar için
    Kw_fn,      // Fonksiyon tanımlamak için (alternatif veya ek)
    Kw_let,     // Değişken tanımlamak için (Rust benzeri)
    Kw_mut,     // Değişkeni mutable yapmak için (Rust benzeri)
    Kw_ref,     // Referans tipi veya ref anahtar kelimesi
    Kw_box,     // Heap tahsis için (basit bir örnek)
    Kw_Result,  // Hata yönetimi tipi
    Kw_Option,  // Optional değer tipi


    // Operatörler
    Op_Plus,        // +
    Op_Minus,       // -
    Op_Star,        // *
    Op_Slash,       // /
    Op_Percent,     // %
    Op_Equal,       // =
    Op_EqualEqual,  // ==
    Op_NotEqual,    // !=
    Op_LessThan,    // <
    Op_GreaterThan, // >
    Op_LessEqual,   // <=
    Op_GreaterEqual,// >=
    Op_And,         // && (Mantıksal AND)
    Op_Or,          // || (Mantıksal OR)
    Op_Not,         // ! (Mantıksal NOT)
    Op_BitAnd,      // & (Bitwise AND)
    Op_BitOr,       // | (Bitwise OR)
    Op_BitXor,      // ^ (Bitwise XOR)
    Op_ShiftLeft,   // <<
    Op_ShiftRight,  // >>
    Op_Dot,         // .
    Op_Arrow,       // ->
    Op_DoubleColon, // :: (Path separator veya scope resolution)

    // Noktalama İşaretleri
    Punc_Semicolon, // ;
    Punc_Colon,     // :
    Punc_Comma,     // ,
    Punc_OpenParen, // (
    Punc_CloseParen,// )
    Punc_OpenBrace, // {
    Punc_CloseBrace,// }
    Punc_OpenBracket,// [
    Punc_CloseBracket,// ]

    // ... diğer operatörler ve noktalama işaretleri eklenebilir
    // Örneğin: ++, --, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
    // ?, ?? (D'den), ... (variadic veya range)
    // @ (attribute), # (preprocessor veya macro), $ (lambda/macro), ` (alias/mixin)
    // Tilda ~ (destructor veya cat)
    // <> (template arguments)
    // # (version/debug conditionals)
    // __LINE__ vb. önceden tanımlı sabitler
};

// TokenType'ı stringe çevirme (Debug için faydalı)
const char* tokenTypeToString(TokenType type);

// Token yapısı
struct Token {
    TokenType type;
    std::string value; // Identifier adı, literal değeri string olarak saklanır
    SourceLocation loc; // Token'ın kaynak dosyadaki konumu

    Token(TokenType t, std::string val, SourceLocation l)
        : type(t), value(std::move(val)), loc(std::move(l)) {}

    Token(TokenType t, SourceLocation l)
        : type(t), value(""), loc(std::move(l)) {} // Değeri olmayan tokenlar için (operatörler, noktalama)

    // Token'ı yazdırmak için helper
    friend std::ostream& operator<<(std::ostream& os, const Token& token) {
        os << "Token(" << tokenTypeToString(token.type);
        if (!token.value.empty()) {
            os << ", \"" << token.value << "\"";
        }
        os << ", " << token.loc << ")";
        return os;
    }
};

} // namespace lexer
} // namespace dppc

#endif // DPP_COMPILER_LEXER_TOKEN_H
