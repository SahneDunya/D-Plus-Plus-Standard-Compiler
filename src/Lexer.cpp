#include "Lexer.h"
#include <cctype>   // isalpha, isdigit, isalnum vb. için
#include <string>
#include <iostream> // Debug için
#include <map>

namespace dppc {
namespace lexer {

// TokenType'ı stringe çevirme implementasyonu
const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::EndOfFile: return "EndOfFile";
        case TokenType::Error: return "Error";
        case TokenType::Identifier: return "Identifier";
        case TokenType::IntegerLiteral: return "IntegerLiteral";
        case TokenType::FloatingLiteral: return "FloatingLiteral";
        case TokenType::StringLiteral: return "StringLiteral";
        case TokenType::CharacterLiteral: return "CharacterLiteral";
        case TokenType::BooleanLiteral: return "BooleanLiteral";

        // Anahtar kelimeler (Tam listeyi buraya eklemeniz gerekir)
        case TokenType::Kw_auto: return "Kw_auto";
        case TokenType::Kw_bool: return "Kw_bool";
        // ... diğer D anahtar kelimeleri
        case TokenType::Kw_match: return "Kw_match";
        case TokenType::Kw_trait: return "Kw_trait";
        case TokenType::Kw_impl: return "Kw_impl";
        case TokenType::Kw_fn: return "Kw_fn";
        case TokenType::Kw_let: return "Kw_let";
        case TokenType::Kw_mut: return "Kw_mut";
        case TokenType::Kw_ref: return "Kw_ref";
        case TokenType::Kw_box: return "Kw_box";
        case TokenType::Kw_Result: return "Kw_Result";
        case TokenType::Kw_Option: return "Kw_Option";
        // ... diğer D++ anahtar kelimeleri

        // Operatörler (Tam listeyi buraya eklemeniz gerekir)
        case TokenType::Op_Plus: return "Op_Plus";
        case TokenType::Op_Minus: return "Op_Minus";
        case TokenType::Op_Star: return "Op_Star";
        case TokenType::Op_Slash: return "Op_Slash";
        case TokenType::Op_Percent: return "Op_Percent";
        case TokenType::Op_Equal: return "Op_Equal";
        case TokenType::Op_EqualEqual: return "Op_EqualEqual";
        case TokenType::Op_NotEqual: return "Op_NotEqual";
        case TokenType::Op_LessThan: return "Op_LessThan";
        case TokenType::Op_GreaterThan: return "Op_GreaterThan";
        case TokenType::Op_LessEqual: return "Op_LessEqual";
        case TokenType::Op_GreaterEqual: return "Op_GreaterEqual";
        case TokenType::Op_And: return "Op_And";
        case TokenType::Op_Or: return "Op_Or";
        case TokenType::Op_Not: return "Op_Not";
        case TokenType::Op_BitAnd: return "Op_BitAnd";
        case TokenType::Op_BitOr: return "Op_BitOr";
        case TokenType::Op_BitXor: return "Op_BitXor";
        case TokenType::Op_ShiftLeft: return "Op_ShiftLeft";
        case TokenType::Op_ShiftRight: return "Op_ShiftRight";
        case TokenType::Op_Dot: return "Op_Dot";
        case TokenType::Op_Arrow: return "Op_Arrow";
        case TokenType::Op_DoubleColon: return "Op_DoubleColon";

        // Noktalama İşaretleri (Tam listeyi buraya eklemeniz gerekir)
        case TokenType::Punc_Semicolon: return "Punc_Semicolon";
        case TokenType::Punc_Colon: return "Punc_Colon";
        case TokenType::Punc_Comma: return "Punc_Comma";
        case TokenType::Punc_OpenParen: return "Punc_OpenParen";
        case TokenType::Punc_CloseParen: return "Punc_CloseParen";
        case TokenType::Punc_OpenBrace: return "Punc_OpenBrace";
        case TokenType::Punc_CloseBrace: return "Punc_CloseBrace";
        case TokenType::Punc_OpenBracket: return "Punc_OpenBracket";
        case TokenType::Punc_CloseBracket: return "Punc_CloseBracket";

        default: return "Unknown"; // Bilinmeyen türler için
    }
}


// Lexer Constructor
Lexer::Lexer(std::istream& input, const std::string& filename, ErrorReporter& reporter)
    : Input(input), Filename(filename), Reporter(reporter), CurrentChar('\0'), CurrentLoc(filename, 1, 0) // Başlangıç pozisyonu
{
    // Anahtar Kelimeleri Map'e Ekle
    // D anahtar kelimeleri (eksik olabilir, tamamlayın)
    Keywords["auto"] = TokenType::Kw_auto;
    Keywords["bool"] = TokenType::Kw_bool;
    Keywords["break"] = TokenType::Kw_break;
    Keywords["case"] = TokenType::Kw_case;
    Keywords["catch"] = TokenType::Kw_catch;
    Keywords["class"] = TokenType::Kw_class;
    Keywords["const"] = TokenType::Kw_const;
    Keywords["continue"] = TokenType::Kw_continue;
    Keywords["default"] = TokenType::Kw_default;
    Keywords["delegate"] = TokenType::Kw_delegate;
    Keywords["delete"] = TokenType::Kw_delete;
    Keywords["do"] = TokenType::Kw_do;
    Keywords["else"] = TokenType::Kw_else;
    Keywords["enum"] = TokenType::Kw_enum;
    Keywords["export"] = TokenType::Kw_export;
    Keywords["extern"] = TokenType::Kw_extern;
    Keywords["false"] = TokenType::Kw_false;
    Keywords["final"] = TokenType::Kw_final;
    Keywords["finally"] = TokenType::Kw_finally;
    Keywords["float"] = TokenType::Kw_float;
    Keywords["for"] = TokenType::Kw_for;
    Keywords["foreach"] = TokenType::Kw_foreach;
    Keywords["foreach_reverse"] = TokenType::Kw_foreach_reverse;
    Keywords["function"] = TokenType::Kw_function;
    Keywords["goto"] = TokenType::Kw_goto;
    Keywords["if"] = TokenType::Kw_if;
    Keywords["immutable"] = TokenType::Kw_immutable;
    Keywords["import"] = TokenType::Kw_import;
    Keywords["in"] = TokenType::Kw_in;
    Keywords["inout"] = TokenType::Kw_inout;
    Keywords["int"] = TokenType::Kw_int;
    Keywords["interface"] = TokenType::Kw_interface;
    Keywords["invariant"] = TokenType::Kw_invariant;
    Keywords["is"] = TokenType::Kw_is;
    Keywords["lazy"] = TokenType::Kw_lazy;
    Keywords["short"] = TokenType::Kw_short;
    Keywords["long"] = TokenType::Kw_long;
    Keywords["super"] = TokenType::Kw_super;
    Keywords["switch"] = TokenType::Kw_switch;
    Keywords["synchronized"] = TokenType::Kw_synchronized;
    Keywords["template"] = TokenType::Kw_template;
    Keywords["this"] = TokenType::Kw_this;
    Keywords["throw"] = TokenType::Kw_throw;
    Keywords["true"] = TokenType::Kw_true;
    Keywords["try"] = TokenType::Kw_try;
    Keywords["typeof"] = TokenType::Kw_typeof; // D'de hem keyword hem operator gibi
    Keywords["typeid"] = TokenType::Kw_typeid; // D'de hem keyword hem operator gibi
    Keywords["uint"] = TokenType::Kw_uint;
    Keywords["ulong"] = TokenType::Kw_ulong;
    Keywords["union"] = TokenType::Kw_union;
    Keywords["unittest"] = TokenType::Kw_unittest;
    Keywords["ushort"] = TokenType::Kw_ushort;
    Keywords["version"] = TokenType::Kw_version;
    Keywords["void"] = TokenType::Kw_void;
    Keywords["volatile"] = TokenType::Kw_volatile;
    Keywords["while"] = TokenType::Kw_while;
    Keywords["with"] = TokenType::Kw_with;
    Keywords["__FILE__"] = TokenType::Kw_FILE;
    Keywords["__LINE__"] = TokenType::Kw_LINE;
    Keywords["__MODULE__"] = TokenType::Kw_MODULE;
    Keywords["__PACKAGE__"] = TokenType::Kw_PACKAGE;
    Keywords["__FUNCTION__"] = TokenType::Kw_FUNCTION;
    Keywords["__PRETTY_FUNCTION__"] = TokenType::Kw_PRETTY_FUNCTION;
    Keywords["__traits"] = TokenType::Kw_traits;
    Keywords["__vector"] = TokenType::Kw_vector; // D'de yok olabilir, kontrol edin
    Keywords["__vec"] = TokenType::Kw_vec;       // D'de yok olabilir, kontrol edin
    Keywords["body"] = TokenType::Kw_body;
    Keywords["debug"] = TokenType::Kw_debug;
    Keywords["deprecated"] = TokenType::Kw_deprecated;
    Keywords["new"] = TokenType::Kw_new;
    Keywords["override"] = TokenType::Kw_override;
    Keywords["package"] = TokenType::Kw_package;
    Keywords["pragma"] = TokenType::Kw_pragma;
    Keywords["scope"] = TokenType::Kw_scope;
    Keywords["static"] = TokenType::Kw_static;
    Keywords["struct"] = TokenType::Kw_struct;
    Keywords["mixin"] = TokenType::Kw_mixin;
    Keywords["enum_member"] = TokenType::Kw_enum_member; // Özel durum
    Keywords["module"] = TokenType::Kw_module;


    // D++'a Eklenen Anahtar Kelimeler
    Keywords["match"] = TokenType::Kw_match;
    Keywords["trait"] = TokenType::Kw_trait;
    Keywords["impl"] = TokenType::Kw_impl;
    Keywords["fn"] = TokenType::Kw_fn;
    Keywords["let"] = TokenType::Kw_let;
    Keywords["mut"] = TokenType::Kw_mut;
    Keywords["ref"] = TokenType::Kw_ref;
    Keywords["box"] = TokenType::Kw_box;
    Keywords["Result"] = TokenType::Kw_Result; // Tip isimleri de anahtar kelime olabilir
    Keywords["Option"] = TokenType::Kw_Option; // Tip isimleri de anahtar kelime olabilir


    // İlk karakteri oku
    readChar();
}

// Sonraki karakteri oku ve pozisyonu güncelle
char Lexer::readChar() {
    char lastChar = CurrentChar;
    if (Input.get(CurrentChar)) {
        // Karakter başarıyla okundu
        if (lastChar == '\n') {
            CurrentLoc.line++;
            CurrentLoc.column = 1; // Yeni satır başladı, sütun 1
        } else {
            CurrentLoc.column++;
        }
         std::cout << "Read: '" << CurrentChar << "' at " << CurrentLoc << std::endl; // Debug
    } else {
        // Okuma başarısız, dosya sonu veya hata
        CurrentChar = EOF; // EOF olarak işaretle
         std::cout << "EOF reached at " << CurrentLoc << std::endl; // Debug
    }
    return CurrentChar;
}

// Sonraki karakteri okumadan bak
char Lexer::peekChar() {
    return Input.peek();
}

// Dosya sonuna gelip gelmediğini kontrol et
bool Lexer::isAtEOF() const {
    return CurrentChar == EOF;
}

// Hata raporlayıcıya hata ilet
void Lexer::error(const std::string& message) {
    Reporter.reportError(CurrentLoc, message);
}

// Boşlukları ve yorumları atla
void Lexer::skipWhitespaceAndComments() {
    while (true) {
        // Boşlukları atla
        while (std::isspace(CurrentChar)) {
            readChar();
        }

        // Yorumları kontrol et
        if (CurrentChar == '/') {
            char next = peekChar();
            if (next == '/') {
                skipSingleLineComment();
                continue; // Yorumdan sonra tekrar boşluk ve yorum kontrolü yap
            } else if (next == '*') {
                skipMultiLineComment();
                continue; // Yorumdan sonra tekrar boşluk ve yorum kontrolü yap
            }
        }
        // Başka atlanacak bir şey yoksa döngüyü kır
        break;
    }
}

// Tek satırlık yorumu atla (//)
void Lexer::skipSingleLineComment() {
    // İlk '/' zaten okundu, ikinci '/' bekleniyor
    readChar(); // '/' karakterini oku
    readChar(); // İkinci '/' karakterini oku

    // Satır sonuna veya dosya sonuna kadar oku
    while (CurrentChar != '\n' && !isAtEOF()) {
        readChar();
    }
    // Satır sonu karakteri veya EOF okunduğunda döngü biter.
    // readChar zaten pozisyonu bir sonraki satıra veya EOF'a ayarlar.
}

// Çok satırlı yorumu atla (/* ... */)
void Lexer::skipMultiLineComment() {
    // İlk '/' zaten okundu, '*' bekleniyor
    readChar(); // '/' karakterini oku
    readChar(); // '*' karakterini oku

    // '*/' dizisini bulana kadar oku
    while (true) {
        if (isAtEOF()) {
            error("Yorum kapatılmadı.");
            return;
        }
        if (CurrentChar == '*') {
            if (peekChar() == '/') {
                readChar(); // '*' karakterini oku
                readChar(); // '/' karakterini oku
                return; // Yorum bitti
            }
        }
        readChar(); // Yorum karakterini oku
    }
}

// Identifier veya anahtar kelime okur
Token Lexer::lexIdentifier() {
    SourceLocation startLoc = CurrentLoc;
    std::string value;
    while (std::isalnum(CurrentChar) || CurrentChar == '_') {
        value += CurrentChar;
        readChar();
    }

    // Okunan identifier bir anahtar kelime mi?
    if (Keywords.count(value)) {
        return Token(Keywords[value], startLoc); // Anahtar kelimenin türünü döndür
    } else {
        return Token(TokenType::Identifier, value, startLoc); // Identifier türünü ve değerini döndür
    }
}

// Sayı (integer veya float) okur
Token Lexer::lexNumber() {
    SourceLocation startLoc = CurrentLoc;
    std::string value;
    bool isFloat = false;

    // Tam sayı kısmını oku
    while (std::isdigit(CurrentChar)) {
        value += CurrentChar;
        readChar();
    }

    // Ondalık kısmı kontrol et
    if (CurrentChar == '.' && std::isdigit(peekChar())) {
        isFloat = true;
        value += CurrentChar; // '.' karakterini ekle
        readChar(); // '.' karakterini oku
        // Ondalık basamakları oku
        while (std::isdigit(CurrentChar)) {
            value += CurrentChar;
            readChar();
        }
    }

    // Üstel kısmı kontrol et (örn: 1.2e+3)
    if ((CurrentChar == 'e' || CurrentChar == 'E') &&
        (std::isdigit(peekChar()) || (peekChar() == '+' || peekChar() == '-') && std::isdigit(Input.peekg()))) {
        isFloat = true;
        value += CurrentChar; // 'e' veya 'E' karakterini ekle
        readChar(); // 'e' veya 'E' karakterini oku

        // İşareti kontrol et (+ veya -)
        if (CurrentChar == '+' || CurrentChar == '-') {
             value += CurrentChar;
             readChar();
        }
        // Üstel basamakları oku
        while (std::isdigit(CurrentChar)) {
            value += CurrentChar;
            readChar();
        }
    }

    // Sayı türüne göre token oluştur
    if (isFloat) {
        return Token(TokenType::FloatingLiteral, value, startLoc);
    } else {
        // Integer suffix'leri (örn: 123u, 456L) burada işlenebilir
        return Token(TokenType::IntegerLiteral, value, startLoc);
    }
}

// String literal okur ("...")
Token Lexer::lexString() {
    SourceLocation startLoc = CurrentLoc;
    std::string value;

    readChar(); // Açılış tırnak işaretini (") atla

    while (CurrentChar != '"' && !isAtEOF()) {
        if (CurrentChar == '\\') {
            // Escape sequence (\n, \t, \\, \", \', \uXXXX vb.)
            readChar(); // '\\' karakterini atla
            char escapedChar;
            switch (CurrentChar) {
                case 'n': escapedChar = '\n'; break;
                case 't': escapedChar = '\t'; break;
                case 'r': escapedChar = '\r'; break;
                case '\\': escapedChar = '\\'; break;
                case '"': escapedChar = '"'; break;
                case '\'': escapedChar = '\''; break;
                // TODO: Diğer escape sequence'ler (\uXXXX gibi Unicode kaçışları)
                default:
                    error("Geçersiz kaçış dizisi: \\" + std::string(1, CurrentChar));
                    // Hata sonrası lexing'e devam etmeye çalışabiliriz veya bir hata token'ı döndürebiliriz.
                    // Şimdilik bu karakteri string'e ekleyip devam edelim, hata Parser/Sema aşamasında da yakalanabilir.
                    value += '\\';
                    escapedChar = CurrentChar;
                    break;
            }
             value += escapedChar;
        } else {
            value += CurrentChar;
        }
        readChar();
    }

    if (isAtEOF()) {
        error("String literal kapatılmadı.");
        return Token(TokenType::Error, startLoc); // Hata token'ı döndür
    }

    readChar(); // Kapanış tırnak işaretini (") atla
    return Token(TokenType::StringLiteral, value, startLoc);
}

// Karakter literal okur ('...')
Token Lexer::lexCharLiteral() {
     SourceLocation startLoc = CurrentLoc;
     std::string value; // Tek karakter veya escape sequence

     readChar(); // Açılış tek tırnak işaretini (') atla

     if (isAtEOF()) {
         error("Karakter literal kapatılmadı.");
         return Token(TokenType::Error, startLoc);
     }

     if (CurrentChar == '\\') {
         // Escape sequence (\n, \t, \\, \", \', \uXXXX vb.)
         readChar(); // '\\' karakterini atla
         char escapedChar;
         switch (CurrentChar) {
             case 'n': escapedChar = '\n'; break;
             case 't': escapedChar = '\t'; break;
             case 'r': escapedChar = '\r'; break;
             case '\\': escapedChar = '\\'; break;
             case '"': escapedChar = '"'; break;
             case '\'': escapedChar = '\''; break;
             // TODO: Diğer escape sequence'ler
             default:
                 error("Geçersiz kaçış dizisi karakter literalinde: \\" + std::string(1, CurrentChar));
                 // Hata sonrası devam et
                 escapedChar = CurrentChar; // Hatalı karakteri ekleyip devam et
                 break;
         }
          value += escapedChar;
     } else {
         value += CurrentChar;
     }
     readChar(); // Karakteri oku

     if (CurrentChar != '\'') {
         error("Karakter literal beklenen tek karakter veya kaçış dizisinden sonra kapatılmadı.");
         // Hata sonrası ' karakterini bulana veya satır sonuna kadar atlayabiliriz.
         while (CurrentChar != '\'' && CurrentChar != '\n' && !isAtEOF()) {
             readChar();
         }
         if (CurrentChar == '\'') readChar(); // Kapatan ' varsa onu da oku
         return Token(TokenType::Error, startLoc);
     }

     readChar(); // Kapanış tek tırnak işaretini (') atla
     return Token(TokenType::CharacterLiteral, value, startLoc);
}


// Operatör veya noktalama işaretini okur
Token Lexer::lexOperatorOrPunctuation() {
    SourceLocation startLoc = CurrentLoc;
    char firstChar = CurrentChar;
    std::string value(1, firstChar); // Tek karakterlik değeri başlat

    // Karakteri oku ve ilerle
    readChar();

    // İkinci karaktere bakarak birden fazla karakterli operatörleri kontrol et
    char nextChar = peekChar();

    switch (firstChar) {
        case '+':
            // Eğer ikinci karakter '+' ise '++' token'ını işle
            // Eğer ikinci karakter '=' ise '+=' token'ını işle
            // Aksi halde sadece '+' token'ını işle
             if (nextChar == '+') { value += readChar(); return Token(TokenType::Op_Plus, startLoc); } // Örnek: ++
             if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_Plus, startLoc); } // Örnek: +=
             return Token(TokenType::Op_Plus, startLoc); // Tek karakter '+'
        case '-':
             if (nextChar == '>') { value += readChar(); return Token(TokenType::Op_Arrow, startLoc); } // ->
             if (nextChar == '-') { value += readChar(); return Token(TokenType::Op_Minus, startLoc); } // --
             if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_Minus, startLoc); } // -=
             return Token(TokenType::Op_Minus, startLoc);
        case '*':
             if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_Star, startLoc); } // *=
             return Token(TokenType::Op_Star, startLoc);
        case '/':
             if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_Slash, startLoc); } // /=
             // Yorumlar burada zaten skipWhitespaceAndComments içinde işlenir,
             // bu kısım sadece operatör '/' içindir.
             return Token(TokenType::Op_Slash, startLoc);
        case '%':
             if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_Percent, startLoc); } // %=
             return Token(TokenType::Op_Percent, startLoc);
        case '=':// =, ==
            if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_EqualEqual, startLoc); }
            return Token(TokenType::Op_Equal, startLoc);
        case '!': // !, !=
            if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_NotEqual, startLoc); }
            return Token(TokenType::Op_Not, startLoc);
        case '<': // <, <=, <<, <<=
            if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_LessEqual, startLoc); }
            if (nextChar == '<') {
                value += readChar();
                if (peekChar() == '=') { value += readChar(); return Token(TokenType::Op_ShiftLeft, startLoc); } // <<=
                return Token(TokenType::Op_ShiftLeft, startLoc); // <<
            }
            return Token(TokenType::Op_LessThan, startLoc);
        case '>': // >, >=, >>, >>=
            if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_GreaterEqual, startLoc); }
             if (nextChar == '>') {
                value += readChar();
                if (peekChar() == '=') { value += readChar(); return Token(TokenType::Op_ShiftRight, startLoc); } // >>=
                return Token(TokenType::Op_ShiftRight, startLoc); // >>
            }
            return Token(TokenType::Op_GreaterThan, startLoc);
        case '&': // &, &&, &=
            if (nextChar == '&') { value += readChar(); return Token(TokenType::Op_And, startLoc); }
            if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_BitAnd, startLoc); }
            return Token(TokenType::Op_BitAnd, startLoc);
        case '|': // |, ||, |=
            if (nextChar == '|') { value += readChar(); return Token(TokenType::Op_Or, startLoc); }
            if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_BitOr, startLoc); }
            return Token(TokenType::Op_BitOr, startLoc);
        case '^': // ^, ^=
             if (nextChar == '=') { value += readChar(); return Token(TokenType::Op_BitXor, startLoc); }
             return Token(TokenType::Op_BitXor, startLoc);
        case '.': // ., ... (variadic)
             if (nextChar == '.') { // İkinci noktayı kontrol et
                 value += readChar();
                 if (peekChar() == '.') { // Üçüncü noktayı kontrol et
                     value += readChar();
                     // TODO: Variadic (...) token türünü ekleyin
                     return Token(TokenType::Op_Dot, value, startLoc); // Şimdilik string değeriyle Op_Dot olarak döndür
                 }
                 // İki nokta .. (slice veya range?)
                 // TODO: Range (..) token türünü ekleyin
                 return Token(TokenType::Op_Dot, value, startLoc); // Şimdilik string değeriyle Op_Dot olarak döndür
             }
             return Token(TokenType::Op_Dot, startLoc); // Tek nokta
        case ':': // :, ::
            if (nextChar == ':') { value += readChar(); return Token(TokenType::Op_DoubleColon, startLoc); }
            return Token(TokenType::Punc_Colon, startLoc);

        // Tek karakterli noktalama işaretleri
        case ';': return Token(TokenType::Punc_Semicolon, startLoc);
        case ',': return Token(TokenType::Punc_Comma, startLoc);
        case '(': return Token(TokenType::Punc_OpenParen, startLoc);
        case ')': return Token(TokenType::Punc_CloseParen, startLoc);
        case '{': return Token(TokenType::Punc_OpenBrace, startLoc);
        case '}': return Token(TokenType::Punc_CloseBrace, startLoc);
        case '[': return Token(TokenType::Punc_OpenBracket, startLoc);
        case ']': return Token(TokenType::Punc_CloseBracket, startLoc);

        // TODO: Diğer operatörler ve noktalama işaretleri (+ gibi bileşikler hariç)
        // ? , ~ , # , @ , $ , ` , <>
         case '?': return Token(TokenType::Op_Question, startLoc);
         case '~': return Token(TokenType::Op_Tilde, startLoc);
         case '@': return Token(TokenType::Op_At, startLoc);
         case '#': return Token(TokenType::Op_Hash, startLoc);
         case '$': return Token(TokenType::Op_Dollar, startLoc);
         case '`': return Token(TokenType::Op_Backtick, startLoc);


        default:
            // Tanınmayan karakter
            error("Tanınmayan karakter '" + std::string(1, firstChar) + "'");
            return Token(TokenType::Error, startLoc);
    }
}

// Sonraki token'ı almak için ana fonksiyon
Token Lexer::getNextToken() {
    // Önce boşlukları ve yorumları atla
    skipWhitespaceAndComments();

    // Token'ın başlangıç konumunu kaydet
    SourceLocation startLoc = CurrentLoc;

    // Dosya sonu kontrolü
    if (isAtEOF()) {
        return Token(TokenType::EndOfFile, startLoc);
    }

    // Karakter türüne göre lexing yap
    char currentChar = CurrentChar;

    if (std::isalpha(currentChar) || currentChar == '_') {
        return lexIdentifier(); // Identifier veya anahtar kelime olabilir
    } else if (std::isdigit(currentChar)) {
        return lexNumber(); // Sayı olabilir
    } else if (currentChar == '"') {
        return lexString(); // String literal olabilir
    } else if (currentChar == '\'') {
        return lexCharLiteral(); // Karakter literal olabilir
    } else {
        // Operatör veya noktalama işareti veya bilinmeyen karakter
        return lexOperatorOrPunctuation();
    }
}

} // namespace lexer
} // namespace dppc
