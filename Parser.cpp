#include "Parser.h"
#include <stdexcept> // İstisnalar için
#include <sstream>   // String akımları için

namespace dppc {
namespace parser {

// İkili operatör önceliklerini başlat
std::map<lexer::TokenType, int> BinOpPrecedence;

void InitializeBinOpPrecedence() {
    // Öncelik değerleri örneklerdir, kendi dilinize göre ayarlayın.
    // Düşük değer -> Düşük öncelik (örn: atama)
    // Yüksek değer -> Yüksek öncelik (örn: çarpma/bölme)
    // Tekli operatörler genellikle en yüksek önceliğe sahiptir.

    BinOpPrecedence[lexer::TokenType::Op_Equal] = 10; // =
    BinOpPrecedence[lexer::TokenType::Op_Plus] = 20; // +
    BinOpPrecedence[lexer::TokenType::Op_Minus] = 20; // -
    BinOpPrecedence[lexer::TokenType::Op_Star] = 30; // *
    BinOpPrecedence[lexer::TokenType::Op_Slash] = 30; // /
    BinOpPrecedence[lexer::TokenType::Op_Percent] = 30; // %
    BinOpPrecedence[lexer::TokenType::Op_EqualEqual] = 40; // ==, !=
    BinOpPrecedence[lexer::TokenType::Op_NotEqual] = 40;
    BinOpPrecedence[lexer::TokenType::Op_LessThan] = 40; // <, >, <=, >=
    BinOpPrecedence[lexer::TokenType::Op_GreaterThan] = 40;
    BinOpPrecedence[lexer::TokenType::Op_LessEqual] = 40;
    BinOpPrecedence[lexer::TokenType::Op_GreaterEqual] = 40;
    BinOpPrecedence[lexer::TokenType::Op_And] = 50; // &&
    BinOpPrecedence[lexer::TokenType::Op_Or] = 60; // ||
    BinOpPrecedence[lexer::TokenType::Op_BitAnd] = 70; // &
    BinOpPrecedence[lexer::TokenType::Op_BitOr] = 80; // |
    BinOpPrecedence[lexer::TokenType::Op_BitXor] = 90; // ^
    BinOpPrecedence[lexer::TokenType::Op_ShiftLeft] = 100; // <<, >>
    BinOpPrecedence[lexer::TokenType::Op_ShiftRight] = 100;
    BinOpPrecedence[lexer::TokenType::Op_Dot] = 110; // . (Üye erişimi genellikle yüksek öncelikli)
    BinOpPrecedence[lexer::TokenType::Op_Arrow] = 110; // -> (Üye erişimi)
    BinOpPrecedence[lexer::TokenType::Op_DoubleColon] = 120; // :: (En yüksek olabilir veya bağlama göre değişir)

    // Bileşik atamalar (+=, -= vb.) genellikle atama (=) ile aynı veya biraz daha yüksek öncelikli olabilir
    // D syntax'ına göre bu öncelikleri belirleyin.
}


// Parser Constructor
Parser::Parser(lexer::Lexer& lexer, ErrorReporter& reporter)
    : Lexer(lexer), Reporter(reporter)
{
    // LLVM'in cl kütüphanesini kullanmıyorsak veya başka bir yerde initialize etmiyorsak,
    // ikili operatör öncelik haritasını burada initialize edebiliriz.
    // InitializeBinOpPrecedence(); // main içinde de çağırabilirsiniz
    CurrentToken = Lexer.getNextToken(); // İlk token'ı al
}

// Mevcut token'ı tüketir ve bir sonraki token'ı alır
lexer::Token Parser::consumeToken() {
    lexer::Token oldToken = std::move(CurrentToken); // Mevcut token'ı taşı
    CurrentToken = Lexer.getNextToken(); // Yeni token al
    // std::cerr << "Consumed: " << oldToken << ", Next: " << CurrentToken << std::endl; // Debug
    return oldToken;
}

// Mevcut token'a bakar
const lexer::Token& Parser::getCurrentToken() const {
    return CurrentToken;
}

// Mevcut token'ın türünü kontrol eder
bool Parser::peekToken(lexer::TokenType type) {
    return CurrentToken.type == type;
}

// Mevcut token beklenen türdeyse tüketir ve true döner, aksi halde false
bool Parser::consumeIf(lexer::TokenType type) {
    if (peekToken(type)) {
        consumeToken();
        return true;
    }
    return false;
}

// Mevcut token beklenen türde değilse hata verir ve tüketir
void Parser::expect(lexer::TokenType type, const std::string& errorMessage) {
    if (!consumeIf(type)) {
        parseError(errorMessage + ", ancak '" + lexer::tokenTypeToString(CurrentToken.type) + "' bulundu.");
        // Hata sonrası toparlanma burada düşünülebilir
        synchronize(); // Basit hata toparlanması
    }
}

// Hata raporlayıcıya hata iletmek için
void Parser::parseError(const std::string& message) {
    Reporter.reportError(CurrentToken.loc, message);
}

// Basit hata sonrası toparlanma
// Hatadan sonra ';' veya '}' gibi belirteçlere kadar token atlar
void Parser::synchronize() {
    consumeToken(); // Hatalı token'ı atla

    while (getCurrentToken().type != lexer::TokenType::EndOfFile) {
        switch (getCurrentToken().type) {
            case lexer::TokenType::Punc_Semicolon: // ;
            case lexer::TokenType::Punc_CloseBrace: // }
            case lexer::TokenType::Kw_fn: // function
            case lexer::TokenType::Kw_struct: // struct
            case lexer::TokenType::Kw_trait: // trait
            case lexer::TokenType::Kw_impl: // impl
            case lexer::TokenType::Kw_enum: // enum
            case lexer::TokenType::Kw_let: // let
            case lexer::TokenType::Kw_for: // for
            case lexer::TokenType::Kw_while: // while
            case lexer::TokenType::Kw_if: // if
            case lexer::TokenType::Kw_match: // match
            case lexer::TokenType::Kw_return: // return
                return; // Bu tokenlardan birini bulduğumuzda toparlandığımızı varsayabiliriz
            default:
                // Hata sonrası anlamsız tokenları atla
                consumeToken();
        }
    }
}


// --- Ayrıştırma Fonksiyonları İmplementasyonları ---

// Kaynak dosyanın tamamını ayrıştırmak için ana fonksiyon
std::unique_ptr<ast::SourceFileAST> Parser::parseSourceFile() {
    SourceLocation startLoc = getCurrentToken().loc;
    std::vector<std::unique_ptr<ast::DeclAST>> declarations;

    // Dosya sonuna kadar üst düzey bildirimleri ayrıştır
    while (getCurrentToken().type != lexer::TokenType::EndOfFile) {
        if (auto decl = parseDeclaration()) {
            declarations.push_back(std::move(decl));
        } else {
            // Bir bildirim ayrıştırılamazsa hata olmuştur, toparlanmaya çalış
             parseError("Üst düzey bildirim bekleniyor.");
             synchronize(); // Hata sonrası toparlanma
             // Tekrar deneme veya döngüden çıkma kararı verilebilir
             if (getCurrentToken().type == lexer::TokenType::EndOfFile) break; // Toparlanma başarısızsa sonsuz döngüyü önle
        }
    }

    return std::make_unique<ast::SourceFileAST>(startLoc.filename, std::move(declarations), startLoc);
}

// Üst düzey bildirimleri ayrıştırır (fonksiyon, struct, trait vb.)
std::unique_ptr<ast::DeclAST> Parser::parseDeclaration() {
     // Hangi bildirim türü olduğunu belirlemek için mevcut tokene bakılır.
    switch (getCurrentToken().type) {
        case lexer::TokenType::Kw_fn:
             return parseFunctionDecl();
        case lexer::TokenType::Kw_struct:
             return parseStructDecl();
        case lexer::TokenType::Kw_trait:
             return parseTraitDecl();
        case lexer::TokenType::Kw_impl:
             return parseImplDecl();
        case lexer::TokenType::Kw_enum:
            return parseEnumDecl();
        // TODO: D'deki diğer bildirim türleri (class, interface, union, alias, module vb.)

        default:
            // Bir bildirimle başlamayan ancak üst düzeyde bulunan bir şeyle karşılaşıldı
            // Muhtemelen bir deyim veya hata. Basitlik için şimdilik hata verelim.
            parseError("Beklenmeyen token: Üst düzey bildirim (fonksiyon, struct, trait, impl, enum vb.) bekleniyor.");
            return nullptr; // Ayrıştırma başarısız
    }
}


// Deyimleri ayrıştırır
std::unique_ptr<ast::StmtAST> Parser::parseStatement() {
    // Mevcut token türüne göre uygun deyim ayrıştırma fonksiyonunu çağırır.
    switch (getCurrentToken().type) {
        case lexer::TokenType::Punc_OpenBrace: // { ... } blok deyimi
             return parseBlockStmt();
        case lexer::TokenType::Kw_if: // if deyimi
             return parseIfStmt();
        case lexer::TokenType::Kw_while: // while deyimi
             return parseWhileStmt();
        case lexer::TokenType::Kw_for: // for deyimi
             return parseForStmt();
        case lexer::TokenType::Kw_return: // return deyimi
             return parseReturnStmt();
        case lexer::TokenType::Kw_break: // break deyimi
            return parseBreakStmt();
        case lexer::TokenType::Kw_continue: // continue deyimi
            return parseContinueStmt();
        case lexer::TokenType::Kw_let: // let deyimi (D++'a özgü)
            return parseLetStmt();
        // TODO: D'ye özgü diğer deyimler (switch, do, foreach vb.)

        default:
            // Yukarıdaki özel deyimlerden hiçbiri değilse, bir ifade deyimi olabilir.
            // Bir ifadeyi ayrıştır ve sonunda noktalı virgül bekle.
            if (auto expr = parseExpression()) {
                 SourceLocation loc = expr->loc; // İfade deyiminin konumu
                 expect(lexer::TokenType::Punc_Semicolon, "Deyim sonunda ';' bekleniyor");
                 return std::make_unique<ast::ExprStmtAST>(std::move(expr), loc);
            }
            // Hiçbir deyim ayrıştırılamazsa hata döndür.
            parseError("Beklenmeyen token: Deyim bekleniyor.");
            return nullptr; // Ayrıştırma başarısız
    }
}

// Bloğu ayrıştırır ({ ... })
std::unique_ptr<ast::BlockStmtAST> Parser::parseBlockStmt() {
    SourceLocation startLoc = getCurrentToken().loc;
    expect(lexer::TokenType::Punc_OpenBrace, "'{' bekleniyor");

    std::vector<std::unique_ptr<ast::StmtAST>> statements;
    // '}' token'ına veya dosya sonuna kadar deyimleri ayrıştır
    while (!peekToken(lexer::TokenType::Punc_CloseBrace) && !peekToken(lexer::TokenType::EndOfFile)) {
        if (auto stmt = parseStatement()) {
            statements.push_back(std::move(stmt));
        } else {
            // Deyim ayrıştırma başarısız oldu, toparlanmaya çalış
            synchronize();
            if (peekToken(lexer::TokenType::EndOfFile) || peekToken(lexer::TokenType::Punc_CloseBrace)) break;
        }
    }

    expect(lexer::TokenType::Punc_CloseBrace, "'}' bekleniyor");
    return std::make_unique<ast::BlockStmtAST>(std::move(statements), startLoc);
}

// İfadeyi ayrıştırır (öncelik tabanlı)
// Bu, 'pratt parser' veya 'precedence climbing' olarak bilinen tekniktir.
std::unique_ptr<ast::ExprAST> Parser::parseExpression() {
    // İlk olarak tekli operatörleri ve birincil ifadeyi ayrıştır
    auto left = parseUnaryOpExpr();
    if (!left) {
        // parseUnaryOpExpr başarısız olduysa, bu birincil ifadeyi ayrıştır
        left = parsePrimaryExpr();
        if (!left) {
            return nullptr; // İfade ayrıştırılamadı
        }
    }


    // Daha yüksek öncelikli ikili operatörleri ayrıştır
    // `1 + 2 * 3` gibi durumlarda önceliklerin doğru çalışmasını sağlar
    return parseBinaryOpExpr(0, std::move(left));
}

// Tekli operatör ifadelerini ayrıştırır (-x, !y, *p, &v)
std::unique_ptr<ast::ExprAST> Parser::parseUnaryOpExpr() {
    // Tekli operatör tokenlarını kontrol et
    if (peekToken(lexer::TokenType::Op_Minus) ||
        peekToken(lexer::TokenType::Op_Not) ||
        peekToken(lexer::TokenType::Op_Star) || // Dereference (*)
        peekToken(lexer::TokenType::Op_BitAnd)) // Borrow (&)
    {
        SourceLocation opLoc = getCurrentToken().loc;
        lexer::TokenType opType = getCurrentToken().type;
        consumeToken(); // Operatörü tüket

        // Operandı rekürsif olarak ayrıştır (genellikle daha yüksek öncelikli ifadeler)
        if (auto operand = parseUnaryOpExpr()) { // Tekli operatörler zincirlenebilir (!!x)
             if (opType == lexer::TokenType::Op_BitAnd) { // & veya &mut için özel kontrol
                  bool isMut = false;
                  if (peekToken(lexer::TokenType::Kw_mut)) {
                      consumeToken(); // 'mut' kelimesini tüket
                      isMut = true;
                  }
                 return std::make_unique<ast::BorrowExprAST>(std::move(operand), isMut, opLoc);
             }
            return std::make_unique<ast::UnaryOpExprAST>(opType, std::move(operand), opLoc);
        } else {
            // Operand ayrıştırılamadı
            parseError("Tekli operatörden sonra ifade bekleniyor.");
            return nullptr;
        }
    }

    // Tekli operatör yoksa null döndür, parseExpression birincil ifadeyi dener.
    return nullptr;
}

// İkili operatör ifadelerini önceliğe göre ayrıştırır
std::unique_ptr<ast::ExprAST> Parser::parseBinaryOpExpr(int minPrecedence, std::unique_ptr<ast::ExprAST> left) {
    while (true) {
        lexer::TokenType currentOpType = getCurrentToken().type;

        // Mevcut token bir ikili operatör değilse veya önceliği yeterli değilse döngüyü kır
        auto it = BinOpPrecedence.find(currentOpType);
        if (it == BinOpPrecedence.end() || it->second < minPrecedence) {
            break; // Mevcut token, sağ operandın parçası değil
        }

        // Operatör bulundu, tüket
        lexer::TokenType op = currentOpType;
        SourceLocation opLoc = getCurrentToken().loc;
        int currentPrecedence = it->second;
        consumeToken(); // Operatörü tüket

        // Sağ operandı ayrıştır
        auto right = parseUnaryOpExpr(); // Sağ operand bir tekli operatörle başlayabilir
         if (!right) {
             right = parsePrimaryExpr(); // Veya birincil ifade olabilir
             if (!right) {
                  parseError("İkili operatörden sonra operand bekleniyor.");
                  return nullptr; // Hata
             }
         }


        // Sağ operanddan sonra daha yüksek öncelikli operatörler var mı kontrol et
        // Bu, sağ asociativity (örn: atama) veya sol asociativity (örn: toplama) için öncelik kurallarına göre yapılır.
        // Çoğu operatör sol asociativity'dir.
        lexer::TokenType nextOpType = getCurrentToken().type;
        auto nextIt = BinOpPrecedence.find(nextOpType);

        while (nextIt != BinOpPrecedence.end() &&
               (nextIt->second > currentPrecedence ||
               (nextIt->second == currentPrecedence /* && operatör sağ associativity ise */))) // TODO: Associativity kurallarını ekle
         {
             // Sağ tarafta daha yüksek öncelikli (veya aynı öncelikte sağ associativity) bir operatör var.
             // Sağ operandın geri kalanını yeni minPrecedence ile tekrar ayrıştır.
             right = parseBinaryOpExpr(nextIt->second, std::move(right));
             if (!right) {
                 return nullptr; // Hata
             }
             nextOpType = getCurrentToken().type;
             nextIt = BinOpPrecedence.find(nextOpType);
         }

        // Sol operandı (şu ana kadar ayrıştırdığımız ifade) ve yeni ayrıştırdığımız sağ operandı
        // mevcut operatörle birleştirerek yeni bir ikili operatör düğümü oluştur.
        left = std::make_unique<ast::BinaryOpExprAST>(op, std::move(left), std::move(right), opLoc);
    }

    // Döngü bittiğinde, elimizdeki 'left' tam olarak ayrıştırılmış ifade ağacının köküdür.
    return left;
}


// En temel ifadeleri ayrıştırır (literal, identifier, parantezli ifade vb.)
std::unique_ptr<ast::ExprAST> Parser::parsePrimaryExpr() {
    SourceLocation startLoc = getCurrentToken().loc;
    switch (getCurrentToken().type) {
        case lexer::TokenType::Identifier:
             return parseIdentifierExpr();
        case lexer::TokenType::IntegerLiteral:
             return parseNumberExpr();
        case lexer::TokenType::FloatingLiteral:
             return parseNumberExpr(); // Hem int hem float aynı fonksiyonda işlenebilir
        case lexer::TokenType::StringLiteral:
             return parseStringExpr();
        case lexer::TokenType::BooleanLiteral:
             return parseBooleanExpr();
        case lexer::TokenType::Punc_OpenParen: // Parantezli ifade (expr)
        {
            consumeToken(); // '(' tüket
            auto expr = parseExpression(); // Parantez içindeki ifadeyi ayrıştır
            if (!expr) return nullptr; // Hata oluştu
            expect(lexer::TokenType::Punc_CloseParen, "')' bekleniyor");
            return expr; // Parantezler AST'de temsil edilmez, içindeki ifade döndürülür
        }
        case lexer::TokenType::Kw_new: // new ifadesi
            return parseNewExpr();
        case lexer::TokenType::Kw_match: // match ifadesi (D++'a özgü)
             return parseMatchExpr();

        // TODO: D'deki diğer birincil ifadeler (array literal [], struct literal, function literal vb.)
        // Array literal: [1, 2, 3]
        // Struct literal: MyStruct { field: value }
        // Tuple literal: (1, "hello")
        // Lambda/Anon fonksiyon: (args) => body

        default:
            // Tanınmayan veya beklenmeyen birincil ifade başlangıcı
            parseError("Beklenmeyen token: İfade bekleniyor.");
            return nullptr;
    }
}

// Identifier ifadelerini ayrıştırır ve üye erişimi/fonksiyon çağrısı için devamını kontrol eder
std::unique_ptr<ast::ExprAST> Parser::parseIdentifierExpr() {
    SourceLocation startLoc = getCurrentToken().loc;
    std::string name = getCurrentToken().value;
    expect(lexer::TokenType::Identifier, "Identifier bekleniyor");

    auto idExpr = std::make_unique<ast::IdentifierExprAST>(std::move(name), startLoc);

    // Identifier'dan sonra gelen . veya -> ile üye erişimi veya parantez () ile fonksiyon çağrısı olabilir.
    // Bunlar sol ilişkili (left-associative) olduğu için döngüde işlenir.
    return parseMemberAccessOrCall(std::move(idExpr));
}


// . veya -> ile üye erişimi veya () ile fonksiyon çağrısını ayrıştırır
std::unique_ptr<ast::ExprAST> Parser::parseMemberAccessOrCall(std::unique_ptr<ast::ExprAST> base) {
     auto left = std::move(base); // Şu ana kadar ayrıştırdığımız sol taraf (identifier, call, member access vb.)

     while (true) {
         SourceLocation currentLoc = getCurrentToken().loc;
         if (consumeIf(lexer::TokenType::Punc_OpenParen)) { // Fonksiyon çağrısı ()
             // Sol taraf bir fonksiyon çağrısı (CallExprAST) veya IdentifierExprAST olmalı
             // Bu kısım fonksiyon çağırma argümanlarını ayrıştırır.
             std::vector<std::unique_ptr<ast::ExprAST>> args;
             // Kapatan parantez ')' gelene kadar argümanları oku
             while (!peekToken(lexer::TokenType::Punc_CloseParen) && !peekToken(lexer::TokenType::EndOfFile)) {
                 if (auto arg = parseExpression()) {
                     args.push_back(std::move(arg));
                     // Argümanlar arasında virgül beklenir
                     if (!peekToken(lexer::TokenType::Punc_CloseParen)) {
                         expect(lexer::TokenType::Punc_Comma, "Fonksiyon çağrısı argümanları arasında ',' bekleniyor");
                     }
                 } else {
                     // Argüman ayrıştırılamadı
                     parseError("Fonksiyon çağrısı argümanı bekleniyor.");
                     synchronize(); // Hata sonrası toparlanma
                     if (peekToken(lexer::TokenType::Punc_CloseParen) || peekToken(lexer::TokenType::EndOfFile)) break;
                 }
             }
             expect(lexer::TokenType::Punc_CloseParen, "')' bekleniyor");

             // Şu anki sol ifade (left) fonksiyonun adı/çağrılabilir ifadesi olmalı.
             // Basitlik için şimdilik sadece IdentifierExprAST olduğunu varsayalım.
             // Daha gelişmiş bir parser, left'in herhangi bir CallExprAST veya üye erişimi olabileceğini işleyecektir.
             // Şimdilik CalleeName'i string olarak alalım.
             std::string calleeName = "";
             if (auto idExpr = dynamic_cast<ast::IdentifierExprAST*>(left.get())) {
                 calleeName = idExpr->Name;
             } else {
                 // Eğer sol taraf bir identifier değilse, bu daha karmaşık bir durumdur (örn: (obj.method)(args)).
                 // Bu örnek kod için bunu basitleştiriyoruz.
                 parseError("Fonksiyon çağrısından önce identifier bekleniyor. Daha karmaşık ifadeler şu anda desteklenmiyor.");
                 // left yine de CallExprAST içinde tutulabilir, sema aşaması tipini kontrol eder.
                 // Ancak AST düğümü şimdilik basitleştirilmiş CallExprAST'ye uymayabilir.
                 // Geçici olarak bir IdentifierExprAST oluşturup ismine <unknown> diyelim.
                 calleeName = "<unknown>";
             }
             // IdentifierExprAST'nin sahipliği CallExprAST'ye taşınacağı için burada nullptr yapalım veya dikkatli taşıyalım.
             // En iyisi CalleeName'i IdentifierExprAST'den alıp yeni CallExprAST oluşturmak.
             // Ancak calleeExpr'ın kendisi de bir AST düğümü olabilir. Gerçekçi bir AST şöyle olur:
             // CallExprAST { Callee: unique_ptr<ExprAST>, Args: ... }
             // Nerede Callee: IdentifierExprAST, MemberAccessExprAST, diğer CallExprAST olabilir.
             // Bu basitleştirilmiş örnekte CalleeName string'i kullanılıyor. Daha doğru AST yapısı:
             // left = std::make_unique<ast::CallExprAST>(std::move(left), std::move(args), currentLoc); // Sol ifadeyi doğrudan kullan
             // Şimdilik string ismini kullanan versiyonu döndürelim:
             left = std::make_unique<ast::CallExprAST>(calleeName, std::move(args), currentLoc);


         } else if (peekToken(lexer::TokenType::Op_Dot) || peekToken(lexer::TokenType::Op_Arrow)) { // Üye Erişimi . veya ->
             bool isPointerAccess = consumeIf(lexer::TokenType::Op_Arrow); // -> ise true
             if (!isPointerAccess) {
                 expect(lexer::TokenType::Op_Dot, "'.' veya '->' bekleniyor"); // . ise kontrol et
             }

             // Üye adını (identifier) ayrıştır
             SourceLocation memberLoc = getCurrentToken().loc;
             std::string memberName = getCurrentToken().value;
             expect(lexer::TokenType::Identifier, "Üye adı bekleniyor");

             // Yeni bir üye erişimi düğümü oluştur
             left = std::make_unique<ast::MemberAccessExprAST>(std::move(left), std::move(memberName), isPointerAccess, currentLoc);

         } else if (peekToken(lexer::TokenType::Punc_OpenBracket)) { // Dizi Erişimi []
             // Sol taraf bir dizi/slice ifadesi olmalı
             expect(lexer::TokenType::Punc_OpenBracket, "'[' bekleniyor");

             auto indexExpr = parseExpression(); // İndeks ifadesini ayrıştır
             if (!indexExpr) {
                 parseError("Dizi indeksi bekleniyor.");
                 // Hata sonrası toparlanma düşünülebilir
             }

             expect(lexer::TokenType::Punc_CloseBracket, "']' bekleniyor");

             // Yeni bir dizi erişimi düğümü oluştur
             left = std::make_unique<ast::ArrayAccessExprAST>(std::move(left), std::move(indexExpr), currentLoc);

         } else {
             // Ne . ne -> ne de ( ne [ geldi. Zincir sona erdi.
             break;
         }
     }

     return left; // Tamamlanmış ifadeyi döndür
}

// Sayısal literal ifadeleri ayrıştırır (Integer veya Floating)
std::unique_ptr<ast::ExprAST> Parser::parseNumberExpr() {
    SourceLocation loc = getCurrentToken().loc;
    std::string valStr = getCurrentToken().value;

    if (getCurrentToken().type == lexer::TokenType::IntegerLiteral) {
        // String'i long long'a çevir
        try {
            long long val = std::stoll(valStr);
            expect(lexer::TokenType::IntegerLiteral, "Integer literal bekleniyor");
            return std::make_unique<ast::IntegerExprAST>(val, loc);
        } catch (const std::out_of_range& oor) {
             parseError("Integer literal taşması: " + valStr);
             expect(lexer::TokenType::IntegerLiteral, "Integer literal bekleniyor"); // Token'ı tüket
             return std::make_unique<ast::IntegerExprAST>(0, loc); // Varsayılan değerle devam et
        } catch (...) {
             parseError("Geçersiz integer literal: " + valStr);
             expect(lexer::TokenType::IntegerLiteral, "Integer literal bekleniyor"); // Token'ı tüket
             return std::make_unique<ast::IntegerExprAST>(0, loc); // Varsayılan değerle devam et
        }

    } else if (getCurrentToken().type == lexer::TokenType::FloatingLiteral) {
        // String'i double'a çevir
         try {
             double val = std::stod(valStr);
             expect(lexer::TokenType::FloatingLiteral, "Floating literal bekleniyor");
             return std::make_unique<ast::FloatingExprAST>(val, loc);
         } catch (const std::out_of_range& oor) {
             parseError("Floating literal taşması: " + valStr);
              expect(lexer::TokenType::FloatingLiteral, "Floating literal bekleniyor"); // Token'ı tüket
              return std::make_unique<ast::FloatingExprAST>(0.0, loc); // Varsayılan değerle devam et
         } catch (...) {
              parseError("Geçersiz floating literal: " + valStr);
              expect(lexer::TokenType::FloatingLiteral, "Floating literal bekleniyor"); // Token'ı tüket
              return std::make_unique<ast::FloatingExprAST>(0.0, loc); // Varsayılan değerle devam et
         }
    }
    // Bu fonksiyon sadece sayısal literal tokenlarla çağrılmalı
    return nullptr; // Hata durumu, buraya ulaşılmamalı
}

// String literal ifadeleri ayrıştırır
std::unique_ptr<ast::ExprAST> Parser::parseStringExpr() {
     SourceLocation loc = getCurrentToken().loc;
     std::string val = getCurrentToken().value;
     expect(lexer::TokenType::StringLiteral, "String literal bekleniyor");
     return std::make_unique<ast::StringExprAST>(std::move(val), loc);
}

// Boolean literal ifadeleri ayrıştırır (true/false)
std::unique_ptr<ast::ExprAST> Parser::parseBooleanExpr() {
    SourceLocation loc = getCurrentToken().loc;
    bool val = false;
    if (peekToken(lexer::TokenType::Kw_true)) {
        val = true;
        consumeToken();
    } else if (peekToken(lexer::TokenType::Kw_false)) {
        val = false;
        consumeToken();
    } else {
        parseError("Boolean literal (true veya false) bekleniyor.");
        return nullptr;
    }
    return std::make_unique<ast::BooleanExprAST>(val, loc);
}


// new ifadesini ayrıştırır
std::unique_ptr<ast::ExprAST> Parser::parseNewExpr() {
    SourceLocation startLoc = getCurrentToken().loc;
    expect(lexer::TokenType::Kw_new, "'new' anahtar kelimesi bekleniyor");

    // new'den sonra oluşturulacak tipin adı beklenir (şimdilik identifier varsayalım)
    SourceLocation typeLoc = getCurrentToken().loc;
    std::string typeName = getCurrentToken().value;
    expect(lexer::TokenType::Identifier, "'new' anahtar kelimesinden sonra tip adı bekleniyor");

    // Opsiyonel olarak constructor argümanları için parantezli liste gelebilir
    std::vector<std::unique_ptr<ast::ExprAST>> args;
    if (peekToken(lexer::TokenType::Punc_OpenParen)) {
         expect(lexer::TokenType::Punc_OpenParen, "'(' bekleniyor");
         // Kapatan parantez ')' gelene kadar argümanları oku
         while (!peekToken(lexer::TokenType::Punc_CloseParen) && !peekToken(lexer::TokenType::EndOfFile)) {
             if (auto arg = parseExpression()) {
                 args.push_back(std::move(arg));
                 // Argümanlar arasında virgül beklenir
                 if (!peekToken(lexer::TokenType::Punc_CloseParen)) {
                     expect(lexer::TokenType::Punc_Comma, "'new' argümanları arasında ',' bekleniyor");
                 }
             } else {
                 parseError("'new' ifadesinde argüman bekleniyor.");
                 synchronize();
                 if (peekToken(lexer::TokenType::Punc_CloseParen) || peekToken(lexer::TokenType::EndOfFile)) break;
             }
         }
         expect(lexer::TokenType::Punc_CloseParen, "')' bekleniyor");
    }

     return std::make_unique<ast::NewExprAST>(std::move(typeName), std::move(args), startLoc);
}


// --- D++'a Özgü Ayrıştırma Fonksiyonları ---

// Let deyimini ayrıştırır (let mut? pattern [: Type] = initializer;)
std::unique_ptr<ast::StmtAST> Parser::parseLetStmt() {
    SourceLocation startLoc = getCurrentToken().loc;
    expect(lexer::TokenType::Kw_let, "'let' anahtar kelimesi bekleniyor");

    // Pattern'ı ayrıştır (genellikle IdentifierPattern olacaktır ama daha fazlası da olabilir)
    auto pattern = parsePattern();
    if (!pattern) {
        parseError("'let' anahtar kelimesinden sonra desen (pattern) bekleniyor.");
        // Hata sonrası toparlanma düşünülebilir
        return nullptr;
    }

    // Opsiyonel tip belirtimi (: Type)
    std::unique_ptr<ast::TypeAST> type = nullptr;
    if (consumeIf(lexer::TokenType::Punc_Colon)) {
        type = parseType();
        if (!type) {
            parseError("':' işaretinden sonra tip bekleniyor.");
             // Hata sonrası toparlanma düşünülebilir
        }
    }

    // Opsiyonel başlangıç değeri ataması (= initializer)
    std::unique_ptr<ast::ExprAST> initializer = nullptr;
    if (consumeIf(lexer::TokenType::Op_Equal)) {
        initializer = parseExpression();
        if (!initializer) {
            parseError("'=' işaretinden sonra başlangıç değeri ifadesi bekleniyor.");
             // Hata sonrası toparlanma düşünülebilir
        }
    }

    // Let deyimi noktalı virgülle bitmelidir
    expect(lexer::TokenType::Punc_Semicolon, "'let' deyim sonunda ';' bekleniyor");

    return std::make_unique<ast::LetStmtAST>(std::move(pattern), std::move(type), std::move(initializer), startLoc);
}

// Match ifadesini ayrıştırır (match value { ... arms ... })
std::unique_ptr<ast::ExprAST> Parser::parseMatchExpr() {
    SourceLocation startLoc = getCurrentToken().loc;
    expect(lexer::TokenType::Kw_match, "'match' anahtar kelimesi bekleniyor");

    // Match yapılan ifadeyi ayrıştır
    auto valueExpr = parseExpression();
    if (!valueExpr) {
        parseError("'match' anahtar kelimesinden sonra ifade bekleniyor.");
        // Hata sonrası toparlanma düşünülebilir
        return nullptr;
    }

    // Match kolları için bloğu ayrıştır
    expect(lexer::TokenType::Punc_OpenBrace, "'match' ifadesinden sonra '{' bekleniyor");

    std::vector<ast::MatchExprAST::MatchArm> arms;
    // Kapanan '}' gelene kadar match kollarını ayrıştır
    while (!peekToken(lexer::TokenType::Punc_CloseBrace) && !peekToken(lexer::TokenType::EndOfFile)) {
        // Her kolu ayrıştır (pattern => expr)
        auto arm = parseMatchArm();
        // Ayrıştırma başarılıysa listeye ekle
        // parseMatchArm kendi içinde hata yönetimi yapmalı ve gerekirse nullptr döndürmeli.
        // Şimdilik basit bir kontrol yapalım:
         if (arm.Pattern && arm.Body) { // Pattern ve Body'nin varlığını kontrol et (Guard opsiyonel)
             arms.push_back(std::move(arm));
             // Kollar arasında virgül olması opsiyonel veya zorunlu olabilir, dilin gramerine göre belirleyin.
             // Rust genellikle son kol hariç virgül ister veya her koldan sonra virgül opsiyoneldir.
             // Basitlik için şimdilik sadece '> ' tokenını ayırıcı olarak kullanan bir dil syntax'ı varsayalım.
             // Eğer kollar noktalı virgülle ayrılıyorsa expect(lexer::TokenType::Punc_Semicolon, "Match kolu sonunda ';' bekleniyor");
             // Eğer virgülle ayrılıyorsa:
             if (!peekToken(lexer::TokenType::Punc_CloseBrace)) {
                  // Opsiyonel virgül işleme veya zorunlu virgül hatası
                  consumeIf(lexer::TokenType::Punc_Comma); // Opsiyonel virgül
             }
         } else {
             // parseMatchArm içinde hata raporlanmıştır, toparlanmaya çalış.
              synchronize();
              if (peekToken(lexer::TokenType::Punc_CloseBrace) || peekToken(lexer::TokenType::EndOfFile)) break;
         }
    }

    expect(lexer::TokenType::Punc_CloseBrace, "'match' ifadesinin sonunda '}' bekleniyor");

    return std::make_unique<ast::MatchExprAST>(std::move(valueExpr), std::move(arms), startLoc);
}

// Tek bir match kolunu ayrıştırır (pattern => expr)
ast::MatchExprAST::MatchArm Parser::parseMatchArm() {
    SourceLocation startLoc = getCurrentToken().loc; // Kolun başlangıç lokasyonu

    // Paterni ayrıştır
    auto pattern = parsePattern();
    if (!pattern) {
        parseError("Match kolunda desen (pattern) bekleniyor.");
        // Hata sonrası toparlanma düşünülebilir.
        // Hata durumunda eksik bir arm döndürebiliriz, Sema aşaması bunu yakalar.
         return ast::MatchExprAST::MatchArm(nullptr, nullptr, nullptr);
    }

    // Opsiyonel guard ifadesi (if condition)
    std::unique_ptr<ast::ExprAST> guard = nullptr;
    if (consumeIf(lexer::TokenType::Kw_if)) { // 'if' anahtar kelimesi
         guard = parseExpression(); // Koşul ifadesini ayrıştır
         if (!guard) {
             parseError("'if' anahtar kelimesinden sonra koşul ifadesi bekleniyor (match guard).");
             // Hata sonrası toparlanma
         }
    }


    // '=>' ok işaretini bekle
    expect(lexer::TokenType::Op_Arrow, "Match kolunda '=>' bekleniyor"); // -> ok işaretini kullanıyoruz

    // Kolun gövdesini (ifadesini) ayrıştır
    auto body = parseExpression(); // Gövde bir ifade olabilir
    if (!body) {
        parseError("Match kolunda '=>' işaretinden sonra ifade (gövde) bekleniyor.");
        // Hata sonrası toparlanma düşünülebilir
    }

    // MatchArm struct'ını oluştur ve döndür
    return ast::MatchExprAST::MatchArm(std::move(pattern), std::move(guard), std::move(body));
}

// Genel deseni ayrıştırır (Pattern Matching için)
std::unique_ptr<ast::PatternAST> Parser::parsePattern() {
    SourceLocation startLoc = getCurrentToken().loc;

    // Mevcut token türüne göre farklı pattern türlerini ayrıştır.
    // Patternler basitten karmaşığa doğru denenir veya belirleyici token'a bakılır.
    switch (getCurrentToken().type) {
        case lexer::TokenType::Op_BitAnd: // & veya &mut
            return parseReferencePattern();
        case lexer::TokenType::Identifier: { // Identifier Pattern veya Enum Variant veya Struct Deconstruction olabilir
             // Identifier'ı ayrıştır
            std::string name = getCurrentToken().value;
            consumeToken(); // Identifier token'ı tüket

            // Identifier'dan sonra '::' geliyorsa, bu bir Enum Variant Pattern olabilir
            if (consumeIf(lexer::TokenType::Op_DoubleColon)) {
                 // Enum adı: 'name', sonraki identifier varyant adı olmalı
                 SourceLocation variantLoc = getCurrentToken().loc;
                 std::string variantName = getCurrentToken().value;
                 expect(lexer::TokenType::Identifier, "Enum varyant adı bekleniyor.");

                 // Varyanttan sonra opsiyonel olarak argümanlar için parantezli liste gelebilir
                 std::vector<std::unique_ptr<ast::PatternAST>> args;
                 if (consumeIf(lexer::TokenType::Punc_OpenParen)) {
                      // Kapatan parantez ')' gelene kadar argüman patternlerini ayrıştır
                      while (!peekToken(lexer::TokenType::Punc_CloseParen) && !peekToken(lexer::TokenType::EndOfFile)) {
                          if (auto argPattern = parsePattern()) {
                              args.push_back(std::move(argPattern));
                              if (!peekToken(lexer::TokenType::Punc_CloseParen)) {
                                  expect(lexer::TokenType::Punc_Comma, "Enum varyant argümanları arasında ',' bekleniyor.");
                              }
                          } else {
                                parseError("Enum varyant argümanı deseni bekleniyor.");
                                synchronize();
                                if (peekToken(lexer::TokenType::Punc_CloseParen) || peekToken(lexer::TokenType::EndOfFile)) break;
                          }
                      }
                      expect(lexer::TokenType::Punc_CloseParen, "')' bekleniyor.");
                 }
                 return std::make_unique<ast::EnumVariantPatternAST>(std::move(name), std::move(variantName), std::move(args), startLoc);

            } else if (consumeIf(lexer::TokenType::Punc_OpenBrace)) { // Identifier'dan sonra '{' geliyorsa, Struct Deconstruction Pattern olabilir
                 // Yapı adı: 'name'
                 std::vector<ast::StructDeconstructionPatternAST::FieldMatch> fields;
                 bool ignoreExtra = false;

                 // Kapanan '}' gelene kadar alan eşleşmelerini ayrıştır
                 while (!peekToken(lexer::TokenType::Punc_CloseBrace) && !peekToken(lexer::TokenType::EndOfFile)) {
                     SourceLocation fieldLoc = getCurrentToken().loc;
                     if (consumeIf(lexer::TokenType::Op_Dot) && consumeIf(lexer::TokenType::Op_Dot)) { // '..' ignore extra fields
                         ignoreExtra = true;
                         // '..' geldiyse başka alan olmamalı (genellikle)
                         if (!peekToken(lexer::TokenType::Punc_CloseBrace)) {
                              parseError("'..' sonrasında başka alan beklenmiyor.");
                         }
                         break; // Döngüden çık
                     }

                     // Alan adını ayrıştır
                     std::string fieldName = getCurrentToken().value;
                     expect(lexer::TokenType::Identifier, "Yapı alanı adı bekleniyor.");

                     std::unique_ptr<ast::PatternAST> fieldBinding;
                     if (consumeIf(lexer::TokenType::Punc_Colon)) { // Alan yeniden adlandırılmış veya pattern bağlanmış (field: pattern)
                         fieldBinding = parsePattern(); // Alan için deseni ayrıştır
                         if (!fieldBinding) {
                             parseError("Yapı alanı ':' sonrasında desen bekleniyor.");
                              synchronize();
                              if (peekToken(lexer::TokenType::Punc_CloseBrace) || peekToken(lexer::TokenType::EndOfFile)) break;
                         }
                     } else { // Alan adı kendi başına bir identifier pattern'dır (field)
                         // Field adı aynı zamanda bağlanacak pattern'dır
                         fieldBinding = std::make_unique<ast::IdentifierPatternAST>(fieldName, false, fieldLoc); // Varsayılan olarak mutable değil
                         // TODO: Identifier pattern mutable olabilir mi? Rust'ta struct deconstruction'da alanlar varsayılan immutable bind edilir.
                     }
                     fields.emplace_back(std::move(fieldName), std::move(fieldBinding));

                     if (!peekToken(lexer::TokenType::Punc_CloseBrace)) {
                          expect(lexer::TokenType::Punc_Comma, "Yapı deconstruction alanları arasında ',' bekleniyor.");
                     }
                 }
                 expect(lexer::TokenType::Punc_CloseBrace, "Yapı deconstruction deseninin sonunda '}' bekleniyor.");
                 return std::make_unique<ast::StructDeconstructionPatternAST>(std::move(name), std::move(fields), ignoreExtra, startLoc);

            } else { // Sadece identifier, bu bir Identifier Pattern
                 // Identifier token'ı zaten tüketildi.
                 bool isMut = false; // Identifier Pattern'ın mutable olup olmadığını kontrol et (let mut x = 5;)
                 // Bu kısım biraz karmaşık olabilir. `let mut x` formunda `mut` pattern'ın bir parçasıdır.
                 // `let x` ise `mut` yoktur. Pattern ayrıştırıcısının bunu Lexer'dan gelen tokenlara göre anlaması gerekir.
                 // Eğer IdentifierPatternAST'de IsMutable alanı varsa, lexer/parser'ın bunu tanıması gerekir.
                 // Basitlik için şimdilik sadece non-mutable identifier pattern varsayalım.
                 // Gerçek implementasyonda, `parseLetStmt` veya `parsePattern` başlangıcında `mut` kontrol edilip pattern'a bilgi verilebilir.
                 return std::make_unique<ast::IdentifierPatternAST>(std::move(name), false, startLoc);
            }
        }

        case lexer::TokenType::IntegerLiteral: // Literal Pattern
        case lexer::TokenType::FloatingLiteral:
        case lexer::TokenType::StringLiteral:
        case lexer::TokenType::BooleanLiteral:
             return parseLiteralPattern();

        case lexer::TokenType::Punc_OpenParen: // Tuple Pattern
             return parseTuplePattern();

        case lexer::TokenType::Op_BitAnd: // Reference Pattern (&, &mut) - Yukarıda işlendi ama buraya tekrar eklenebilir
             return parseReferencePattern();

        case lexer::TokenType::Op_Dot: // Wildcard Pattern (_) veya Range Pattern (..) ?
             if (consumeIf(lexer::TokenType::Op_Dot)) { // İkinci nokta
                  if (peekToken(lexer::TokenType::Op_Dot)) { // Üçüncü nokta ...
                      // TODO: Variadic pattern?
                  } else { // İki nokta ..
                      // Range Pattern veya ignore extra fields pattern (struct deconstruction içinde)
                       // ignore extra fields pattern'ı StructDeconstructionPattern içinde işliyoruz.
                       // Bu sadece bir range pattern olabilir.
                       // TODO: parseRangePattern(); // Range pattern ayrıştırma
                  }
             }
             // Tek nokta beklenmeyen bir pattern başlangıcı
             parseError("Beklenmeyen token: Desen bekleniyor.");
             consumeToken(); // Hatalı tokenı atla
             return nullptr;

        case lexer::TokenType::Op_Star: // Dereference pattern (*pat) - D++'a özgü?
             // Rust'ta *p pattern'ı değeri dereference edip o değere bağlanır.
             // D++'ta bu davranışı istiyor muyuz?
             // Şimdilik desteklemiyoruz.
             parseError("Beklenmeyen token: Desen bekleniyor. Dereference pattern desteklenmiyor.");
             consumeToken();
             return nullptr;


        case lexer::TokenType::Punc_OpenBracket: // Slice pattern? Array pattern?
             // D'de array literal, Rust'ta slice pattern [pat1, pat2], [pat..]
             // TODO: D++'ın array/slice pattern syntax'ını belirle ve ayrıştır.
             parseError("Beklenmeyen token: Desen bekleniyor. Array/Slice pattern desteklenmiyor.");
             // Hata toparlanma
             while(!peekToken(lexer::TokenType::Punc_CloseBracket) && !peekToken(lexer::TokenType::EndOfFile)) consumeToken();
             consumeIf(lexer::TokenType::Punc_CloseBracket);
             return nullptr;


        default:
             // Tanınmayan pattern başlangıcı
            parseError("Beklenmeyen token: Desen (pattern) bekleniyor.");
             // Hata toparlanma
             consumeToken(); // Hatalı tokenı atla
             return nullptr;
    }
}

// Wildcard pattern (_)
std::unique_ptr<ast::PatternAST> Parser::parseWildcardPattern() {
     SourceLocation loc = getCurrentToken().loc;
     expect(lexer::TokenType::Op_Dot, "'_' bekleniyor."); // Lexer'da '_' token türü Op_Dot olarak tanımlandıysa...
     // TODO: Lexer'da '_' için ayrı bir TokenType tanımlamak daha iyidir.
      expect(lexer::TokenType::Kw_Wildcard, "'_' bekleniyor."); // Yeni token türü
      return std::make_unique<ast::WildcardPatternAST>(loc);
}


// Literal pattern (1, "hello", true)
std::unique_ptr<ast::PatternAST> Parser::parseLiteralPattern() {
    SourceLocation loc = getCurrentToken().loc;
    // Literal ifadeleri ayrıştır
    auto literalExpr = parseNumberExpr(); // Sayı
    if (!literalExpr) literalExpr = parseStringExpr(); // String
    if (!literalExpr) literalExpr = parseBooleanExpr(); // Boolean
    // TODO: Karakter literal, null literal vb.

    if (literalExpr) {
        return std::make_unique<ast::LiteralPatternAST>(std::move(literalExpr), loc);
    } else {
        // parse...Expr fonksiyonları hata raporlamıştır.
         parseError("Literal pattern için sayı, string veya boolean literal bekleniyor.");
        return nullptr; // Ayrıştırma başarısız
    }
}

// Tuple Pattern ((a, b, _))
std::unique_ptr<ast::PatternAST> Parser::parseTuplePattern() {
     SourceLocation startLoc = getCurrentToken().loc;
     expect(lexer::TokenType::Punc_OpenParen, "'(' bekleniyor");

     std::vector<std::unique_ptr<ast::PatternAST>> elements;
     // Kapanan ')' gelene kadar element patternleri ayrıştır
     while (!peekToken(lexer::TokenType::Punc_CloseParen) && !peekToken(lexer::TokenType::EndOfFile)) {
         if (auto elementPattern = parsePattern()) {
             elements.push_back(std::move(elementPattern));
              if (!peekToken(lexer::TokenType::Punc_CloseParen)) {
                   expect(lexer::TokenType::Punc_Comma, "Tuple pattern elementleri arasında ',' bekleniyor.");
              }
         } else {
              parseError("Tuple pattern elementi bekleniyor.");
              synchronize();
              if (peekToken(lexer::TokenType::Punc_CloseParen) || peekToken(lexer::TokenType::EndOfFile)) break;
         }
     }
     expect(lexer::TokenType::Punc_CloseParen, "')' bekleniyor.");

     return std::make_unique<ast::TuplePatternAST>(std::move(elements), startLoc);
}

// Referans Pattern (&pat, &mut pat)
std::unique_ptr<ast::PatternAST> Parser::parseReferencePattern() {
    SourceLocation startLoc = getCurrentToken().loc;
    expect(lexer::TokenType::Op_BitAnd, "'&' bekleniyor");

    bool isMut = false;
    if (consumeIf(lexer::TokenType::Kw_mut)) {
        isMut = true;
    }

    // Referansın bağlandığı alt patterni ayrıştır
    auto subPattern = parsePattern();
    if (!subPattern) {
        parseError("Referans (& veya &mut) sonrasında desen (pattern) bekleniyor.");
        // Hata sonrası toparlanma düşünülebilir
        return nullptr;
    }

    return std::make_unique<ast::ReferencePatternAST>(std::move(subPattern), isMut, startLoc);
}


// --- Tip Ayrıştırma Fonksiyonları ---

// Genel tipi ayrıştırır
std::unique_ptr<ast::TypeAST> Parser::parseType() {
    SourceLocation startLoc = getCurrentToken().loc;

    // Hangi tip türü olduğunu belirlemek için ilk tokene bakılır.
    switch (getCurrentToken().type) {
        // İlkel tipler
        case lexer::TokenType::Kw_int:
        case lexer::TokenType::Kw_uint:
        case lexer::TokenType::Kw_short:
        case lexer::TokenType::Kw_ushort:
        case lexer::TokenType::Kw_long:
        case lexer::TokenType::Kw_ulong:
        case lexer::TokenType::Kw_float: // Float, double vb.
        case lexer::TokenType::Kw_bool:
        case lexer::TokenType::Kw_void:
            return parsePrimitiveType();

        case lexer::TokenType::Identifier: { // Kullanıcı tanımlı tip veya jenerik tip başlangıcı
             SourceLocation typeLoc = getCurrentToken().loc;
             std::string typeName = getCurrentToken().value;
             consumeToken(); // Identifier'ı tüket

            // Identifier'dan sonra '<' geliyorsa jenerik tip olabilir (örn: Option<T>, Result<T, E>, MyStruct<int>)
            // TODO: Jenerik tip argümanlarını parseType fonksiyonunu recursive çağırarak ayrıştır.
            // Şimdilik basit IdentifierType döndürelim.
             if (peekToken(lexer::TokenType::Op_LessThan)) {
                 // Eğer Option<T> veya Result<T,E> gibi özel D++ tipleri varsa,
                 // isimlerine göre özel fonksiyonlara yönlendirme yapılabilir.
                 if (typeName == "Option") return parseOptionType();
                 if (typeName == "Result") return parseResultType();

                 // Diğer jenerik tipler için genel jenerik tip ayrıştırma fonksiyonu çağrılabilir.
                 // return parseGenericType(std::move(typeName)); // TODO: GenericTypeAST ve ayrıştırma
             }

            return std::make_unique<ast::IdentifierTypeAST>(std::move(typeName), typeLoc);
        }

        case lexer::TokenType::Op_Star: // Pointer tipi (*T)
            return parsePointerType();

        case lexer::TokenType::Op_BitAnd: // Referans tipi (&T, &mut T)
            return parseReferenceType();

        case lexer::TokenType::Punc_OpenBracket: // Dizi tipi ([T] veya [T; N])
            return parseArrayType();

        case lexer::TokenType::Punc_OpenParen: // Tuple tipi ((T1, T2))
            return parseTupleType();

        // TODO: Fonksiyon tipi, delegate tipi, trait object tipi vb.

        default:
            parseError("Beklenmeyen token: Tip bekleniyor.");
            // Hata sonrası toparlanma
            consumeToken(); // Hatalı tokenı atla
            return nullptr;
    }
}

// İlkel tipi ayrıştırır (int, float, bool, void vb.)
std::unique_ptr<ast::TypeAST> Parser::parsePrimitiveType() {
    SourceLocation loc = getCurrentToken().loc;
    lexer::TokenType typeToken = getCurrentToken().type;
    // İlkel tip tokenlarından biri olduğundan emin ol (parseType içinde kontrol edildi)
    consumeToken(); // İlkel tip tokenını tüket
    return std::make_unique<ast::PrimitiveTypeAST>(typeToken, loc);
}


// Pointer tipi ayrıştırır (*T)
std::unique_ptr<ast::TypeAST> Parser::parsePointerType() {
    SourceLocation startLoc = getCurrentToken().loc;
    expect(lexer::TokenType::Op_Star, "'*' bekleniyor (pointer tipi için)");

    auto baseType = parseType(); // Taban tipi ayrıştır
    if (!baseType) {
        parseError("Pointer '*' işaretinden sonra tip bekleniyor.");
        // Hata sonrası toparlanma
    }
     return std::make_unique<ast::PointerTypeAST>(std::move(baseType), startLoc);
}

// Referans tipi ayrıştırır (&T, &mut T)
std::unique_ptr<ast::TypeAST> Parser::parseReferenceType() {
    SourceLocation startLoc = getCurrentToken().loc;
    expect(lexer::TokenType::Op_BitAnd, "'&' bekleniyor (referans tipi için)");

    bool isMut = false;
    if (consumeIf(lexer::TokenType::Kw_mut)) {
        isMut = true;
    }

    auto baseType = parseType(); // Taban tipi ayrıştır
     if (!baseType) {
        parseError("Referans '&' veya '&mut' işaretinden sonra tip bekleniyor.");
        // Hata sonrası toparlanma
    }

    return std::make_unique<ast::ReferenceTypeAST>(std::move(baseType), isMut, startLoc);
}

// Dizi tipi ayrıştırır ([T] veya [T; N])
std::unique_ptr<ast::TypeAST> Parser::parseArrayType() {
     SourceLocation startLoc = getCurrentToken().loc;
     expect(lexer::TokenType::Punc_OpenBracket, "'[' bekleniyor (dizi tipi için)");

     auto elementType = parseType(); // Element tipini ayrıştır
     if (!elementType) {
          parseError("Dizi tipi '[ ' işaretinden sonra element tipi bekleniyor.");
          // Hata toparlanma
          // Kapanan ']' veya ';' bulana kadar atla
          while(!peekToken(lexer::TokenType::Punc_CloseBracket) && !peekToken(lexer::TokenType::Punc_Semicolon) && !peekToken(lexer::TokenType::EndOfFile)) consumeToken();
          consumeIf(lexer::TokenType::Punc_CloseBracket);
          return nullptr; // Ayrıştırma başarısız
     }

    std::unique_ptr<ast::ExprAST> sizeExpr = nullptr;
    if (consumeIf(lexer::TokenType::Punc_Semicolon)) { // Boyut belirtilmiş ([T; N])
        sizeExpr = parseExpression(); // Boyut ifadesini ayrıştır (compile-time sabit olmalı, sema kontrol eder)
        if (!sizeExpr) {
             parseError("Dizi tipi ';' işaretinden sonra boyut ifadesi bekleniyor.");
             // Hata toparlanma
        }
    }

     expect(lexer::TokenType::Punc_CloseBracket, "']' bekleniyor (dizi tipi için)");

     return std::make_unique<ast::ArrayTypeAST>(std::move(elementType), std::move(sizeExpr), startLoc);
}

// Tuple tipi ayrıştırır ((T1, T2))
std::unique_ptr<ast::TypeAST> Parser::parseTupleType() {
    SourceLocation startLoc = getCurrentToken().loc;
    expect(lexer::TokenType::Punc_OpenParen, "'(' bekleniyor (tuple tipi için)");

    std::vector<std::unique_ptr<ast::TypeAST>> elementTypes;
    // Kapanan ')' gelene kadar element tiplerini ayrıştır
    while (!peekToken(lexer::TokenType::Punc_CloseParen) && !peekToken(lexer::TokenType::EndOfFile)) {
        if (auto elementType = parseType()) {
            elementTypes.push_back(std::move(elementType));
            if (!peekToken(lexer::TokenType::Punc_CloseParen)) {
                expect(lexer::TokenType::Punc_Comma, "Tuple tipi elementleri arasında ',' bekleniyor.");
            }
        } else {
            parseError("Tuple tipi elementi bekleniyor.");
             synchronize();
             if (peekToken(lexer::TokenType::Punc_CloseParen) || peekToken(lexer::TokenType::EndOfFile)) break;
        }
    }
    expect(lexer::TokenType::Punc_CloseParen, "')' bekleniyor (tuple tipi için)");

    // Not: Tek elementli (T,) bir tuple ile sadece parantezli ifade (T) arasındaki farkı burada ele almanız gerekebilir.
    // Genellikle (T,) bir tuple tipi olarak kabul edilirken, (T) sadece T'nin kendisidir.
    // parseType içinde parsePrimaryExpr gibi parantezli ifade ayrıştırma da olduğu için bu çakışabilir.
    // Tuple tipinin tanımını netleştirmek önemlidir.

    return std::make_unique<ast::TupleTypeAST>(std::move(elementTypes), startLoc);
}


// Option<T> tipini ayrıştırır (D++'a özgü)
std::unique_ptr<ast::TypeAST> Parser::parseOptionType() {
    SourceLocation startLoc = getCurrentToken().loc; // 'Option' tokenının lokasyonu
    // 'Option' identifier token'ı parseType içinde tüketildi.

    // '<' jenerik argüman başlangıcını bekle
    expect(lexer::TokenType::Op_LessThan, "'Option' tipi için '<' bekleniyor.");

    auto innerType = parseType(); // T tipini ayrıştır
    if (!innerType) {
        parseError("'Option<T>' tipi için T bekleniyor.");
        // Hata toparlanma
    }

    // '>' jenerik argüman sonunu bekle
    expect(lexer::TokenType::GreaterThan, "'Option<T>' tipi için '>' bekleniyor.");

    return std::make_unique<ast::OptionTypeAST>(std::move(innerType), startLoc);
}

// Result<T, E> tipini ayrıştırır (D++'a özgü)
std::unique_ptr<ast::TypeAST> Parser::parseResultType() {
     SourceLocation startLoc = getCurrentToken().loc; // 'Result' tokenının lokasyonu
     // 'Result' identifier token'ı parseType içinde tüketildi.

     // '<' jenerik argüman başlangıcını bekle
     expect(lexer::TokenType::Op_LessThan, "'Result' tipi için '<' bekleniyor.");

     auto okType = parseType(); // T tipini ayrıştır
     if (!okType) {
         parseError("'Result<T, E>' tipi için T bekleniyor.");
          // Hata toparlanma
     }

     expect(lexer::TokenType::Punc_Comma, "'Result<T, E>' tipi için T'den sonra ',' bekleniyor.");

     auto errType = parseType(); // E tipini ayrıştır
      if (!errType) {
         parseError("'Result<T, E>' tipi için ',' işaretinden sonra E bekleniyor.");
          // Hata toparlanma
     }

     expect(lexer::TokenType::GreaterThan, "'Result<T, E>' tipi için '>' bekleniyor.");

     return std::make_unique<ast::ResultTypeAST>(std::move(okType), std::move(errType), startLoc);
}


// TODO: parseFunctionDecl(), parseStructDecl(), parseTraitDecl(), parseImplDecl(), parseEnumDecl()
// Bu fonksiyonları da AST.h'da tanımladığınız struct'ları oluşturacak şekilde implement etmeniz gerekecektir.
// Her biri, ilgili anahtar kelimeyi bekleyecek, gerekli alt öğeleri (parametre listesi, fonksiyon gövdesi, alanlar, metodlar vb.)
// parseParameterList, parseType, parseBlockStmt gibi diğer fonksiyonları çağırarak ayrıştıracaktır.


// Örnek: parseFunctionDecl başlangıcı

std::unique_ptr<ast::FunctionDeclAST> Parser::parseFunctionDecl() {
    SourceLocation startLoc = getCurrentToken().loc;
    expect(lexer::TokenType::Kw_fn, "'fn' veya 'function' anahtar kelimesi bekleniyor"); // D++'ta fn mi kullanacağız D'deki function mı? fn varsayalım

    // Fonksiyon adını ayrıştır
    SourceLocation nameLoc = getCurrentToken().loc;
    std::string funcName = getCurrentToken().value;
    expect(lexer::TokenType::Identifier, "Fonksiyon adı bekleniyor");

    // Parametre listesini ayrıştır
    auto params = parseParameterList(); // (param1, param2, ...)

    // Dönüş tipini ayrıştır (: ReturnType) - opsiyonel olabilir
    std::unique_ptr<ast::TypeAST> returnType = nullptr;
    if (consumeIf(lexer::TokenType::Punc_Colon)) {
        returnType = parseType();
        if (!returnType) {
             parseError("':' işaretinden sonra dönüş tipi bekleniyor.");
        }
    } else {
        // Dönüş tipi belirtilmezse void varsayılabilir. Void tipini temsil eden bir AST düğümü veya nullptr döndürme kararı verin.
         returnType = std::make_unique<ast::PrimitiveTypeAST>(lexer::TokenType::Kw_void, nameLoc); // Varsayılan dönüş tipi void
    }

    // Fonksiyon gövdesini ayrıştır (blok) veya ';' (bildirimler için)
     std::unique_ptr<ast::BlockStmtAST> body = nullptr;
     if (peekToken(lexer::TokenType::Punc_OpenBrace)) {
        body = parseBlockStmt();
     } else {
         // Gövde yoksa bildirimdir, sonunda ';' beklenir.
         expect(lexer::TokenType::Punc_Semicolon, "Fonksiyon bildiriminde ';' bekleniyor");
     }


    return std::make_unique<ast::FunctionDeclAST>(std::move(funcName), std::move(params), std::move(returnType), std::move(body), startLoc);
}

// Örnek: parseParameterList başlangıcı
 std::vector<std::unique_ptr<ast::ParameterAST>> Parser::parseParameterList() { ... }

// Örnek: parseParameter başlangıcı
 std::unique_ptr<ast::ParameterAST> Parser::parseParameter() { ... }




} // namespace parser
} // namespace dppc
